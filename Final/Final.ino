#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <DHT.h> // Include the DHT library

#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
//SDA A4
//SCL A5
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

int pset = 8;
int phour = 3;
int pmin = 10;
int pok = 12; // New button for confirming alarm setting
int pexit = 11;
int buzzer = 6; // Define the buzzer pin
int h = 0;
int m = 0;
bool activate = false;
DateTime now;

// Debouncing parameters
const unsigned long debounceDelay = 50; // milliseconds
unsigned long lastDebounceTimeSet = 0;
unsigned long lastDebounceTimeHour = 0;
unsigned long lastDebounceTimeMin = 0;
unsigned long lastDebounceTimeOK = 0; // Debounce time for OK button
unsigned long lastDebounceTimeExit = 0;

// Timing variables for temperature and humidity display
unsigned long previousMillis = 0;
const long interval = 5000; // Interval for displaying temperature and humidity readings (in milliseconds)

void setup() {
  Serial.begin(9600);
  pinMode(pset, INPUT_PULLUP); // Using internal pull-up resistors
  pinMode(phour, INPUT_PULLUP);
  pinMode(pmin, INPUT_PULLUP);
  pinMode(pok, INPUT_PULLUP); // Set OK button as input with pull-up resistor
  pinMode(pexit, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT); // Set buzzer pin as output
  digitalWrite(buzzer, LOW); // Ensure buzzer is initially off
  rtc.begin();
  lcd.init();
  lcd.backlight();
  dht.begin(); // Initialize DHT sensor
  Serial.println("Setup complete.");
}

// Debouncing function
bool debounce(int pin, unsigned long &lastDebounceTime) {
  bool reading = digitalRead(pin);
  if (reading != LOW) {
    lastDebounceTime = millis();
    return false;
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    return true;
  }
  return false;
}

void loop() {
  now = rtc.now();
  unsigned long currentMillis = millis();

  // Display current time and date for 5 seconds
  if (currentMillis - previousMillis <= 5000) {
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    if (now.hour() < 10) {
      lcd.print("0");
    }
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) {
      lcd.print("0");
    }
    lcd.print(now.minute());
    lcd.print(":");
    if (now.second() < 10) {
      lcd.print("0");
    }
    lcd.print(now.second());

    lcd.setCursor(0, 1);
    lcd.print("Date: ");
    if (now.day() < 10) {
      lcd.print("0");
    }
    lcd.print(now.day());
    lcd.print("/");
    if (now.month() < 10) {
      lcd.print("0");
    }
    lcd.print(now.month());
    lcd.print("/");
    lcd.print(now.year());
  } else if (currentMillis - previousMillis <= 7000) { // Display temperature and humidity for 2 seconds
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(dht.readTemperature());
    lcd.print("  C");

    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(dht.readHumidity());
    lcd.print("%");
  } else { // Reset the timer after displaying both sets of data
    previousMillis = currentMillis;
  }

  // Check if any button is pressed to enter alarm setting mode
  if (!activate && (debounce(pset, lastDebounceTimeSet) || debounce(phour, lastDebounceTimeHour) || debounce(pmin, lastDebounceTimeMin))) {
    activate = true;
    lcd.clear(); // Clear the display
    lcd.setCursor(0, 0);
    lcd.print("Set Alarm");
    lcd.setCursor(0, 1);
    lcd.print("Hour= ");
    lcd.print(h);
    lcd.setCursor(9, 1);
    lcd.print("Min= ");
    lcd.print(m);
    Serial.println("Alarm setting mode activated.");
  }

  if (activate) {
    // Alarm setting interface
    lcd.clear(); // Clear the display
    lcd.setCursor(0, 0);
    lcd.print("Set Alarm");
    lcd.setCursor(0, 1);
    lcd.print("Hour= ");
    lcd.print(h);
    lcd.setCursor(9, 1);
    lcd.print("Min= ");
    lcd.print(m);

    // Adjust hour and minute settings
    if (debounce(phour, lastDebounceTimeHour)) {
      h = (h + 1) % 24;
      Serial.print("Hour set to: ");
      Serial.println(h);
    }

    if (debounce(pmin, lastDebounceTimeMin)) {
      m = (m + 1) % 60;
      Serial.print("Minute set to: ");
      Serial.println(m);
    }

    // Confirm alarm setting and exit setting mode
    if (debounce(pok, lastDebounceTimeOK)) { // Check if OK button is pressed
      activate = false;
      lcd.clear(); // Clear the display
      Serial.println("Alarm set."); // Indicate that the alarm is set
      delay(1000); // Just for a brief pause to see the message
    }

    // Exit alarm setting mode
    if (debounce(pexit, lastDebounceTimeExit)) {
      activate = false;
      lcd.clear(); // Clear the display
      Serial.println("Exiting alarm setting mode.");
    }
  }

  // Display temperature and humidity every 5 seconds
  if (currentMillis - previousMillis >= 5000 && currentMillis - previousMillis <= 7000) {
    // Clear LCD
    lcd.setCursor(0, 3);
    lcd.print("             "); // Clear previous temperature and humidity readings

    // Read temperature and humidity
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if any reads failed and exit early (to try again).
    if (isnan(temperature) || isnan(humidity)) {
      lcd.setCursor(0, 3);
      lcd.print("Failed to read");
      Serial.println("Failed to read from DHT sensor");
      delay(2000);
      return;
    }

    // Display temperature and humidity
    lcd.setCursor(0, 3);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print("C");

    lcd.setCursor(0, 4);
    lcd.print("Humidity:");
    lcd.print(humidity);
    //lcd.print("%");

    // Display data in Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("C, Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
  }

  // Check if it's time for the alarm
  if (!activate && now.hour() == h && now.minute() == m) {
    tone(buzzer, 400, 300); // Activate the buzzer
    Serial.println("Alarm triggered.");
    lcd.setCursor(0, 5);
    lcd.println("Alarm triggered."); // Print to serial monitor
    delay(1000); // Wait for 1 second to prevent repeated triggering
  }

  delay(100); // Adjust delay as needed
}
