#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- WIFI ----------------
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ---------------- TELEGRAM ----------------
#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ---------------- GPS ----------------
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- BUTTON ----------------
#define BUTTON_PIN 4
volatile bool sosPressed = false;

void IRAM_ATTR buttonISR() {
  sosPressed = true;
}

// ---------------- STARTUP LOGO ----------------
const unsigned char startup_logo [] PROGMEM = {
  0x00,0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,
  0x18,0x00
};

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // OLED INIT
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
    while(true);
  }

  showStartupScreen();

  // WIFI CONNECT
  displayStatus("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.setInsecure();

  displayStatus("WiFi Connected");
  delay(1000);
}

// ---------------- LOOP ----------------
void loop() {

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  updateMainDisplay();

  if (sosPressed) {
    sendSOS();
    sosPressed = false;
  }
}

// ---------------- FUNCTIONS ----------------

void showStartupScreen() {

  display.clearDisplay();
  display.drawBitmap(60, 10, startup_logo, 8, 8, WHITE);

  display.setTextSize(2);
  display.setCursor(15,30);
  display.println("SAFETY");

  display.setCursor(30,50);
  display.println("BAND");

  display.display();
  delay(2000);
  display.clearDisplay();
}

void displayStatus(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(msg);
  display.display();
}

void updateMainDisplay() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);

  // WiFi Status
  if (WiFi.status() == WL_CONNECTED)
    display.println("WiFi: Connected");
  else
    display.println("WiFi: Not Connected");

  // GPS Status
  if (gps.location.isValid()) {
    display.println("GPS: Fixed");
    display.print("Lat: ");
    display.println(gps.location.lat(), 6);
    display.print("Lng: ");
    display.println(gps.location.lng(), 6);
  } else {
    display.println("GPS: Searching...");
  }

  display.println("----------------");
  display.println("Press Button for SOS");

  display.display();
  delay(1000);
}

void sendSOS() {

  displayStatus("Sending SOS...");

  if (gps.location.isValid()) {

    float lat = gps.location.lat();
    float lng = gps.location.lng();

    String message = "🚨 EMERGENCY ALERT 🚨\n";
    message += "User Needs Help!\n";
    message += "Location:\n";
    message += "https://www.google.com/maps?q=";
    message += String(lat,6);
    message += ",";
    message += String(lng,6);

    bot.sendMessage(CHAT_ID, message, "");

    displayStatus("SOS Sent Successfully");
    delay(2000);

  } else {
    displayStatus("GPS Not Fixed!");
    delay(2000);
  }
}
