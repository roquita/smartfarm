/*
  POST /sensormeasures HTTP/1.1
  Host: 137.184.189.75
  Content-Type: application/json
  Content-Length: 119

  {
  "sensor_code":"SF00021",
  "temperature":20.4,
  "humidity":1,
  "rssi":1,
  "supply_voltage":5.3
  }
*/


#define dht11_data 4
#define led_wifi_signal 18
#define led_upload_done 16
#define button_show_lcd GPIO_NUM_25
#define lcd_rs 14
#define lcd_rw 27
#define lcd_e 26
#define lcd_d4 19
#define lcd_d5 21
#define lcd_d6 22
#define lcd_d7 23
#define bat_pin 36

#include <WiFi.h>
#include <HTTPClient.h>
#include "Adafruit_LiquidCrystal.h"
#include "icons.h"

Adafruit_LiquidCrystal lcd(lcd_rs, lcd_rw, lcd_e, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

// wifi credenciales
const char* ssid = "TE_HACKEL_EL_INTER";
const char* password = "QWE&ASD%ZXC#!!";

// server endpoint
const char* serverName = "http://137.184.189.75/sensormeasures";

// variables
float temperature = 0.0;
float humidity = 0.0;
int rssi = 0;
float supply_voltage = 0.0;

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.createChar(0, bat0);
  lcd.createChar(1, bat1);
  lcd.createChar(2, bat2);
  lcd.createChar(3, bat3);
  lcd.createChar(4, wifi1);
  lcd.createChar(5, wifi2);
  lcd.createChar(6, wifi3);
  lcd.createChar(7, wifi4);

  pinMode(led_wifi_signal, OUTPUT);
  pinMode(led_upload_done, OUTPUT);
}

void loop() {

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
      {
        Serial.println("Wakeup caused by external signal using RTC_IO");
        bool wifi_is_connected = connect_to_wifi();
        update_readings();
        if (wifi_is_connected) {
          uplink_data();
        }
        show_lcd();
      }
      break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.println("Wakeup caused by timer");
      if (connect_to_wifi() == true) {
        update_readings();
        uplink_data();
      }
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default :
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      if (connect_to_wifi() == true) {
        update_readings();
        uplink_data();
      }
      break;
  }

  Serial.println(F("Going to sleep"));
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F(""));

  pinMode(button_show_lcd, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(button_show_lcd, 0);
  esp_sleep_enable_timer_wakeup(10 * 1000000);
  esp_deep_sleep_start();
}
void show_lcd() {
  // show temperature and humidity
  char bufer[50] = {0};
  sprintf(bufer, "T=%.2f  H=%.2f", temperature, humidity);
  lcd.setCursor(0, 0);
  lcd.print(bufer);

  // show battery level
  lcd.setCursor(4, 1);
  if (supply_voltage >= 3.9) {
    lcd.write(3);
  } else if (supply_voltage >= 3.7) {
    lcd.write(2);
  }  else if (supply_voltage >= 3.5) {
    lcd.write(1);
  }  else if (supply_voltage >= 3.3) {
    lcd.write(0);
  }  else  {
    lcd.print(F("X"));
  }

  // show wifi signal level
  lcd.setCursor(5, 1);
  if (WiFi.status() != WL_CONNECTED) {
    lcd.print(F("X"));
  } else if (rssi >= -50) {
    lcd.write(7);
  } else if (rssi >= -70) {
    lcd.write(6);
  }  else if (rssi >= -80) {
    lcd.write(5);
  }  else {
    lcd.write(4);
  }

  delay(5000);
  lcd.clear();
}
void update_readings() {
  temperature = random(150, 300) / 10.0;
  humidity = random(250, 800) / 10.0;
  rssi = WiFi.RSSI();
  supply_voltage = (2.0 * (analogRead(bat_pin) * 3.3 / 4095.0) + 0.3);

  Serial.println(F("UPDATE READINGS:"));
  Serial.print(F("temperature="));
  Serial.println(temperature);
  Serial.print(F("humidity="));
  Serial.println(humidity);
  Serial.print(F("rssi="));
  Serial.println(rssi);
  Serial.print(F("supply_voltage="));
  Serial.println(supply_voltage);
}
bool connect_to_wifi() {
  WiFi.begin(ssid, password);
  Serial.println(F("Connecting"));
  int counter = 0;
  while ( (WiFi.status() != WL_CONNECTED) && (counter <= 10)) {
    delay(500);
    counter++;
    Serial.print(".");
  }
  Serial.println(F(""));
  Serial.print(F("Connected to WiFi network with IP Address: "));
  Serial.println(WiFi.localIP());

  bool is_connected = WiFi.status() == WL_CONNECTED;
  if (!is_connected) {
    Serial.println(F("Wifi disconnected."));
    digitalWrite(led_wifi_signal, LOW);
  } else {
    digitalWrite(led_wifi_signal, HIGH);
  }
  return is_connected;
}

void uplink_data() {
  //Check WiFi connection status
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);

  http.addHeader(F("Content-Type"), F("application/json"));

  char bufer[200] = {0};
  sprintf(bufer, "{"
          "\"sensor_code\":\"SF00021\","
          "\"temperature\":%.2f,"
          "\"humidity\":%.2f,"
          "\"rssi\":%i,"
          "\"supply_voltage\":%.2f"
          "}",
          temperature, humidity, rssi, supply_voltage);
  int httpResponseCode = http.POST(bufer);

  Serial.print(F("HTTP Response code: "));
  Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    digitalWrite(led_upload_done, HIGH);
    delay(500);
    digitalWrite(led_upload_done, LOW);
  }

  // Free resources
  http.end();

}
