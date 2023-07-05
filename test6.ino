
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <WiFi.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <BH1750.h>
#include "RTClib.h"

unsigned long previousMillis = 0;
//unsigned long interval = 10000;
unsigned long interval = 7000;

// LoRa port configuration
const byte rxPin = 16;
const byte txPin = 17;
SoftwareSerial myLoRa(rxPin, txPin);

BH1750 lightMeter(0x23);
RTC_DS1307 rtc;
#define DS1307_ADDRESS 0x68


#define DHT_PIN 4
#define MOISTURE_PIN 34
#define DHT_TYPE DHT11
#define relayPin 5 // kết nối với chân EN của module relay
#define relayQuat 18 // kết nối với chân EN của module relay
#define relayDen 19 // kết nối với chân EN của module relay
#define relayMayPhunNuoc 15 // kết nối với chân EN của module relay
#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27,20,4);  // Khởi tạo đối tượng LCD với địa chỉ I2C và kích thước 20x4

// create variable
//FirebaseData firebaseData;
StaticJsonDocument<200> jsonDoc;

DHT dht(DHT_PIN, DHT_TYPE); 
int led = 2;   

// set
String ledStatus = "false";
String phunSuong = "false";
String quat = "false";
String den = "false";
String phunNuoc = "false";
String _auto = "false";

String ledStatusLcd = "F";
String phunSuongLcd = "F";
String quatLcd = "F";
String denLcd = "F";
String phunNuocLcd = "F";
String _autoLcd = "F";

// set threshold
#define HUMIDITY_THRESHOLD_MAX 65 
#define HUMIDITY_THRESHOLD_MIN 60 
#define TEMPERATURE_THRESHOLD_MAX 30
#define TEMPERATURE_THRESHOLD_MIN 18
#define MOISTURE_THRESHOLD_MAX 70 
#define MOISTURE_THRESHOLD_MIN 60
#define LIGHT_THRESHOLD_MIN_DAY 500  
#define LIGHT_THRESHOLD_MIN_NIGHT 0 

void setup()
{
  Serial.begin(9600);
  while (!Serial) continue;

  // Set the baud rate for the LoRa module
   myLoRa.begin(9600);


  // Gửi các lệnh AT để cấu hình LoRa module
  myLoRa.println("AT+MODE=LoRa");
  myLoRa.println("AT+ADDR=1"); // Đặt địa chỉ LoRa module
  myLoRa.println("AT+SF=12"); // Đặt spreading factor (SF) là 12
  myLoRa.println("AT+CR=4/5"); // Đặt coding rate (CR) là 4/5
  myLoRa.println("AT+BAND=915000000"); // Đặt tần số hoạt động là 915MHz
  
  Wire.begin(SDA_PIN,SCL_PIN);
  lcd.begin();                      // Khởi tạo LCD
  lcd.backlight();                 // Bật đèn nền LCD
  
  // cảm biến ánh sáng
  // SDA/SCl 
  // cảm biến ánh sáng
  lightMeter.begin();
  lightMeter.configure(BH1750::CONTINUOUS_HIGH_RES_MODE);  

  // rtc 
  rtc.begin(); 
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Module Tiny RTC I2C chưa được cấu hình ngày tháng!");
    // Bạn có thể thay đổi dòng trên để cài đặt ngày giờ mặc định nếu cần.
  };

  delay(5000);
    
  // set up pin 
  pinMode(2, OUTPUT); 
             
  dht.begin();
  Serial.println("DHT sensor initialized");
  pinMode(relayPin, OUTPUT);
  pinMode(relayQuat, OUTPUT);
  pinMode(relayDen, OUTPUT);
  pinMode(relayMayPhunNuoc, OUTPUT);

  lcd.clear();                     // Xóa màn hình LCD
  lcd.setCursor(0,0);              // Di chuyển con trỏ đến vị trí (0,0)
  lcd.print("T:");             // In ra chữ "Temp: "
  lcd.print(0);                    // In ra giá trị nhiệt độ đọc được
  lcd.print(" C");                 // In ra độ đơn vị đo nhiệt độ
  

  lcd.setCursor(10,0);              // Di chuyển con trỏ đến vị trí (0,1)
  lcd.print("H:");         // In ra chữ "Humidity: "
  lcd.print(0);                    // In ra giá trị độ ẩm đọc được
  lcd.print(" %");                 // In ra đơn vị đo độ ẩm
  
  lcd.setCursor(0,1);              // Di chuyển con trỏ đến vị trí (0,0)
  lcd.print("M:");             // In ra chữ "Temp: "
  lcd.print(0);                    // In ra giá trị nhiệt độ đọc được
  lcd.print(" %");                 // In ra độ đơn vị đo nhiệt độ
  
  lcd.setCursor(9,1);              // Di chuyển con trỏ đến vị trí (0,1)
  lcd.print("L:");         // In ra chữ "Humidity: "
  lcd.print(0);                    // In ra giá trị độ ẩm đọc được
  lcd.print(" lux");    
                     
  lcd.setCursor(0,2);              
  lcd.print("SNG: ");             
  lcd.print(phunSuongLcd);                                  

  lcd.setCursor(8,2);             
  lcd.print("NC: ");         
  lcd.print(phunNuocLcd);                                

  lcd.setCursor(14,2);              
  lcd.print("QT: ");             
  lcd.print(quatLcd);                    

  lcd.setCursor(0,3);             
  lcd.print("DN: ");         
  lcd.print(denLcd);    

  lcd.setCursor(8,3);             
  lcd.print("AUTO: ");         
  lcd.print(_autoLcd);  
};

// read humidity/temp
float* getTempHumidity() {
  static float result[2];
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read data from DHT sensor");
    result[0] = NAN;
    result[1] = NAN;
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    result[0] = humidity;
    result[1] = temperature;
  }
  return result;
}
//
//// read light
int readLight() {
  float result = 0;
  uint16_t lux = lightMeter.readLightLevel();

  if (isnan(lux)){
     Serial.println("Failed to read data from BH1750");
     result = NAN;
  } else {
    Serial.print("Light level: ");
    Serial.print(lux);
    Serial.println(" lux");

    result = lux;
  }

  return result;
};


// read moisture
float readMoisture() {
  float  result = 0;
  
  int moistureReading = 0;
  int value = 0;


// Chúng ta sẽ tạo một hàm for để đọc 10 lần giá trị cảm biến, sau đó lấy giá trị trung bình để được giá trị chính xác nhất.
  for(int i=0;i<=9;i++){
    moistureReading+=analogRead(MOISTURE_PIN);
     if(isnan(moistureReading)){
      Serial.println("Failed to read data from Moisture");
        if(i == 0) {
          i = 0;
         } else {
          i = i -1;
         };
        } 
  };
  
  value=moistureReading/10;
  int percent = map(value, 0, 4095, 0, 100);    // Set giá thang giá trị đầu và giá trị cuối để đưa giá trị về thang từ 0-100. 
                                                  // Cái này sẽ bằng thực nghiệm nhé
  percent=100-percent;                            // Tính giá trị phần trăm thực. Chuyển điện thế từ 3.3V ( khô ) thành 3.3V ( ẩm )
  Serial.print(percent);
  Serial.println('%');
  delay(200);
  moistureReading=0;
  result = percent;
  return result;
};

// control relay
void controlRelay(String field, int pin, String name) {
  if (field == "true") {                         // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned ON");                 
    digitalWrite(pin, HIGH);                                                         // make output ON
  }

  else if (field == "false") {              // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned OFF");
    digitalWrite(pin, LOW);                                                         // make output OFF
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
};



void loop() {
  // rtc time 
  DateTime now = rtc.now();

  // Đọc thời gian từ module Tiny RTC I2C
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  Serial.print("Ngay: ");
  Serial.print(day);
  Serial.print("/");
  Serial.print(month);
  Serial.print("/");
  Serial.print(year);
  Serial.print("  ");
  Serial.print("Gio: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);

  Serial.print("Hour: ");
  Serial.println(hour);

  String jsonString;

 float humidity = 0.0;
  float temperature = 0.0;

  int lightLux = 0;
  float moisture = 0.0;

  // send lora 
  unsigned long currentMillis = millis();
  if((currentMillis - previousMillis > interval)) {
  float* tempHumidity = getTempHumidity();
   humidity = tempHumidity[0];
   temperature = tempHumidity[1];

   lightLux = readLight();
   moisture = readMoisture();

  lcd.setCursor(0,0);              // Di chuyển con trỏ đến vị trí (0,0)
  lcd.print("T:");             // In ra chữ "Temp: "
  lcd.print(temperature);                    // In ra giá trị nhiệt độ đọc được
  lcd.print(" C");                 // In ra độ đơn vị đo nhiệt độ  
  
  lcd.setCursor(10,0);              // Di chuyển con trỏ đến vị trí (0,1)
  lcd.print("H:");         // In ra chữ "Humidity: "
  lcd.print(humidity);                    // In ra giá trị độ ẩm đọc được
  lcd.print(" %");                 // In ra đơn vị đo độ ẩm
 
  lcd.setCursor(0,1);              // Di chuyển con trỏ đến vị trí (0,0)
  lcd.print("M:");             // In ra chữ "Temp: "
  lcd.print(moisture);                    // In ra giá trị nhiệt độ đọc được
  lcd.print(" %");                 // In ra độ đơn vị đo nhiệt độ
    
  lcd.setCursor(9,1);              // Di chuyển con trỏ đến vị trí (0,1)
  lcd.print("L:");         // In ra chữ "Humidity: "
  lcd.print(lightLux);                    // In ra giá trị độ ẩm đọc được
  lcd.print(" lux");    

    
     // Thêm dữ liệu vào đối tượng JSON
   jsonDoc["temp"] = temperature;
   jsonDoc["humidity"] = humidity;
   jsonDoc["lightLux"] = lightLux;
   jsonDoc["moisture"] = moisture;
  
   // Chuyển đối tượng JSON thành chuỗi
   serializeJson(jsonDoc, jsonString);
   
   // Gửi chuỗi JSON qua LoRa
    myLoRa.println(jsonString);
    previousMillis = currentMillis;

  }

   while (myLoRa.available() > 0){
      byte dataRcvd = myLoRa.read();
      Serial.print("Recieved command: "); Serial.println(dataRcvd, HEX); Serial.println();
      switch(dataRcvd) {
        case 0x00:
          Serial.print("ledStatus: ");
          Serial.println("T");
          ledStatus = "true";
          ledStatusLcd = "T";
          break;
        case 0x01:
          Serial.print("ledStatus: ");
          Serial.println("F");
          ledStatus = "false";
          ledStatusLcd = "F";
          break;
        case 0x02:
          Serial.print("phun suong: ");
          Serial.println("T");
          phunSuong = "true";
          phunSuongLcd = "";
          break;
        case 0x03:
          Serial.print("phun suong: : ");
          Serial.println("T");
          phunSuong = "false";
          phunSuongLcd = "F";
          break;
        case 0x04:
          Serial.print("quat:");
          Serial.println("T");
          quatLcd = "T";
          quat = "true";
          break;
        case 0x05:
          Serial.print("quat:");
          Serial.println("F");
          quat = "false";
          quatLcd = "F";
          break;
        case 0x06:
          Serial.print("den:");
          Serial.println("T");
          den = "true";
          denLcd = "T";
          break;
        case 0x07:
          Serial.print("den:");
          Serial.println("F");
          den = "false";
          denLcd = "F";
          break;
        case 0x08:
          Serial.print("phun nuoc:");
          Serial.println("T");
          phunNuoc = "true";
          phunNuocLcd = "T";
          break;
        case 0x09:
          Serial.print("phun nuoc:");
          Serial.println("F");
          phunNuoc = "false";
          phunNuocLcd = "F";
          break;
        case 0x10:
          Serial.print("AUTO:");
          Serial.println("T");
          _auto = "true";
          _autoLcd = "T";
          break;
        case 0x11:
          Serial.print("AUTO:");
          Serial.println("F");
          _auto = "false";
          _autoLcd = "F";
          break;
        
    };

    if(_auto == "true") {
       auto_control(temperature, humidity,moisture, lightLux);  
    }
        
    // control relay  
    controlRelay(ledStatus, 2, "LED" );
    controlRelay(phunSuong, relayPin, "PHUN SUONG" );
    controlRelay(quat, relayQuat, "QUAT" );
    controlRelay(den, relayDen, "DEN" );
    controlRelay(phunNuoc, relayMayPhunNuoc, "PHUN_NUOC" );

   lcd.setCursor(0,2);              
  lcd.print("SNG: ");             
  lcd.print(phunSuongLcd);                                  

  lcd.setCursor(8,2);             
  lcd.print("NC: ");         
  lcd.print(phunNuocLcd);                                

  lcd.setCursor(14,2);              
  lcd.print("QT: ");             
  lcd.print(quatLcd);                    

  lcd.setCursor(0,3);             
  lcd.print("DN: ");         
  lcd.print(denLcd);      
  
    Serial.println("-------------------------------------------------------");
   };

   delay(1000);
};

// auto
void auto_control( float temperature,float humidity, float moisture,int lightLux ) {
   if (temperature > TEMPERATURE_THRESHOLD_MAX) {
        if (humidity < HUMIDITY_THRESHOLD_MAX) {
            phunSuong = "true";
            controlRelay(phunSuong, relayPin, "PHUN SUONG");
        }
        else {
            quat = "true";
            controlRelay(quat, relayQuat, "QUAT");
        }

        if (moisture < MOISTURE_THRESHOLD_MAX) {
            phunNuoc = "true";
            controlRelay(phunNuoc, relayMayPhunNuoc, "PHUN_NUOC");
        }
    }
    else if (temperature < TEMPERATURE_THRESHOLD_MIN) {
        den = "true";
        controlRelay(den, relayDen, "DEN" );
    }

    if (humidity < HUMIDITY_THRESHOLD_MIN) {
        phunSuong = "true";
        controlRelay(phunSuong, relayPin, "PHUN SUONG");
    }
    else if (humidity > HUMIDITY_THRESHOLD_MAX) {
        quat = "true";
        controlRelay(quat, relayQuat, "QUAT");
    }

    if (moisture < MOISTURE_THRESHOLD_MIN) {
        phunNuoc = "true";
        controlRelay(phunNuoc, relayMayPhunNuoc, "PHUN_NUOC");
    }
    if (lightLux < LIGHT_THRESHOLD_MIN_DAY) {
            den = "true";
            controlRelay(den, relayDen, "DEN");
        }
    else {  // Nếu là ban đêm
        if (lightLux < LIGHT_THRESHOLD_MIN_NIGHT) {
            den = "true";
            controlRelay(den, relayDen, "DEN");
        }
    }
    
}; 
