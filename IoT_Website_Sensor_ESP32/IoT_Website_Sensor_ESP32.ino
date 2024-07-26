#include <WiFi.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

const char* ssid = "";
const char* password = "";

const int pinAnalog = 33;      // Initialization of the soil moisture sensor pin (analog pin for ESP32)
const int relay = 32;          // Initialization of the relay pin

#define DHTPIN 18              // Pin where the DHT11 is connected
#define DHTTYPE DHT11          // Type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);  // Change the address (0x27) if needed

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float power_mW = 0; 
float loadvoltage = 0;
int count = 0;

void setup()
{
  // Debug console
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  pinMode(pinAnalog, INPUT);   // Set analog pin as input
  pinMode(relay, OUTPUT);      // Set relay pin as output

  dht.begin();

//  if (! ina219.begin()) //ina219 {
//    Serial.println("Failed to find INA219 chip");
//    while (1) { delay(10); 
//    }

  lcd.init();
  lcd.backlight();
}

void loop()
{
  int analogValue = analogRead(pinAnalog);  // Read the value from the soil moisture sensor

  // Read temperature and humidity from DHT sensor
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();

  // Print the results to the LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Test Code :");
  lcd.print(count);

  count = count + 1; 


  
  lcd.setCursor(0, 1);
  lcd.print("Tanah: ");
  lcd.print(analogValue);

  lcd.setCursor(0, 2);
  lcd.print("Udara:");
  
  lcd.setCursor(5, 2);
  lcd.print("T:");
  lcd.print(temperature);
  lcd.print("C");

  lcd.setCursor(0, 3);
  lcd.print("H:");
  lcd.print(humidity);
  lcd.print("%");

  // Print the results to the serial monitor
  Serial.print("A0: ");
  Serial.print(analogValue);
  Serial.print(" | Temperature: ");
  Serial.print(temperature);
  Serial.print(" | Humidity: ");
  Serial.println(humidity);

  // Your existing moisture level conditions here...
  if (analogValue < 2500) {
    Serial.println(". Media Masih Basah");
    digitalWrite(relay, HIGH);
  }
  else if (analogValue > 2500 && analogValue < 4000) {
    Serial.println(". Kelembaban Tanah Masih Cukup");
    digitalWrite(relay, LOW);
  }
  else {
    Serial.println(". Perlu Tambahan Air");
    digitalWrite(relay, LOW);
  }

  // Send data to server
  sendDataToServer(temperature, humidity);

  //ina219
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  Serial.print("VOLTAGE: ");
  Serial.print(loadvoltage); Serial.print("\t");
  Serial.print("CURRENT:");
  Serial.print(current_mA); Serial.print("\t");
  Serial.print("POWER: ");
  Serial.print(power_mW);Serial.print("\n");

  delay(500);
}

void sendDataToServer(float suhu, int kelembapan) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = "http://.../suhu:" + String(suhu) + "/kelembapan:" + String(kelembapan);

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending GET request: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}