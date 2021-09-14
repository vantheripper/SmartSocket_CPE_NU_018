#include <BluetoothSerial.h>  //librarie สำหรับการใช้งานการสื่อสารbluetooth
#include <Preferences.h>      //librarie สำหรับการจัดการหน่วยความจำภายใน
#include <WiFi.h>             //librarie สำหรับการใช้งานการ WiFi
#include <IOXhop_FirebaseESP32.h> //librarie สำหรับการเชื่อมต่อฐานข้อมูล Firebase
#include "DHT.h"  //librarie สำหรับโมดูล วัดอุณหภูมิและความชื้น

#define FIREBASE_HOST "neon-rex-258413.firebaseio.com"
#define FIREBASE_AUTH "kf2FQi4mccHxvvJgtb9QoO6bGKZLEXtm0sQwbsJK"
#define socket_1_pin  19
#define socket_2_pin  21
#define socket_3_pin  22
#define socket_4_pin  4
#define SW_PIN 18
#define hold_SW_time 2000
#define DELAY_SEND_DATA 1000000 // หน่วงเวลาการส่งข้อมูลไปที่ firebase ( 1,000,000  = 1 sec )

String deviceID;
String appUUID;
#define LDR_pin 34  // LDR Analog input pin that the potentiometer is attached to

#define Fire_Pin  35 //ประกาศตัวแปร ให้ Fire_Pin แทนขา analog ขาที่35

#define DHTTYPE DHT11   // ประกาศรุ่นของเซนเซอร์
#define DHT_PIN 32 // ประกาศตัวแปร ให้ DHT_PIN แทนขา analog ขาที่32
#define BUZZER_PIN 33 // ประกาศตัวแปร ให้ BUZZER แทนขา analog ขาที่33

// ประกาศตัวแปรสำหรับไฟสถานะ
#define LED_statusPin_R 2 
#define LED_statusPin_G 25
#define LED_statusPin_B 26

float h_temp = 0;
float t_temp = 0;
int LDR_Value = 0;        // value read from the pot
int LDR_p_Value = 0;  


// timer parameters
volatile int interruptCounter;
int totalInterruptCounter;
 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//timer ISR function
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}

Preferences preferences;
DHT dht(DHT_PIN, DHTTYPE); //สร้าง Instant ในการเข้าถึงไลบรารี่ DHT ด้วยชื่อ dht
struct Button {
  const uint8_t PIN;
  uint32_t timer;
  bool pressed;
};
bool check5sec = true;
bool trigTimer = false;
bool BLstate = false;
bool recieve_ssid = false;
bool recieve_pass = false;
bool wifi_connected =false;

bool socket_1_delayTrick = false;
bool socket_2_delayTrick = false;
bool socket_3_delayTrick = false;
bool socket_4_delayTrick = false;
int s1_delay_duration = 0;
int s2_delay_duration = 0;
int s3_delay_duration = 0;
int s4_delay_duration = 0;
bool socket_1_delayCount = false;
bool socket_2_delayCount = false;
bool socket_3_delayCount = false;
bool socket_4_delayCount = false;
unsigned long s1_delay_check;
unsigned long s2_delay_check;
unsigned long s3_delay_check;
unsigned long s4_delay_check;
bool socket_1_output_delay = false;
bool socket_2_output_delay = false;
bool socket_3_output_delay = false;
bool socket_4_output_delay = false;

bool beep = false;
int beep_count = 0;

//ประกาศตัวแปรเกี่ยวกับการกำหนด condition 
String LDRCon = "";
String FireCon = "";
String HumidCon = "";
String TempCon = "";
int LDRConVal = 0;
int FireConVal = 0;
int HumidConVal = 0;
int TempConVal = 0;

bool start = true;

String incoming;
char ssid[64];
char pass[64];
String WiFiSSID;

BluetoothSerial ESP32BT;
Button button1 = {SW_PIN, 0, false};
bool setup_wifi() {
  delay(10);
  Serial.println();
  int wifiTimeOut = millis();
  WiFi.begin(ssid, pass);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED && millis() - wifiTimeOut <= 8000) {
    Serial.print(".");
    delay(100);
  }
  if( millis() - wifiTimeOut >= 8000)
  {
    Serial.print("connecton time out");
    return false;
  }
  else{
     wifi_connected = true;
     Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  return true;
  }}

  
  bool connect_old_wifi() {
  delay(10);
  Serial.println();

   
  char WiFiSSID[64];
  char WiFiPASS[64];
  int wifiTimeOut = millis();
  preferences.begin("SSID", false);
  String WiFiSSID_temp = preferences.getString("SSID", "/empty");
  preferences.end();
  preferences.begin("PASS", false);
  String WiFiPASS_temp = preferences.getString("PASS", "/empty");
  preferences.end();
  WiFiSSID_temp.toCharArray(WiFiSSID,sizeof(WiFiSSID));
  WiFiPASS_temp.toCharArray(WiFiPASS,sizeof(WiFiPASS));
  Serial.println("reconnect"+ String(WiFiSSID_temp));
  Serial.println("pass"+ String(WiFiPASS_temp));
  WiFi.begin(WiFiSSID, WiFiPASS);
  Serial.print("reconnect");
  while (WiFi.status() != WL_CONNECTED && millis() - wifiTimeOut <= 8000) {
    Serial.print(".");
    delay(100);
  }
  if( millis() - wifiTimeOut >= 8000)
  {
    Serial.print("connecton time out");
    return false;
  }
  else{
     wifi_connected = true;
     Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  return true;
  }
 
  }
void checkSwHold()
{
   button1.pressed = digitalRead(button1.PIN);
      
      if (!button1.pressed ) // check sw pressed
       {
       if(!trigTimer) // start timer
       {
        button1.timer = millis();
        trigTimer = true;
        
       }
      if(millis() - button1.timer  >= hold_SW_time && check5sec) // check holding switch time
      {
        check5sec = false;
        Serial.println("Done");
      }
       }             
}

void LED_status(int pin)
{
  if(LED_statusPin_R == pin)
  {
    digitalWrite(LED_statusPin_G,LOW);
    digitalWrite(LED_statusPin_B,LOW);
    delay(20);
    digitalWrite(LED_statusPin_R,HIGH);
  }
  else if(LED_statusPin_G == pin)
  {
    digitalWrite(LED_statusPin_R,LOW);
    digitalWrite(LED_statusPin_B,LOW);
    delay(20);
    digitalWrite(LED_statusPin_G,HIGH);
  }
  else if(LED_statusPin_B == pin)
  {
    digitalWrite(LED_statusPin_R,LOW);
    digitalWrite(LED_statusPin_G,LOW);
    delay(20);
    digitalWrite(LED_statusPin_B,HIGH);
  }
  
}

void setup() {
  
  pinMode(socket_1_pin, OUTPUT);
  pinMode(socket_2_pin, OUTPUT);
  pinMode(socket_3_pin, OUTPUT);
  pinMode(socket_4_pin, OUTPUT);
  
  pinMode(LED_statusPin_R, OUTPUT);
  pinMode(LED_statusPin_G, OUTPUT);
  pinMode(LED_statusPin_B, OUTPUT);
  LED_status(LED_statusPin_R);
  pinMode(BUZZER_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,HIGH);
  pinMode(23,INPUT_PULLUP);
  Serial.begin(115200);
  pinMode(button1.PIN, INPUT_PULLUP);
  //pinMode(Fire_Pin,INPUT);
    preferences.begin("SSID", false);
   WiFiSSID = preferences.getString("SSID", "/empty");
   Serial.println(WiFiSSID);
   //configTime(TIMEZONE * 3600, 0,"time.uni.net.th","ntp.ku.ac.th","time1.nimt.or.th");
  if(WiFiSSID != "/empty")
  {
    preferences.end();
    bool check_wifi_start = connect_old_wifi();   
    start_steam();
  }
  
  
  
  
}


void loop() {
  if (interruptCounter > 0) { // ส่งข้อมูลเซนเซอร์ไปที่ firebase ทุก ๆเวลาที่กำหนด
     
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    
    totalInterruptCounter++;
   //Serial.println(totalInterruptCounter);
    if(WiFi.status() != WL_CONNECTED && !BLstate )
      {
        Serial.println("reconnect1");
        
        if(WiFiSSID != "/empty"){
          Serial.println("reconnect2");
          
          LED_status(LED_statusPin_R);
          WiFi.reconnect();
         // bool check_wifi_start = connect_old_wifi();  
          while(WiFi.status() != WL_CONNECTED )
          {
            Serial.print(".");
            delay(100);
          }
          LED_status(LED_statusPin_G);
        }
        
      }
    JsonObject& objectList = StaticJsonBuffer<200>().createObject();

    //read LDR sensor 
    LDR_Value = analogRead(LDR_pin);
    LDR_p_Value = map(LDR_Value, 0, 4095, 100, 0);
    //Serial.println(LDR_Value);

    //read Gas sensor
    int Fire_val = 0;
    int Fire_data=0;
    
    //Fire_data = digitalRead(Fire_Pin); //อ่านค่าสัญญาณ analog 
    Fire_data = analogRead(Fire_Pin); //อ่านค่าสัญญาณ analog 
    Fire_val = map(Fire_data,0,4095,100,0);
    //Serial.println(Fire_data);

    //read DHT sensor
    float h = dht.readHumidity(); 
    float t = dht.readTemperature();  // Read temperature as Celsius (the default)
    
    
    if (isnan(h) ){h = h_temp;Serial.println("failed to read");} else{ h_temp = h;}
    if (isnan(t) ){t = t_temp;} else{ t_temp = t;}
    /*Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");*/
    
    objectList["tempurature"] = t;
    objectList["LDR"] = LDR_p_Value;
    objectList["Fire"] = Fire_val;
    objectList["Humidity"] = h;
   
    if(beep)
    {
      
      if( beep_count % 2 == 0){
        Serial.println("Beep!");
        digitalWrite(BUZZER_PIN,LOW);
      }
      else{
        digitalWrite(BUZZER_PIN,HIGH);
      }
      beep_count++;
      if(beep_count >5)
      {
        beep_count = 0;
        beep = false;
      }
    }
      
    if(LDRCon[0] == '1')
    {
      
      if(LDRCon[1] == '0')// น้อยกว่า
      {
        
        if( LDR_p_Value < LDRConVal)
        {
          if(LDRCon[2] == '0' && LDRCon[3] == '0')
          {
            //int output = LDRCon[5]
            Firebase.setInt(deviceID + String("/socket_1/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 1 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '0' && LDRCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_2/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 2 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '1' && LDRCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_3/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 3 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '1' && LDRCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_4/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 4 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
        }
      }
      if(LDRCon[1] == '1')// มากกว่าเท่ากับ
      {
        if( LDR_p_Value > LDRConVal)
        {
          if(LDRCon[2] == '0' && LDRCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_1/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 1 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '0' && LDRCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_2/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 2 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '1' && LDRCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_3/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 3 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
             LDRCon = "0";
          }
          if(LDRCon[2] == '1' && LDRCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_4/status"),String(LDRCon[5]).toInt());
            Serial.println("Socket 4 LDR Condition  Sucsess!!");
            if(LDRCon[4] == '1'){
              beep = true;
            }
            LDRCon = "0";
          }
        }
      }
    }

    if(FireCon[0] == '1')
    {
     
      if(FireCon[1] == '0')// น้อยกว่า
      {
         
        if( Fire_val < FireConVal)
        {
          if(FireCon[2] == '0' && FireCon[3] == '0')
          {
            //int output = LDRCon[5]
            Firebase.setInt(deviceID + String("/socket_1/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 1 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '0' && FireCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_2/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 2 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '1' && FireCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_3/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 3 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '1' && FireCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_4/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 4 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
        }
      }
      if(FireCon[1] == '1')// มากกว่าเท่ากับ
      {
        if( Fire_val > FireConVal)
        {
          if(FireCon[2] == '0' && FireCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_1/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 1 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '0' && FireCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_2/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 2 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '1' && FireCon[3] == '0')
          {
            Firebase.setInt(deviceID + String("/socket_3/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 3 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
          if(FireCon[2] == '1' && FireCon[3] == '1')
          {
            Firebase.setInt(deviceID + String("/socket_4/status"),String(FireCon[5]).toInt());
            Serial.println("Socket 4 Fire Condition  Sucsess!!");
            if(FireCon[4] == '1'){
              beep = true;
            }
            FireCon = "0";
          }
        }
      }
    }

    if(HumidCon[0] == '1')
    {
       
      if(HumidCon[1] == '0')// น้อยกว่า
      {
         
        if( h < HumidConVal)
        {
          if(HumidCon[2] == '0' && HumidCon[3] == '0')
          {
            
            Firebase.setInt(deviceID + String("/socket_1/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 1 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
             HumidCon = "0";
          }
          if(HumidCon[2] == '0' && HumidCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_2/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 2 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
             HumidCon = "0";
          }
          if(HumidCon[2] == '1' && HumidCon[3] == '0')
          {
            
            Firebase.setInt(deviceID + String("/socket_3/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 3 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
             HumidCon = "0";
          }
          if(HumidCon[2] == '1' && HumidCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_4/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 4 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
             HumidCon = "0";
          }
        }
      }
      if(HumidCon[1] == '1')// มากกว่าเท่ากับ
      {
        if( h > HumidConVal)
        {
          if(HumidCon[2] == '0' && HumidCon[3] == '0')
          {
             
            Firebase.setInt(deviceID + String("/socket_1/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 1 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
            HumidCon = "0";
          }
          if(HumidCon[2] == '0' && HumidCon[3] == '1')
          {
             
            Firebase.setInt(deviceID + String("/socket_2/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 2 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
            HumidCon = "0";
          }
          if(HumidCon[2] == '1' && HumidCon[3] == '0')
          {
             
            Firebase.setInt(deviceID + String("/socket_3/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 3 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
            HumidCon = "0";
          }
          if(HumidCon[2] == '1' && HumidCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_4/status"),String(HumidCon[5]).toInt());
            Serial.println("Socket 4 Humid Condition  Sucsess!!");
            if(HumidCon[4] == '1'){
              beep = true;
            }
            HumidCon = "0";
          }
        }
      }
    }
    if(TempCon[0] == '1')
    {
      
      if(TempCon[1] == '0')// น้อยกว่า
      {
        
        if( t < TempConVal)
        {
          if(LDRCon[2] == '0' && LDRCon[3] == '0')
          {
            
           
            Firebase.setInt(deviceID + String("/socket_1/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 1 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '0' && TempCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_2/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 2 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '1' && TempCon[3] == '0')
          {
            
            Firebase.setInt(deviceID + String("/socket_3/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 3 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '1' && TempCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_4/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 4 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
        }
      }
      if(TempCon[1] == '1')// มากกว่าเท่ากับ
      {
        if( t > TempConVal)
        {
          if(TempCon[2] == '0' && TempCon[3] == '0')
          {
            
            Firebase.setInt(deviceID + String("/socket_1/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 1 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '0' && TempCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_2/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 2 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '1' && TempCon[3] == '0')
          {
            
            Firebase.setInt(deviceID + String("/socket_3/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 3 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
          if(TempCon[2] == '1' && TempCon[3] == '1')
          {
            
            Firebase.setInt(deviceID + String("/socket_4/status"),String(TempCon[5]).toInt());
            Serial.println("Socket 4 Temperature Condition  Sucsess!!");
            if(TempCon[4] == '1'){
              beep = true;
            }
            TempCon = "0";
          }
        }
      }
    }
    Firebase.set(deviceID+ String("/Sensor"), objectList);
   
    //Serial.print("An interrupt as occurred. Total number: ");
    //Serial.println(totalInterruptCounter);
    if(socket_1_delayTrick)
    {
      
      //socket_1_delayTrick = false;
      //Firebase.setInt(deviceID+ String("/socket_1/delay/trick"),0);
       
      
      socket_1_delayCount = true;
    }
    else if(socket_1_delayTrick == 0)
    {
      //Serial.println("Socket 1 delay Cancel!!");
      socket_1_delayCount = false;
    }
    if(socket_1_delayCount)
    {
      // waiting for count down
      if(millis() - s1_delay_check >= s1_delay_duration * 60000)
      {
        socket_1_delayTrick = false;
        Firebase.setInt(deviceID+ String("/socket_1/delay/trick"),0);
        Serial.println(millis());
        Serial.println(s1_delay_check);
        Serial.println(s1_delay_duration*60000);
        Firebase.setInt(deviceID + String("/socket_1/status"),socket_1_output_delay);
        Serial.println("Socket 1 delay Sucsess!!");
        socket_1_delayCount = false;
      }
    }

    
    if(socket_2_delayTrick)
    {
    
      socket_2_delayCount = true;
    }
    else if(socket_2_delayTrick == 0)
    {
      socket_2_delayCount = false;
    }
    if(socket_2_delayCount)
    {
      // waiting for count down
      if(millis() - s2_delay_check >= s2_delay_duration * 60000)
      {
        socket_2_delayTrick = false;
        Firebase.setInt(deviceID+ String("/socket_2/delay/trick"),0);
        Serial.println(millis());
        Serial.println(s2_delay_check);
        Serial.println(s2_delay_duration*60000);
        Firebase.setInt(deviceID + String("/socket_2/status"),socket_2_output_delay);
        Serial.println("Socket 2 delay Sucsess!!");
        socket_2_delayCount = false;
      }
    }
     if(socket_3_delayTrick)
    {
    
      socket_3_delayCount = true;
    }
    else if(socket_3_delayTrick == 0)
    {
      socket_3_delayCount = false;
    }
    if(socket_3_delayCount)
    {
      // waiting for count down
      if(millis() - s3_delay_check >= s3_delay_duration * 60000)
      {
        socket_3_delayTrick = false;
        Firebase.setInt(deviceID+ String("/socket_3/delay/trick"),0);
        Serial.println(millis());
        Serial.println(s3_delay_check);
        Serial.println(s3_delay_duration*60000);
        Firebase.setInt(deviceID + String("/socket_3/status"),socket_3_output_delay);
        Serial.println("Socket 3 delay Sucsess!!");
        socket_3_delayCount = false;
      }
    }
     if(socket_4_delayTrick)
    {
    
      socket_4_delayCount = true;
    }
    else if(socket_4_delayTrick == 0)
    {
      socket_4_delayCount = false;
    }
    if(socket_4_delayCount)
    {
      // waiting for count down
      if(millis() - s4_delay_check >= s4_delay_duration * 60000)
      {
        socket_4_delayTrick = false;
        Firebase.setInt(deviceID+ String("/socket_4/delay/trick"),0);
        Serial.println(millis());
        Serial.println(s4_delay_check);
        Serial.println(s4_delay_duration*60000);
        Firebase.setInt(deviceID + String("/socket_4/status"),socket_4_output_delay);
        Serial.println("Socket 4 delay Sucsess!!");
        socket_4_delayCount = false;
      }
    }
    
  }
  
  
    checkSwHold();
    if(!check5sec && !button1.pressed)
    {
      do{return;}
      while(!button1.pressed);
    }
     
    if(check5sec == false)
      {
      check5sec = true;
      preferences.begin("SSID", false);
      preferences.remove("SSID");
      
      preferences.begin("PASS", false);
      preferences.remove("PASS");
      preferences.end();
      WiFi.disconnect();
      wifi_connected = false;
      Firebase.stopStream();
      Serial.println("Try to connect...");
      
      BLstate = true;
      trigTimer = false;
      }
     if(BLstate)
     {
      ESP32BT.begin("Smart_Socket");   // เริ่มการทำงานของบลูทูธ ให้ชื่อ ESP32_MyLED
      // แสดงข้อความพร้อมสำหรับการจับคู่บลูทูธ
      LED_status(LED_statusPin_B);
      Serial.println("Bluetooth device is ready to pair");
       BLstate = false;
     }
     if(ESP32BT.available()){
      WiFi.disconnect();
      incoming = ESP32BT.readStringUntil(13);    // อ่านค่าว่าค่าที่เข้ามาคืออะไร
      incoming = incoming.substring(0,incoming.length());
      
       
        Serial.print(incoming);
        Serial.println();
       int count = 1;
       int split_pass = incoming.indexOf("/",count);
       int split_uuid = incoming.indexOf("/",count);
       while(incoming.substring(split_pass,split_pass+5) != "/pass")
       {
        count++;
        split_pass = incoming.indexOf("/",count);
       }
        //Serial.println("pass" + String(split_pass));
        
        
       while(incoming.substring(split_uuid,split_uuid+5) != "/uuid")
       {
        count++;
        split_uuid = incoming.indexOf("/",count);
       }
       //Serial.println("uuid" + String(split_pass));
       String incoming_ssid = incoming.substring(0,split_pass);
       String incoming_pass = incoming.substring(split_pass,split_uuid);
       String incoming_uuid = incoming.substring(split_uuid,-1);
       Serial.print(incoming_ssid);
        Serial.println();
        Serial.print(incoming_pass);
        Serial.println();
        Serial.println(incoming_uuid);
       if(incoming_ssid.substring(0,5) == "/ssid"){
        String ssid_temp = incoming_ssid.substring(5,-1);
        ssid_temp.toCharArray(ssid,sizeof(ssid));
       recieve_ssid = true;
       preferences.begin("SSID", false);
        preferences.putString("SSID", ssid_temp);
        preferences.end();
       Serial.print("ssid is "+ String(ssid));
       Serial.println();
      }
        if(incoming_pass.substring(0,5) == "/pass"){
        String pass_temp = incoming_pass.substring(5,-1);
        pass_temp.toCharArray(pass,sizeof(pass));
        recieve_pass = true;
        preferences.begin("PASS", false);
        preferences.putString("PASS", pass_temp);
        preferences.end();
        preferences.begin("UUID", false);
        preferences.putString("UUID", incoming_uuid.substring(5,-1));
        preferences.end();
       Serial.print("password is "+ String(pass));
       Serial.println();
       
      }
     
      }
     if(recieve_ssid && recieve_pass)
     {
      //Serial.println("password is "+ String(pass));
        
    
      delay(100);
     
      recieve_ssid = false;
      recieve_pass = false;
     bool check_connect =  setup_wifi();
     
     if (check_connect)
     {
      
      ESP32BT.println("device1");
      ESP32BT.disconnect();
      ESP32BT.end();
      start_steam();
     }
     else{
      ESP32BT.println("time_out");
     }
     
     }
  
}
void start_steam(){
  if(wifi_connected)
  {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  /* Start a namespace "iotsharing"
  in Read-Write mode: set second parameter to false 
  Note: Namespace name is limited to 15 chars */
  preferences.begin("Device", false);

 
  //preferences.clear();
  //preferences.remove("Device");
  //preferences.remove("SSID");
  //preferences.remove("PASS");
  deviceID = preferences.getString("Device", "/empty_serial*)");
  
  if(deviceID == "/empty_serial*)")
  {
    Serial.println("New_device");
    int temp = Firebase.getInt("Total_Device");
    temp++;
    Firebase.setInt("Total_Device",temp);
    JsonObject& initTimer = StaticJsonBuffer<100>().createObject();
    //initTimer["duration"] = 0;
    initTimer["output_status"] = 0;
    initTimer["trick"] = 0;
    deviceID = (String("/device") + temp);
    Firebase.setInt(deviceID + String("/socket_1/status"),0);
    Firebase.setInt(deviceID + String("/socket_2/status"),0);
    Firebase.setInt(deviceID + String("/socket_3/status"),0);
    Firebase.setInt(deviceID + String("/socket_4/status"),0);
    Firebase.set(deviceID + String("/socket_1/delay"),initTimer);
    Firebase.set(deviceID + String("/socket_2/delay"),initTimer);
    Firebase.set(deviceID + String("/socket_3/delay"),initTimer);
    Firebase.set(deviceID + String("/socket_4/delay"),initTimer);
    Firebase.setInt(deviceID + String("/socket_1/delay/duration/duration"),0);
    Firebase.setInt(deviceID + String("/socket_2/delay/duration/duration"),0);
    Firebase.setInt(deviceID + String("/socket_3/delay/duration/duration"),0);
    Firebase.setInt(deviceID + String("/socket_4/delay/duration/duration"),0);
     preferences.putString("Device", deviceID);
    preferences.end();  //-------------------------------------test
   
     ESP.restart();
  }
  
  preferences.end();
  preferences.begin("UUID", false);
  appUUID = preferences.getString("UUID", "/empty_serial*)");
  
  Firebase.set(appUUID + String("/all/")+deviceID + String("/device"),deviceID.substring(1,-1));
 /* if(Firebase.getInt(appUUID + String("/all/")+deviceID + String("/name")) == "")
  {
     Firebase.set(appUUID + String("/all/")+deviceID + String("/name"),"device");
  }*/
  
  preferences.end();
   timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, DELAY_SEND_DATA , true);
  timerAlarmEnable(timer);
  Serial.println(String(deviceID));
  /*JsonObject& alarmList = StaticJsonBuffer<200>().createObject();
  //String alarmList[20];
  
  for(int i = 0;i<2;i++)
  {
    JsonArray& data2 = alarmList.createNestedArray(String(i));
    data2.add(Firebase.get(String("device10/testalarm/" + i )));
  }*/

  
  /*if (!alarmList.success()) {
    Serial.println("parseObject() failed");
    return;
  }
   JsonObject& alarmList2 = Firebase.get("device10/alarm");
  if (!alarmList2.success()) {
    Serial.println("parseObject() failed");
    return;
  }*/
 
  //String out;
  //alarmList["1"].printTo(out); 
  
  //int a = alarmList["0800"]["enable"];
   //Serial.println("enable is " + String(a));
  // String output;
  
  
  
  //Serial.println("output is " + String(out));
  /* we have just reset ESP then increase reset_times */
  //Serial.printf("Number of restart times: %d\n", reset_times);
  Serial.println("Get data from database");
 
  digitalWrite(socket_1_pin, Firebase.getInt(String(deviceID)+"/socket_1/status"));
  digitalWrite(socket_2_pin, Firebase.getInt(String(deviceID)+"/socket_2/status"));
  digitalWrite(socket_3_pin, Firebase.getInt(String(deviceID)+"/socket_3/status"));
  digitalWrite(socket_4_pin, Firebase.getInt(String(deviceID)+"/socket_4/status"));

  s1_delay_duration = Firebase.getInt(String(deviceID)+"/socket_1/delay/duration/duration");
  s2_delay_duration = Firebase.getInt(String(deviceID)+"/socket_2/delay/duration/duration");
  s3_delay_duration = Firebase.getInt(String(deviceID)+"/socket_3/delay/duration/duration");
  s4_delay_duration = Firebase.getInt(String(deviceID)+"/socket_4/delay/duration/duration");

  socket_1_output_delay = Firebase.getInt(String(deviceID)+"/socket_1/delay/output_status");
  socket_2_output_delay = Firebase.getInt(String(deviceID)+"/socket_2/delay/output_status");
  socket_3_output_delay = Firebase.getInt(String(deviceID)+"/socket_3/delay/output_status");
  socket_4_output_delay = Firebase.getInt(String(deviceID)+"/socket_4/delay/output_status");
  
  Serial.println("Start stream led control");
  LED_status(LED_statusPin_G);
    if(start){
      start = false;
      Firebase.setInt(deviceID + String("/temp"),Firebase.getInt(String(deviceID)+"/temp") +1 );
    }
  Firebase.stream(String(deviceID), [](FirebaseStream stream) {
  
    Serial.println("Event: " + stream.getEvent());
    Serial.println("Path: " + stream.getPath());
    String data_firebase =stream.getDataString();
    Serial.println("DATA: " + data_firebase);
    //Serial.println("Data: " + json);
    if ( stream.getEvent() == "put") {     
       
      if (stream.getPath() == "/socket_1/status") {
        digitalWrite(socket_1_pin, stream.getDataInt());
        Serial.println("Socket 1 change");
      }
      if (stream.getPath() == "/socket_2/status") {
        digitalWrite(socket_2_pin, stream.getDataInt());
        Serial.println("Socket 2 change");
      }
      if (stream.getPath() == "/socket_3/status") {
        digitalWrite(socket_3_pin, stream.getDataInt());
        Serial.println("Socket 3 change");
      }
      if (stream.getPath() == "/socket_4/status") {
        digitalWrite(socket_4_pin, stream.getDataInt());
        Serial.println("Socket 4 change");
      }
      if (stream.getPath() == "/socket_1/delay/trick" && stream.getDataInt() == 1) {
        Serial.println("Socket 1 delay trick");   
        s1_delay_check = millis();
        socket_1_delayTrick = true;       
      }
      if(stream.getPath() == "/socket_1/delay/duration/duration" ) {
        s1_delay_duration = stream.getDataInt();
       Serial.println(s1_delay_duration);
      }
      if (stream.getPath() == "/socket_2/delay/trick" && stream.getDataInt() == 1) {
        Serial.println("Socket 2 delay trick");   
        s2_delay_check = millis();
        socket_2_delayTrick = true;         
      }
      if(stream.getPath() == "/socket_2/delay/duration/duration") {
        s2_delay_duration = stream.getDataInt();
       Serial.println(s2_delay_duration);
      }
      if (stream.getPath() == "/socket_3/delay/trick" && stream.getDataInt() == 1) {
        Serial.println("Socket 3 delay trick");   
        s3_delay_check = millis();
        socket_3_delayTrick = true;         
      }
      if(stream.getPath() == "/socket_3/delay/duration/duration") {
        s3_delay_duration = stream.getDataInt();
       Serial.println(s3_delay_duration);
      }
      if (stream.getPath() == "/socket_4/delay/trick" && stream.getDataInt() == 1) {
        Serial.println("Socket 4 delay trick");   
        s4_delay_check = millis();
        socket_4_delayTrick = true;         
      }
      if(stream.getPath() == "/socket_4/delay/duration/duration") {
        s4_delay_duration = stream.getDataInt();
       Serial.println(s4_delay_duration);
      }

      if(stream.getPath() == "/socket_1/delay/output_status") {
        socket_1_output_delay = stream.getDataInt();
       Serial.println(String("S1_output_delay :") + socket_1_output_delay);
      }
      if(stream.getPath() == "/socket_2/delay/output_status") {
        socket_2_output_delay = stream.getDataInt();
       Serial.println(String("S2_output_delay :") + socket_2_output_delay);
      }
      if(stream.getPath() == "/socket_3/delay/output_status") {
        socket_3_output_delay = stream.getDataInt();
       Serial.println(String("S3_output_delay :") + socket_3_output_delay);
      }
      if(stream.getPath() == "/socket_4/delay/output_status") {
        socket_4_output_delay = stream.getDataInt();
       Serial.println(String("S4_output_delay :") + socket_4_output_delay);
      }
   }
    if(stream.getEvent() == "patch" )
    {
      
      if(stream.getPath() == "/socket_1/delay") {
        if(data_firebase.indexOf("output_status") > 0)
        {
        String value  = data_firebase.substring(17,data_firebase.length()-1);
      
       Serial.println(value);
       socket_1_output_delay = value.toInt();
       Serial.println("output_status : " + String(socket_1_output_delay));
          
        }
        if(data_firebase.indexOf("trick") > 0){
          String value  = data_firebase.substring(9,data_firebase.length()-1);
      
        Serial.println(value);
        int trick_temp = value.toInt();
        if(trick_temp == 1)
        {
          Serial.println("Socket 1 delay trick");
          socket_1_delayTrick = true;
          s1_delay_check = millis();
        }
         
        }   
      }
      if(stream.getPath() == "/socket_2/delay") {
        if(data_firebase.indexOf("output_status") > 0)
        {
        String value  = data_firebase.substring(17,data_firebase.length()-1);
      
       Serial.println(value);
       socket_2_output_delay = value.toInt();
       Serial.println("output_status : " + String(socket_2_output_delay));
          
        }
        if(data_firebase.indexOf("trick") > 0){
          String value  = data_firebase.substring(9,data_firebase.length()-1);
      
        Serial.println(value);
        int trick_temp = value.toInt();
        if(trick_temp == 1)
        {
          Serial.println("Socket 2 delay trick");
          socket_2_delayTrick = true;
          s2_delay_check = millis();
        }
         
        }   
      }
      if(stream.getPath() == "/socket_3/delay") {
        if(data_firebase.indexOf("output_status") > 0)
        {
        String value  = data_firebase.substring(17,data_firebase.length()-1);
      
       Serial.println(value);
       socket_3_output_delay = value.toInt();
       Serial.println("output_status : " + String(socket_3_output_delay));
          
        }
        if(data_firebase.indexOf("trick") > 0){
          String value  = data_firebase.substring(9,data_firebase.length()-1);
      
        Serial.println(value);
        int trick_temp = value.toInt();
        if(trick_temp == 1)
        {
          Serial.println("Socket 3 delay trick");
          socket_3_delayTrick = true;
          s3_delay_check = millis();
        }
         
        }   
      }
      if(stream.getPath() == "/socket_4/delay") {
        if(data_firebase.indexOf("output_status") > 0)
        {
        String value  = data_firebase.substring(17,data_firebase.length()-1);
      
       Serial.println(value);
       socket_4_output_delay = value.toInt();
       Serial.println("output_status : " + String(socket_4_output_delay));
          
        }
        if(data_firebase.indexOf("trick") > 0){
          String value  = data_firebase.substring(9,data_firebase.length()-1);
      
        Serial.println(value);
        int trick_temp = value.toInt();
        if(trick_temp == 1)
        {
          Serial.println("Socket 4 delay trick");
          socket_4_delayTrick = true;
          s4_delay_check = millis();
        }
         
        }   
      }
     
      if(stream.getPath() == "/socket_1/delay/duration")
     {
      
      String value  = data_firebase.substring(12,data_firebase.length()-1);
      
       Serial.println(value);
       s1_delay_duration = value.toInt();
       Serial.println("delay : " + String(s1_delay_duration));
     }
     if(stream.getPath() == "/socket_2/delay/duration")
     {
      
      String value  = data_firebase.substring(12,data_firebase.length()-1);
      
       Serial.println(value);
       s2_delay_duration = value.toInt();
       Serial.println("delay : " + String(s2_delay_duration));
     }
     if(stream.getPath() == "/socket_3/delay/duration")
     {
      
      String value  = data_firebase.substring(12,data_firebase.length()-1);
      
       Serial.println(value);
       s3_delay_duration = value.toInt();
       Serial.println("delay : " + String(s3_delay_duration));
     }
     if(stream.getPath() == "/socket_4/delay/duration")
     {
      
      String value  = data_firebase.substring(12,data_firebase.length()-1);
      
       Serial.println(value);
       s4_delay_duration = value.toInt();
       Serial.println("delay : " + String(s4_delay_duration));
     }
     
 
      if(stream.getPath() == "/socket_1")
      {
        String value  = data_firebase.substring(10,11);
         Serial.println(value);
        if(value == "0")
        {
          digitalWrite(socket_1_pin, LOW);
        }
        else{
          digitalWrite(socket_1_pin, HIGH);
            }
        Serial.println("Socket 1 change");
      }
      if(stream.getPath() == "/socket_2")
      {
        String value  = data_firebase.substring(10,11);
         Serial.println(value);
        if(value == "0")
        {
          digitalWrite(socket_2_pin, LOW);
        }
        else{
          digitalWrite(socket_2_pin, HIGH);
            }
        Serial.println("Socket 2 change");
      }
      if(stream.getPath() == "/socket_3")
      {
        String value  = data_firebase.substring(10,11);
         Serial.println(value);
        if(value == "0")
        {
          digitalWrite(socket_3_pin, LOW);
        }
        else{
          digitalWrite(socket_3_pin, HIGH);
            }
        Serial.println("Socket 3 change");
      }
      if(stream.getPath() == "/socket_4")
      {
        String value  = data_firebase.substring(10,11);
         Serial.println(value);
        if(value == "0")
        {
          digitalWrite(socket_4_pin, LOW);
        }
        else{
          digitalWrite(socket_4_pin, HIGH);
            }
        Serial.println("Socket 4 change");
      }
      if(stream.getPath() == "/Conditons")
      {
        int firstq = data_firebase.indexOf("\"");
        int secondq = data_firebase.indexOf("\"" , firstq+1);
        int thirdq = data_firebase.indexOf("\"" , secondq+1);
        int fourthq = data_firebase.indexOf("\"" , thirdq+1);
        String value1  = data_firebase.substring(firstq+1,secondq);
        String value2  = data_firebase.substring(thirdq+1,fourthq);
        Serial.println(value1);
        Serial.println(value2);
      if(value1 == "FireCon")
      {
        FireCon = value2;
      }
      if(value1 == "FireConVal")
      {
        FireConVal = value2.toInt();
      }
      if(value1 == "HumidCon")
      {
        HumidCon = value2;
      }
      if(value1 == "HumidConVal")
      {
        HumidConVal = value2.toInt();
      }
      if(value1 == "LDRCon")
      {
        LDRCon = value2;
      }
      if(value1 == "LDRConVal")
      {
        LDRConVal = value2.toInt();
      }
      if(value1 == "TempCon")
      {
        TempCon = value2;
      }
      if(value1 == "TempConVal")
      {
        TempConVal = value2.toInt();
      } 
        
      }
      
    }
   /* if(Firebase.failed()){
      Firebase.stopStream();
     }*/
  });
  
  }
}
