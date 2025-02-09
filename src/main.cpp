#include <Arduino.h>


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

// WiFi credentials
const char* ssid = "ssid";
const char* password = "wifi password";

// OpenWeatherMap API credentials
const char* apiKey = "account api key";
const char* location = "location "; // Location format: City,CountryCode
const char* weatherHost = "api.openweathermap.org";

// NTP server for time synchronization
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Adjust for GMT offset (0 for UTC)
const int daylightOffset_sec = 3600; // Adjust for daylight saving time if applicable

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables to alternate displays
unsigned long previousMillis = 0;
unsigned long displayInterval = 5000; // Switch display every 5 seconds
bool showWeather = false; // Toggle between time/date and weather

// Variables for weather
String temperature = "--";
String weatherCondition = "Unknown";

// Function prototypes
void fetchWeather();
void displayTimeAndDate();

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connected!");
  delay(2000);
  lcd.clear();

  // Initialize NTP for time synchronization
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  lcd.setCursor(0, 0);
  lcd.print("Syncing Time...");
  delay(2000);

  // Fetch initial weather data
  fetchWeather();
}

void loop() {
  unsigned long currentMillis = millis();

  // Alternate between time/date and weather display
  if (currentMillis - previousMillis >= displayInterval) {
    previousMillis = currentMillis;
    showWeather = !showWeather;

    if (showWeather) {
      fetchWeather(); // Update weather data before displaying
    }
  }

  if (showWeather) {
    // Display weather on LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C   "); // Clear remaining characters

    lcd.setCursor(0, 1);
    lcd.print("Weather: ");
    lcd.print(weatherCondition);
  } else {
    // Display time and date on LCD
    displayTimeAndDate();
  }

  delay(1000); // Refresh every second
}

void displayTimeAndDate() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    lcd.setCursor(0, 0);
    lcd.print("Time sync error ");
    lcd.setCursor(0, 1);
    lcd.print("Retrying...");
    return;
  }

  char timeStr[16]; // HH:MM:SS
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeInfo);

  char dateStr[16]; // DD/MM/YYYY
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeInfo);

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(timeStr);

  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(dateStr);
}

void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Construct the weather API URL
    String weatherURL = String("http://") + weatherHost + "/data/2.5/weather?q=" + location + "&appid=" + apiKey + "&units=metric";

    // Make HTTP GET request
    http.begin(weatherURL.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON for weather data
      int tempIndex = payload.indexOf("\"temp\":") + 7;
      int tempEnd = payload.indexOf(",", tempIndex);
      temperature = payload.substring(tempIndex, tempEnd);

      int weatherIndex = payload.indexOf("\"main\":\"") + 8;
      int weatherEnd = payload.indexOf("\"", weatherIndex);
      weatherCondition = payload.substring(weatherIndex, weatherEnd);
    } else {
      Serial.println("Error fetching weather data");
      temperature = "--";
      weatherCondition = "Unknown";
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected");
    temperature = "--";
    weatherCondition = "No WiFi";
  }
}
