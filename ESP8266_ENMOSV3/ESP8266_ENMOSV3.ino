/*
ENMOS V3 
*/
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

//Konfigurasi Wifi
#define WIFI_SSID "GEDUNG.RF.IRA"
#define WIFI_PASSWORD "telkom135"
#define Username "brokerTTH"
#define Password "brokerTTH"


SoftwareSerial DataSerial(12, 13);

// MQTT Broker address
#define MQTT_HOST IPAddress(36 , 95 , 203 , 54)
//#define MQTT_HOST "broker.hivemq.com"
//MQTT port
#define MQTT_PORT 1884


// MQTT Topics
#define MQTT_PUB_RECORD "ENMOSV2/records"
#define MQTT_PUB_TEMP "ENMOSV2/Ul25z893BbFoTaJAvzJx0eTqgpHpAe/resource/temp"
#define MQTT_PUB_HUM  "ENMOSV2/Ul25z893BbFoTaJAvzJx0eTqgpHpAe/resource/hum"
#define MQTT_PUB_VOLT "ENMOSV2/Ul25z893BbFoTaJAvzJx0eTqgpHpAe/resource/volt" 
#define MQTT_PUB_FREQ "ENMOSV2/Ul25z893BbFoTaJAvzJx0eTqgpHpAe/resource/freq"
#define MQTT_PUB_WARNING "ENMOSV2/Warning"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;  // Stores last time temperature was published
const long interval = 10000;       // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

String arrData[3];

String volt, freq, temp, hum, Name_ID, warning;
float volt1, freq1, temp1, hum1;

unsigned long loopStartTime = millis();

void setup() {
  Serial.begin(9600);
  DataSerial.begin(19200);


  Serial.println();
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(Username, Password);

  connectToWifi();
}

void loop() {

  String Data = "";
  while (DataSerial.available() > 0) {
    Data += char(DataSerial.read());
  
  }

  //buang spasi datanya
  Data.trim();



  if (Data != "") {
    //parsing data (pecah data)
    int index = 0;
    for (int i = 0; i <= Data.length(); i++) {
      char delimiter = '#';
      if (Data[i] != delimiter)
        arrData[index] += Data[i];
      else
        index++;  //variabel index bertambah 1
    }



    //pastikan bahwa data yang dikirim lengkap (ldr, temp, hum)
    if (index == 4) {
      //tampilkan nilai sensor ke serial monitor
      Serial.println("Voltage : " + arrData[1]);  //volt
      Serial.println("Freq    : " + arrData[2]);  //Hz
      Serial.println("Temp    : " + arrData[3]);  //temp
      Serial.println("Humd    : " + arrData[4]);  //humd
      Serial.println();

      //isi variabel yang akan dikirim
      volt  = arrData[1];
      freq  = arrData[2];
      temp  = arrData[3];
      hum   = arrData[4];
      Name_ID = "Ul25z893BbFoTaJAvzJx0eTqgpHpAe";
      //warning = "Perangkat terhubung";
      

      volt1 = arrData[1].toFloat();
      freq1 = arrData[2].toFloat();
      temp1 = arrData[3].toFloat();
      hum1 = arrData[4].toFloat();

    //Notification Alert
    //ALERT TEMP
        if (temp1 < 20) {
        warning = "Low temperature : " + temp;
    }

        if (temp1 > 30) {
        warning = "High temperature : " + temp;
    }
    //ALERT HUMD
    if (hum1 < 40 ) {
        warning = "Low Humidity : " + hum;
    }
        if (hum1 > 80) {
        warning = "High Humidity : " + hum;
    }

    //ALERT VOLTAGE
    if (volt1 < 200 ) {
        warning = "Low voltage : " + volt;
    }
        if (volt1 > 225) {
        warning = "High Voltage : " + volt;
    }


      unsigned long currentMillis = millis();
      // Every X number of seconds (interval = 10 seconds)
      // it publishes a new MQTT message
      if (currentMillis - previousMillis >= interval) {
        // Save the last time a new reading was published
        previousMillis = currentMillis;

        String data_temp = temp;
        uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, data_temp.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
        Serial.printf("Message: %.2f \n", data_temp.c_str());

        String data_volt = volt;
        uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_VOLT, 1, true, data_volt.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_VOLT, packetIdPub2);
        Serial.printf("Message: %.2f \n", data_volt.c_str());

        String data_freq = freq;
        uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_FREQ, 1, true, data_freq.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_FREQ, packetIdPub3);
        Serial.printf("Message: %.2f \n", data_freq.c_str());

        String data_hum = hum;
        uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_HUM, 1, true, data_hum.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_HUM, packetIdPub4);
        Serial.printf("Message: %.2f \n", data_hum.c_str());

        String data_record = Name_ID + "#" + temp + "#" + volt + "#" + freq + "#" + hum;
        uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_RECORD, 1, true, data_record.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_RECORD, packetIdPub5);
        Serial.printf("Message: %.2f \n", data_record.c_str());

        String data_warning = Name_ID + "#" + warning ;
        uint16_t packetIdPub6 = mqttClient.publish(MQTT_PUB_WARNING, 1, true, data_warning.c_str());
        Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_WARNING, packetIdPub6);
        Serial.printf("Message: %.2f \n", data_warning.c_str());
        warning = "";


      }
    }

    delay(2500);

    warning = "";

      
    arrData[0] = "";
    arrData[1] = "";
    arrData[2] = "";
    arrData[3] = "";
    arrData[4] = "";
    

  }

}