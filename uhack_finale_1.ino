#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <TinyGPS++.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ====== OBJECTS ======
MAX30105 particleSensor;
TinyGPSPlus gps;

HardwareSerial sim800(1);
HardwareSerial gpsSerial(2);

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ====== WIFI ======
const char* ssid = "LAVA LXX504";
const char* password = "00778100";

// ====== TELEGRAM ======
#define BOT_TOKEN "7660737248:AAHb_Nm7n6kM7pC9FSroH8-SgsMhC2JhvgU"
#define CHAT_ID "1205282706"

WiFiClientSecure clientSecure;
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);

// ====== SERVER ======
WiFiServer server(80);

// ====== PINS ======
int buttonPin = 4;

// ====== VARIABLES ======
int bpm = 0;
float temperature = 0;

float latitude = 0;
float longitude = 0;

int satelliteCount = 0;
bool gpsFix = false;

byte rates[4];
byte rateSpot = 0;
long lastBeat = 0;

#define GRAPH_POINTS 20
int bpmGraph[GRAPH_POINTS];
int graphIndex = 0;

String emergencyNumber = "+919369002269";

// ====== SETUP ======
void setup()
{
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);

  Wire.begin(21, 22);
  u8g2.begin();

  // UART
  sim800.begin(9600, SERIAL_8N1, 16, 17);
  gpsSerial.begin(9600, SERIAL_8N1, 32, 33);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  clientSecure.setInsecure();
  server.begin();

  // MAX30102
  if (!particleSensor.begin(Wire))
  {
    Serial.println("MAX30102 not found");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeIR(0x0A);
}

// ====== LOOP ======
void loop()
{
  readHeartRate();
  readGPS();

  temperature = temperatureRead();

  displayOLED();
  handleDashboard();
  checkSOS();

  delay(200);
}

// ====== HEART RATE ======
void readHeartRate()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue))
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    float beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= 4;

      bpm = 0;
      for (byte x = 0; x < 4; x++) bpm += rates[x];
      bpm /= 4;

      bpmGraph[graphIndex] = bpm;
      graphIndex = (graphIndex + 1) % GRAPH_POINTS;
    }
  }
}

// ====== GPS ======
void readGPS()
{
  while (gpsSerial.available())
  {
    gps.encode(gpsSerial.read());
  }

  if (gps.satellites.isValid())
    satelliteCount = gps.satellites.value();

  if (gps.location.isValid())
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    gpsFix = true;
  }
  else
  {
    gpsFix = false;
  }
}

// ====== OLED ======
void displayOLED()
{
  char tempStr[20];
  char bpmStr[20];
  char gpsStr[30];

  sprintf(tempStr, "Temp: %.1fC", temperature);
  sprintf(bpmStr, "BPM: %d", bpm);

  if (!gpsFix)
    sprintf(gpsStr, "GPS: Searching");
  else if (satelliteCount < 4)
    sprintf(gpsStr, "GPS: Weak (%d)", satelliteCount);
  else
    sprintf(gpsStr, "GPS: OK (%d)", satelliteCount);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0, 15, tempStr);
  u8g2.drawStr(0, 30, bpmStr);
  u8g2.drawStr(0, 45, gpsStr);

  if (digitalRead(buttonPin) == LOW)
    u8g2.drawStr(70, 15, "SOS!");

  u8g2.sendBuffer();
}

// ====== SOS ======
void checkSOS()
{
  if (digitalRead(buttonPin) == LOW)
  {
    Serial.println("SOS Triggered");

    // CALL
    sim800.println("ATD" + emergencyNumber + ";");
    delay(20000);
    sim800.println("ATH");

    // TELEGRAM
    String message = "🚨 SOS ALERT\n";
    message += "BPM: " + String(bpm) + "\n";
    message += "Location:\nhttps://maps.google.com/?q=";
    message += String(latitude, 6) + "," + String(longitude, 6);

    bot.sendMessage(CHAT_ID, message, "");

    delay(5000);
  }
}

// ====== DASHBOARD ======
void handleDashboard()
{
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client Connected");

  while (!client.available())
    delay(1);

  client.readStringUntil('\r');
  client.flush();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<html><head>");
  client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
  client.println("<meta http-equiv='refresh' content='2'>");
  client.println("</head><body>");

  client.println("<h1>Safety Band Dashboard</h1>");

  client.print("Temp: "); client.print(temperature); client.println("<br>");
  client.print("BPM: "); client.print(bpm); client.println("<br>");

  client.print("<a href='https://maps.google.com/?q=");
  client.print(latitude, 6);
  client.print(",");
  client.print(longitude, 6);
  client.println("'>Open Location</a><br>");

  client.println("<canvas id='chart'></canvas>");

  client.println("<script>");
  client.print("var data=[");

  for (int i = 0; i < GRAPH_POINTS; i++)
  {
    client.print(bpmGraph[i]);
    if (i < GRAPH_POINTS - 1) client.print(",");
  }

  client.println("];");
  client.println("new Chart(document.getElementById('chart'),{type:'line',data:{labels:data,datasets:[{label:'BPM',data:data,borderColor:'red'}]}});");
  client.println("</script>");

  client.println("</body></html>");

  client.stop();
}