#include <SoftwareSerial.h>
#include "Wire.h"
#include "TFT_eSPI.h"
#include <SHT31.h>
#include "RTC_SAMD51.h"
#include "DateTime.h"
#include "rpcWiFi.h"
#include <SPI.h>
#include <ModbusMaster.h>
#include "Free_Fonts.h"
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

unsigned long previousMillis = 0; //Looping layar
unsigned long previousMillis2 = 0; //Looping data pengiriman
unsigned long previousMillis3 =0; //Looping read dan write ke sd card
const long INTERVAL = 11100; //Waktu data dikirim
const long interval = 60000; //waktu read dan write ke sd card
const unsigned long backlightInterval = 180000; // waktu menyala
//const unsigned long screenOnTime = 21600000; // tidak dipakai

RTC_SAMD51 rtc;

typedef struct {
  float V;
  float F;
} READING;

uint32_t start;
uint32_t stop;
SHT31 sht;
TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);
uint32_t updateTime = 0;

// const char* ssid = "Redmi";
// const char* password =  "12345678";
// const char* ssid = "Chamber.RF.";
// const char* password =  "telkom135";
const char* ssid = "IRA-LT1";
const char* password =  "berakhlak";

float humidity, temperature, voltage, freq;

SoftwareSerial serial(D2,D3);
SoftwareSerial SerialMod(D1,D0);
ModbusMaster node;

#define LCD_BACKLIGHT (72Ul)

File myFile;

bool screenOn = false;

void setup() {

  // put your setup code here, to run once:
  serial.begin(19200);
  Serial.begin(115200);
  SerialMod.begin(9600);
  
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);

  delay(500);
  node.begin(17, SerialMod);
  Wire.begin();
  sht.begin(0x44);    //SHT31 I2C Address
 //rtc.begin();
  rtc.begin();
 
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  rtc.adjust(now);

  pinMode(WIO_LIGHT, INPUT);
  Wire.setClock(100000);

  WiFi.mode(WIFI_OFF);
  WiFi.disconnect();
  delay(100);
  
  tft.begin();
  tft.init();
  tft.setRotation(3);
  spr.createSprite(TFT_HEIGHT,TFT_WIDTH);
  spr.setRotation(3);
  updateTime = millis();
  
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSansOblique12pt7b);
  tft.println(" ");
  tft.drawString("Scanning Network!",8,5);
  delay(1000);
  
  int n = WiFi.scanNetworks();
  int i = 0;
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setCursor(87,22);
  tft.print(n);
  tft.println(" networks found");
  do {
    i++;
    tft.println(String (i)+ ". " + String(WiFi.SSID(i)) + String (WiFi.RSSI(i)));
  } while (i != n);
  Serial.println("scan done");
  // delay(100);
  
  WiFi.begin(ssid, password);
  pinMode(WIO_BUZZER, OUTPUT);
  tft.fillScreen(TFT_BLACK);

  digitalWrite(LCD_BACKLIGHT, HIGH);


  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("initialization failed!");
    //while (1);
  }
  Serial.println("initialization done.");

  // myFile = SD.open("wifi.txt", FILE_READ); //Read Mode
  // if (myFile) {
  //   Serial.println("wifi.txt:");

  //   // read from the file until there's nothing else in it:
  //   while (myFile.available()) {
  //     Serial.write(myFile.read());
  //     Serial.println(myFile.read());
  //   }
  //   // close the file:
  //   myFile.close();
  // } else {
  //   // if the file didn't open, print an error:
  //   Serial.println("gagal membuka file");
  // }
  //     String ssidName;
  //     String pass;
  //     String wifi = String(ssidName) + "#" + String(pass);
  //     Serial.println(wifi);
  //     serial.println(wifi);

}


void loop() {
  //Time mematikan layar
  unsigned long currentMillis = millis();
  // Deklarasi variabel status
  bool sensorReadingInProgress = false;

  if (currentMillis - previousMillis >= backlightInterval) {
    Serial.print("Mati");
    previousMillis = currentMillis;
    digitalWrite(LCD_BACKLIGHT, LOW);
  }
  // Tombol menyalakan
  if (!screenOn && digitalRead(WIO_5S_PRESS) == LOW) {
    screenOn = true;
    Serial.println("nyala");
    unsigned long screenOnStart = millis();
    // loop to keep the screen on for screenOnTime
    digitalWrite(LCD_BACKLIGHT, HIGH);
    Serial.println("nyala2");
  
    screenOn = false;
  }
  // Lakukan pembacaan sensor dan komunikasi Modbus secara non-blocking
  if (!sensorReadingInProgress) {
    sensorReadingInProgress = true;
    //kodingan utama
    sht.read();
    float t = sht.getTemperature();
    float h = sht.getHumidity();

    DateTime now = rtc.now();

    delay(1000);

    READING r;
    uint8_t result = node.readHoldingRegisters(0002,10); 

    if (result == node.ku8MBSuccess){
    r.V = (float)node.getResponseBuffer(0x00)/100;
    r.F = (float)node.getResponseBuffer(0x09)/100;
    tft.setTextColor(TFT_GREEN); //Setting text color 
    tft.drawString("Modbus",112,5); 
    }
  
    //kondisi 2 kWhmeter
    if (result != node.ku8MBSuccess){
      r.V = 0;
      r.F = 0;
      tft.setTextColor(TFT_RED); //Setting text color 
      tft.drawString("Modbus",112,5);   
    }
    tft.setFreeFont(&FreeSans9pt7b);
    spr.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);

    Serial.print("Cek koneksi");

    spr.createSprite(70, 20);
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_WHITE);
    spr.setCursor(18,14.5);
    spr.setFreeFont(&FreeSans9pt7b);
    spr.print(now.hour(), DEC);
    spr.print(':');
    spr.print(now.minute(), DEC);
    spr.pushSprite(255,5); //H,V
    spr.deleteSprite();

    tft.drawFastVLine(160,25,220,TFT_DARKCYAN);
    tft.drawFastHLine(0,135,320,TFT_DARKCYAN);
    tft.drawFastHLine(0,25,320,TFT_DARKCYAN);

    //ALERT TEMP
    // if (t < 22 || t > 25) {
    //  analogWrite(WIO_BUZZER,128);
    //  delay(400);
    //  analogWrite(WIO_BUZZER,0);
    //  delay(10);
    //  analogWrite(WIO_BUZZER,128);
    //  delay(400);
    //  analogWrite(WIO_BUZZER,0);
    // }
    //ALERT HUMD
    // if (h < 42 || h > 79) {
    //  analogWrite(WIO_BUZZER,128);
    //  delay(400);
    //  analogWrite(WIO_BUZZER,0);
    //  delay(10);
    //  analogWrite(WIO_BUZZER,128);
    //  delay(400);
    //  analogWrite(WIO_BUZZER,0);
    // }

    unsigned long currentMillis3 = millis();
    delay(1000);

    if (currentMillis3 - previousMillis3 >= interval) { // Setiap 20 detik
      previousMillis3 = currentMillis3; // Update waktu terakhir

      //Menulis datalog
      static char payload[255];
      char payloadBase64[255];

      memset(payload, 0x00, sizeof(payload));
      DateTime lastYear = DateTime(now.unixtime() - 31536000);
      sprintf(payload, "/log-%04d%02d.csv", lastYear.year(), lastYear.month());

      memset(payload, 0x00, sizeof(payload));
      sprintf(payload, "log-%04d%02d.csv", now.year(), now.month());
      delay(10);
      File logging;
      float crcStatus = 1.0; // Ganti 0.0 dengan nilai default yang sesuai
      float sensorStatus = 1.0; // Ganti 0.0 dengan nilai default yang sesuai
      float temperatureValue = 0.0; // Ganti 0.0 dengan nilai default yang sesuai
      float humidityValue = 0.0; // Ganti 0.0 dengan nilai default yang sesuai
      float voltageValue = 0.0; // Ganti 0.0 dengan nilai default yang sesuai
      float fequencyValue = 0.0; // Ganti 0.0 dengan nilai default yang sesuai
      logging = SD.open(payload, FILE_APPEND);
      if (logging)
      {
        Serial.print("Writing to ");
        Serial.println(payload);
        memset(payload, 0x00, sizeof(payload));
        sprintf(payload, "%04d/%02d/%02d %02d:%02d:%02d;%.1f;%.1f;%.1f;%.1f\r\n",
                now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
                t, h, r.V, r.F);
        delay(10);

        Serial.print(payload);
        logging.print(payload);
        delay(100);
        // close the file:
        logging.close();
        Serial.println("done.");
        memset(payload, 0x00, sizeof(payload));
        delay(100);
      }
      else
      {
        // if the file didn't open, print an error:
        Serial.println("error opening log");
      }





      // myFile = SD.open("datalog.txt", FILE_APPEND);
      // if (myFile) {
      //   myFile.print(now.hour());
      //   myFile.print(':');
      //   myFile.print(now.minute());
      //   myFile.print(" | ");
      //   myFile.print(" F : ");
      //   myFile.print(r.F);
      //   myFile.print(" | ");
      //   myFile.print(" V : ");
      //   myFile.print(r.V);
      //   myFile.print(" | ");
      //   myFile.print(" T : ");
      //   myFile.print(t);
      //   myFile.print(" | ");
      //   myFile.print(" H : ");
      //   myFile.println(h);
      //   myFile.close(); // Jangan lupa untuk menutup file setelah selesai menulis
      //   Serial.println("Writed log");
      // } else {
      //   Serial.println("Gagal membuka file datalog.txt");
      // }
    }

    unsigned long currentMillis1 = millis();
    if (currentMillis1 - previousMillis2 >= INTERVAL) {
      previousMillis2 = currentMillis1;
      String datakirim = String("1#") + String(r.V,1)+ "#" + String(r.F,1) + "#" + String(t,1) + "#" + String(h,1);
      Serial.println(datakirim);
      serial.println(datakirim);
    }

    //Kuadran 1
    spr.createSprite(158, 102);
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_WHITE);
    spr.drawString("Temp",55,8);
    //spr.setTextSize(2);
    spr.setTextColor(TFT_GREEN);
    spr.setFreeFont(FSSO24);
    spr.drawString(String(t,1),25,36);
    spr.setTextSize(1);
    spr.setTextColor(TFT_YELLOW); 
    spr.setFreeFont(&FreeSans9pt7b);
    spr.drawString("C",113,85);
    spr.pushSprite(0,27); 
    spr.deleteSprite();

    unsigned long currentMillis = millis();

    //Kuadran 2
    spr.setFreeFont(&FreeSans9pt7b);
    spr.createSprite(158, 102);
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_WHITE);
    spr.setTextSize(1);
    spr.drawString("Voltage",50,8);
    //spr.setTextSize(3);
    spr.setTextColor(TFT_GREEN);
    spr.setFreeFont(FSSO24);
    spr.drawString(String(r.V,1),11,36);
    spr.setTextSize(1);
    spr.setTextColor(TFT_YELLOW);
    spr.setFreeFont(&FreeSans9pt7b);
    spr.drawString("VAC",105,85);
    spr.pushSprite(162,27); 
    spr.deleteSprite();

    //Kuadran 3
    spr.createSprite(158, 100);
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_WHITE);
    spr.setTextSize(1);
    spr.drawString("Freq",62,6);
    //spr.setTextSize(3);
    spr.setTextColor(TFT_GREEN);
    spr.setFreeFont(FSSO24);
    spr.drawString(String(r.F,1),26,34);
    spr.setTextSize(1);
    spr.setTextColor(TFT_YELLOW);
    spr.setFreeFont(&FreeSans9pt7b);
    spr.drawString("Hz",108,82);  
    spr.pushSprite(162,137); //H,V
    spr.deleteSprite();

    //Kuadran 4
    spr.createSprite(158, 100);
    spr.fillSprite(TFT_BLACK);
    spr.setTextColor(TFT_WHITE);
    spr.drawString("Humidity",43,6);
    //spr.setTextSize(2);
    spr.setTextColor(TFT_GREEN);
    spr.setFreeFont(FSSO24);
    spr.drawString(String(h,1),25,34);
    spr.setTextSize(1);
    spr.setTextColor(TFT_YELLOW);
    spr.setFreeFont(&FreeSans9pt7b);
    spr.drawString("%RH",65,82);
    spr.pushSprite(0,137); 
    spr.deleteSprite();

    if (h==0) {
      tft.setTextSize(1);
      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextColor(TFT_RED);
      tft.drawString("Sensor",183,5);
      //  delay(500);
    }
    if (h != 0) {
     tft.setTextSize(1);
     tft.setFreeFont(&FreeSans9pt7b);
     tft.setTextColor(TFT_GREEN);
     tft.drawString("Sensor",183,5);
    }
    //warna WiFi  
    if (WiFi.status() != WL_CONNECTED) { 
      tft.setTextSize(1);
      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextColor(TFT_GREEN);
      tft.drawString("WiFi",8,5);
      tft.drawString("HTTP",53,5);
      WiFi.begin(ssid, password);
    }
    if (WiFi.status() == WL_CONNECTED) {
      tft.setTextSize(1);
      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextColor(TFT_GREEN);
      tft.drawString("WiFi",8,5);
      tft.drawString("HTTP",53,5);
    } 
    sensorReadingInProgress = false;
  }
}


