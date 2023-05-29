#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

// LoRa port configuration
const byte rxPin = 16;
const byte txPin = 17;
SoftwareSerial myLoRa(rxPin, txPin);

// Replace with your network credentials
const char* WIFI_SSID = "MON";
const char* WIFI_PASSWORD = "0374207107";
//const char* WIFI_SSID = "Nguyen Luong Truong Vu";
//const char* WIFI_PASSWORD = "25032003";

// configure firebase
#define FIREBASE_HOST "https://thinhdeptrai-bb5b5-default-rtdb.firebaseio.com/"      // the project name address from firebase id
#define FIREBASE_AUTH "gmN921dmk0wPegCfEb7eVA7QqBHAY4Cwl8MmJkQk"    // the secret key generated from firebase

// create variable
FirebaseData firebaseData;
StaticJsonDocument<200> jsonDoc;

char dataRcvd[200];
int pos = 0;
unsigned long previousMillis = 0;
unsigned long interval = 500;
unsigned long times=millis();

 // led status received from firebase
String fireStatus = "";  

// set data
void setData(const char* field, const char* value) {
  if(Firebase.setString(firebaseData, field, value)) {
    Serial.println("Set field success");     
  } else {
    Serial.println("Set field failure");
  }
}

//get data
String getData(const char* field) {
  Firebase.getString(firebaseData, field);
  return firebaseData.stringData();
}

void setup() {
  // Initialize Serial port
  Serial.begin(9600);
  while (!Serial) continue;
  delay(5000);

  // Define pin modes for TX and RX
//  pinMode(rxPin, INPUT);
//  pinMode(txPin, OUTPUT);
  
  // Set the baud rate for the LoRa module
   myLoRa.begin(9600);
   
   myLoRa.println("AT+MODE=LoRa");
myLoRa.println("AT+ADDR=1"); // Đặt địa chỉ LoRa module
myLoRa.println("AT+SF=12"); // Đặt spreading factor (SF) là 12
myLoRa.println("AT+CR=4/5"); // Đặt coding rate (CR) là 4/5
myLoRa.println("AT+BAND=915000000"); // Đặt tần số hoạt động là 915MHz

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status () != WL_CONNECTED  ) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi connected");

  // kết nối FireBase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
     
  setData("LED_STATUS", "OFF") ;
  setData("PHUN_SUONG", "OFF") ;
  setData("QUAT", "OFF") ;
  setData("DEN", "OFF") ;
  setData("PHUN_NUOC", "OFF") ;
  setData("AUTO", "OFF") ;

  Firebase.reconnectWiFi("true");
  Serial.println("Firebase initialized");

  // fire base
  Firebase.setFloat(firebaseData,"/humidity", 0);
  Firebase.setFloat(firebaseData,"/temperature", 0);
  Firebase.setFloat(firebaseData,"/light", 0);
  Firebase.setFloat(firebaseData,"/moisture", 0);

  // set threshold 
  Firebase.setFloat(firebaseData,"/HUMIDITY_THRESHOLD_MAX", 65);
  Firebase.setFloat(firebaseData,"/HUMIDITY_THRESHOLD_MIN", 55);
  Firebase.setFloat(firebaseData,"/TEMPERATURE_THRESHOLD_MAX", 30);
  Firebase.setFloat(firebaseData,"/TEMPERATURE_THRESHOLD_MIN", 18);
  Firebase.setFloat(firebaseData,"/MOISTURE_THRESHOLD_MIN", 60);
  Firebase.setFloat(firebaseData,"/LIGHT_THRESHOLD_MIN", 500);


    // Configure frequency band (868MHz)
  myLoRa.write(0x06);
  myLoRa.write(0xD9);
  myLoRa.write(0x1B);

  // Configure channel size (125kHz)
  myLoRa.write(0x07);
  myLoRa.write(0x86);
  
  // Configure spreading factor (SF 7)
  myLoRa.write(0x08);
  myLoRa.write(0x07);

  // Configure coding rate (4/5)
  myLoRa.write(0x0A);
  myLoRa.write(0x02);
  
  // Configure transmitting power (14dBm)
  myLoRa.write(0x09);
  myLoRa.write(0x0E);
  
}

void loop() {
  String jsonString;

  unsigned long currentMillis = millis();
  if((currentMillis - previousMillis > interval)) {
    // control relay  
    String ledStatus = getData("LED_STATUS");
    String phunSuong = getData("PHUN_SUONG");
    String quatSuong = getData("QUAT");
    String den = getData("DEN");
    String phunNuoc = getData("PHUN_NUOC");
    String _auto = getData("AUTO");

    // Thêm dữ liệu vào đối tượng JSON
    if (ledStatus == "ON") {
        myLoRa.write(static_cast<int>(0x00));
    } else {
        // jsonDoc["ledStatus"] = 0;   
        myLoRa.write(static_cast<int>(0x01));
    }
    
    if (phunSuong == "ON") {
        // jsonDoc["phunSuong"] = 1;
        myLoRa.write(static_cast<int>(0x02));
    } else {
        // jsonDoc["phunSuong"] = 0;   
        myLoRa.write(static_cast<int>(0x03));
    }
    
    if (quatSuong == "ON") {
        // jsonDoc["quat"] = 1;
        myLoRa.write(static_cast<int>(0x04));
    } else {
        // jsonDoc["quat"] = 0;   
        myLoRa.write(static_cast<int>(0x05));
    }
    
    if (den == "ON") {
        // jsonDoc["den"] = 1;
        myLoRa.write(static_cast<int>(0x06));
    } else {
        // jsonDoc["den"] = 0;   
        myLoRa.write(static_cast<int>(0x07));
    }
    
    if (phunNuoc == "ON") {
        // jsonDoc["phunNuoc"] = 1;
        myLoRa.write(static_cast<int>(0x08));
    } else {
        // jsonDoc["phunNuoc"] = 0;   
        myLoRa.write(static_cast<int>(0x09));
    }

    if (_auto == "ON") {
        myLoRa.write(static_cast<int>(0x10));
    } else {
        // jsonDoc["phunNuoc"] = 0;   
        myLoRa.write(static_cast<int>(0x11));
    }
     
      // Chuyển đối tượng JSON thành chuỗi
    //  serializeJson(jsonDoc, jsonString);
    //  
    //  // Gửi chuỗi JSON qua LoRa
    //  myLoRa.println(jsonString);
    
    previousMillis = currentMillis;
 }

 
  
 while (myLoRa.available() > 0){
    jsonString = myLoRa.readStringUntil('\n'); // Đọc chuỗi JSON từ LoRa
    // Phân tích cú pháp chuỗi JSON và trích xuất dữ liệu
    DeserializationError error = deserializeJson(jsonDoc, jsonString);
    if (error) {
      Serial.println("Parsing failed");
      return;
     }
    // Truy cập và xử lý dữ liệu từ chuỗi JSON
    float temp = jsonDoc["temp"];
    float humidity = jsonDoc["humidity"];
    float lightLux = jsonDoc["lightLux"];
    float moisture = jsonDoc["moisture"];
    
    // In ra Serial Monitor
    Serial.print("Temp: ");
    Serial.println(temp);
    Serial.print("humidity: ");
    Serial.println(humidity);
    Serial.print("lightLux: ");
    Serial.println(lightLux);
    Serial.print("moisture: ");
    Serial.println(moisture);
    // fire base
    Firebase.setFloat(firebaseData,"/humidity", humidity);
    Firebase.setFloat(firebaseData,"/temperature", temp);
    Firebase.setFloat(firebaseData,"/light", lightLux);
    Firebase.setFloat(firebaseData,"/moisture", moisture);
 };

  delay(1000);
};
