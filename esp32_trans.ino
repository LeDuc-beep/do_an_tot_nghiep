#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <WiFi.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <BH1750.h>


unsigned long previousMillis = 0;
//unsigned long interval = 10000;
unsigned long interval = 7000;

// LoRa port configuration
const byte rxPin = 16;
const byte txPin = 17;
SoftwareSerial myLoRa(rxPin, txPin);

BH1750 lightMeter(0x23);

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

String ledStatus = "OFF";
String phunSuong = "OFF";
String quat = "OFF";
String den = "OFF";
String phunNuoc = "OFF";

void setup()
{
  Serial.begin(9600);
  while (!Serial) continue;

  // Set the baud rate for the LoRa module
  myLoRa.begin(9600);
  
  Wire.begin(SDA_PIN,SCL_PIN);
  lcd.begin();                      // Khởi tạo LCD
  lcd.backlight();                 // Bật đèn nền LCD
  
  // cảm biến ánh sáng
  // SDA/SCl 
  // cảm biến ánh sáng
  lightMeter.begin();
  lightMeter.configure(BH1750::CONTINUOUS_HIGH_RES_MODE);  

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
  lcd.print("SUONG:");             
  lcd.print("OFF");                                  

  lcd.setCursor(11,2);             
  lcd.print("LED: ");         
  lcd.print("OFF");                                

  lcd.setCursor(0,3);              
  lcd.print("QUAT: ");             
  lcd.print("OFF");                    

  lcd.setCursor(10,3);             
  lcd.print("DEN: ");         
  lcd.print("OFF");                     
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
  Serial.println(moistureReading);


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
//  Serial.print("Gia tri analog: ");
//  Serial.println(value);
  delay(200);
  moistureReading=0;
  result = percent;
  return result;
};

// control relay
void controlRelay(String field, int pin, String name) {
  if (field == "ON") {                         // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned ON");                 
    digitalWrite(pin, HIGH);                                                         // make output ON
  }

  else if (field == "OFF") {              // compare the input of led status received from firebase
    Serial.print(name);
    Serial.println(" turned OFF");
    digitalWrite(pin, LOW);                                                         // make output OFF
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
}

void loop() {
  String jsonString;
  

  // Get temperature and humidity values
//  float* tempHumidity = getTempHumidity();
//  float humidity = tempHumidity[0];
//  float temperature = tempHumidity[1];
//
//  int lightLux = readLight();
//  float moisture = readMoisture();
//
//  lcd.setCursor(0,0);              // Di chuyển con trỏ đến vị trí (0,0)
//  lcd.print("T:");             // In ra chữ "Temp: "
//  lcd.print(temperature);                    // In ra giá trị nhiệt độ đọc được
//  lcd.print(" C");                 // In ra độ đơn vị đo nhiệt độ  
//  
//  lcd.setCursor(10,0);              // Di chuyển con trỏ đến vị trí (0,1)
//  lcd.print("H:");         // In ra chữ "Humidity: "
//  lcd.print(humidity);                    // In ra giá trị độ ẩm đọc được
//  lcd.print(" %");                 // In ra đơn vị đo độ ẩm
// 
//  lcd.setCursor(0,1);              // Di chuyển con trỏ đến vị trí (0,0)
//  lcd.print("M:");             // In ra chữ "Temp: "
//  lcd.print(moisture);                    // In ra giá trị nhiệt độ đọc được
//  lcd.print(" %");                 // In ra độ đơn vị đo nhiệt độ
//    
//  lcd.setCursor(9,1);              // Di chuyển con trỏ đến vị trí (0,1)
//  lcd.print("L:");         // In ra chữ "Humidity: "
//  lcd.print(lightLux);                    // In ra giá trị độ ẩm đọc được
//  lcd.print(" lux");    

  // send lora 
  unsigned long currentMillis = millis();
  if((currentMillis - previousMillis > interval)) {
    float* tempHumidity = getTempHumidity();
  float humidity = tempHumidity[0];
  float temperature = tempHumidity[1];

  int lightLux = readLight();
  float moisture = readMoisture();

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
  //     jsonString = myLoRa.readStringUntil('\n'); // Đọc chuỗi JSON từ LoRa
  //    // Phân tích cú pháp chuỗi JSON và trích xuất dữ liệu
  //      DeserializationError error = deserializeJson(jsonDoc, jsonString);
  //      if (error) {
  //        Serial.println("Parsing failed");
  //        return;
  //       }
  //  
  //      // Truy cập và xử lý dữ liệu từ chuỗi JSON
  //       int checkLedStatus = jsonDoc["ledStatus"];
  //       int checkPhunSuong = jsonDoc["phunSuong"];
  //       int checkQuat = jsonDoc["quat"];
  //       int checkDen = jsonDoc["den"];
  //       int checkPhunNuoc = jsonDoc["phunNuoc"];
  //
  //       if(checkLedStatus == 1) {
  //          ledStatus = "ON";
  //      };
  //
  //       if(checkPhunSuong == 1) {
  //          phunSuong = "ON";
  //      };
  //
  //       if(checkQuat == 1) {
  //          quat = "ON";
  //      };
  //
  //       if(checkDen == 1) {
  //          den = "ON";
  //      };
  //
  //       if(checkLedStatus == 1) {
  //          phunNuoc = "ON";
  //      };
      byte dataRcvd = myLoRa.read();
      Serial.print("Recieved command: "); Serial.println(dataRcvd, HEX); Serial.println();
      switch(dataRcvd) {
        case 0x00:
          Serial.print("ledStatus: ");
          Serial.println("ON");
          ledStatus = "ON";
          break;
        case 0x01:
          Serial.print("ledStatus: ");
          Serial.println("OFF");
          ledStatus = "OFF";
          break;
        case 0x02:
          Serial.print("phun suong: ");
          Serial.println("ON");
          phunSuong = "ON";
          break;
        case 0x03:
          Serial.print("phun suong: : ");
          Serial.println("OFF");
          phunSuong = "OFF";
          break;
        case 0x04:
          Serial.print("quat:");
          Serial.println("ON");
          quat = "ON";
          break;
        case 0x05:
          Serial.print("quat:");
          Serial.println("OFF");
          quat = "OFF";
          break;
        case 0x06:
          Serial.print("den:");
          Serial.println("ON");
          den = "ON";
          break;
        case 0x07:
          Serial.print("den:");
          Serial.println("OFF");
          den = "OFF";
          break;
        case 0x08:
          Serial.print("phun nuoc:");
          Serial.println("ON");
          phunNuoc = "ON";
          break;
        case 0x09:
          Serial.print("phun nuoc:");
          Serial.println("OFF");
          phunNuoc = "OFF";
          break;
  
        // In ra Serial Monitor
       
  //      Serial.print("phunSuong: ");
  //      Serial.println(phunSuong);
  //      Serial.print("quat: ");
  //      Serial.println(quat);
  //      Serial.print("den: ");
  //      Serial.println(den);
  //      Serial.print("phunNuoc: ");
  //      Serial.println(phunNuoc);
    
    };
        
    // control relay  
//    controlRelay(ledStatus, 2, "LED" );
//    controlRelay(phunSuong, relayPin, "PHUN SUONG" );
//    controlRelay(quat, relayQuat, "QUAT" );
//    controlRelay(den, relayDen, "DEN" );
//    controlRelay(phunNuoc, relayMayPhunNuoc, "PHUN_NUOC" );
  
    
  ////                     
    lcd.setCursor(0,2);              
    lcd.print("SUONG:");             
    lcd.print(phunSuong);                                  
  //
    lcd.setCursor(11,2);             
    lcd.print("LED: ");         
    lcd.print(ledStatus);                                
  //
    lcd.setCursor(0,3);              
    lcd.print("QUAT: ");             
    lcd.print(quat);                    
  //
    lcd.setCursor(10,3);             
    lcd.print("DEN: ");         
    lcd.print(den);        
  
    Serial.println("-------------------------------------------------------");
   };

   delay(1000);
}
