/*
make sure to configure gpsEnabled, screenEnabled, bmeEnabled, debugPrints for any device to work as you want
also, don't forget to change the "thisNodeId". Every id has to be unique, starting from 0.
  todo: impliment a hash based off of a Wifi has mac address, load mac to eeprom and identify that way. 
        Actual usage doesn't depend on wifi. 
*/
#include <Wire.h>
#include <SoftwareSerial.h>


//#define baseStation
//#define intermediaryNode
#define rocket

#define numNodes 3 // when scaling, change this to a hashtable of unique devie id's with each id's data

#ifdef baseStation
  #define screenEnabled
  #define thisNodeId 0
  #define lora_rx 18
  #define lora_tx 19
  #define gps_serial_rx 16
  #define gps_serial_tx 17
  #define readBmeInterval 5000
  #define debugPrints
  //#define calcRatio //potential to crash program if divide by 0

#endif 

#ifdef intermediaryNode
  #define thisNodeId 1
  #define lora_rx 18
  #define lora_tx 19
  #define readBmeInterval 5000
  #define debugPrints
  #define checkin_interval 10000
#endif

#ifdef rocket
  #define screenEnabled//won't be on real launch
  #define bmeEnabled
  #define gpsEnabled

  #define thisNodeId 2
  #define lora_rx 18
  #define lora_tx 19
  #define gps_serial_rx 16
  #define gps_serial_tx 17
  #define readBmeInterval 500
  #define debugPrints
  #define checkin_interval 7000
#endif
  
#ifdef screenEnabled
  #include "SSD1306Wire.h"   
#endif


#ifdef calcRatio //losses to recieved messages
  int msgCounter = 0;
  double ratio;
#endif

#define START_UP_WAKE_TIME 1000
#define MAX_MESSAGE_ID_LENGTH 1000

#ifdef bmeEnabled
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; 
#endif


SoftwareSerial lora(lora_rx,lora_tx);

#ifdef gpsEnabled
#include <TinyGPSPlus.h>
SoftwareSerial gps_serial(gps_serial_rx,gps_serial_tx);
TinyGPSPlus gps;
#endif

#ifdef screenEnabled
SSD1306Wire display(0x3c, SDA, SCL);//default sda is 21 scl 22
#endif

struct message_struct{
  int devId = 0;
  int msgId = 0;

  double longitude = 999;
  double latitude = 999;
  int gps_altitude = 999; //in feet

  double temperature = 999;// celcius
  double pressure = 999; // hectoPascals
  double humidity = 999;//percent water concentration
  int bme_altitude = 999;// in meters

  int rssi = 999;
  int snr = 999;
};

message_struct allNodeData[numNodes];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lora.begin(9600);
  #ifdef gpsEnabled
  gps_serial.begin(9600);
  #endif

  #ifdef screenEnabled
  display.init();
  Serial.println("begun screen");
  #endif

  #ifdef bmeEnabled
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  #endif

  for(int i = 0; i < numNodes;i++){
    allNodeData[i].devId = i;
  }
  
  lora.readString();
  #ifdef screenEnabled
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  #endif

  Serial.println("hello and welcome to the world");
}

#ifdef bmeEnabled
int readBmeTime = 0;
#endif

#ifdef rocket
  int checkinTime = 0;
#endif 
#ifdef intermediaryNode
  int checkinTime = 0;
#endif

#ifdef debugPrints
int lastDebugPrint = 0;
#endif

#ifdef screenEnabled
int lastScreenPrint = 0;
#endif

void loop() {
  
  if(lora.available()) {
    handleLoraRecieve();
  }
  #ifdef gpsEnabled
  if(gps_serial.available()){
    handleGpsRecieve();
  }
  #endif

  #ifdef bmeEnabled
  if(millis() - readBmeTime >= readBmeInterval){
    readBmeTime = millis();
    updateBmeData();
  }
  #endif
  

  #ifdef rocket
    if(millis() - checkinTime > checkin_interval){
      handleSend(0,thisNodeId);
      checkinTime = millis();
    }
  #endif 
  #ifdef intermediaryNode
    if(millis() - checkinTime > checkin_interval){
      handleSend(0,thisNodeId);
      checkinTime = millis();
    }
  #endif 

  #ifdef screenEnabled
    /*if(millis() - lastScreenPrint > 5000){//only update display on recieve
      printDisplay();
      lastScreenPrint = millis();
    }*/
  #endif
  #ifdef debugPrints
    if(millis() - lastDebugPrint > 5000){
      printAllData();
      lastDebugPrint = millis();
    }
  #endif

}

#ifdef bmeEnabled
void updateBmeData(){
  allNodeData[thisNodeId].temperature = bme.readTemperature();
  allNodeData[thisNodeId].pressure = bme.readPressure()/100.0F;
  allNodeData[thisNodeId].humidity = bme.readHumidity(); 
  allNodeData[thisNodeId].bme_altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
}
#endif
#ifdef gpsEnabled
void handleGpsRecieve(){
  while(gps_serial.available()){
    gps.encode(gps_serial.read());
  }
  allNodeData[thisNodeId].longitude = gps.location.lng();
  allNodeData[thisNodeId].latitude = gps.location.lat();
  allNodeData[thisNodeId].gps_altitude = gps.altitude.feet();
}
#endif

void handleLoraRecieve(){
  String rcvInfo = lora.readStringUntil('S');
  
  if(rcvInfo.indexOf("ERR") > 0 || rcvInfo.indexOf("OK") > 0){
    Serial.print("recieved unwanted feedback:  ");
    Serial.println(rcvInfo);
    lora.readString();
  }
  
  else{//good receive but useless data, don't waste time processing
    int devId = lora.readStringUntil('|').toInt();
    if(devId == thisNodeId){
      lora.readString();
    }
    else{//everything nominal
      int msgId = lora.readStringUntil('|').toInt();
      double longitude = lora.readStringUntil('|').toDouble();
      double latitude = lora.readStringUntil('|').toDouble();
      int gps_altitude = lora.readStringUntil('|').toInt();

      double temperature = lora.readStringUntil('|').toDouble();
      double pressure = lora.readStringUntil('|').toDouble();
      double humidity = lora.readStringUntil('|').toDouble();
      int bme_altitude = lora.readStringUntil('E').toInt();

      lora.readStringUntil(',');
      int rssi = lora.readStringUntil(',').toInt();
      int snr = lora.readString().toInt();
      if(lora.available()){
        lora.readString();
      }

      if(msgId > allNodeData[devId].msgId || allNodeData[devId].msgId - msgId >= MAX_MESSAGE_ID_LENGTH/2){//if newer message, rebroadcast and update self information. Otherwise, nothing happens. Note that the sign is > , not >=. If it is equal, a node could be triggered by the rebroadcast of it's own signal.
        #ifdef debugPrints
          printAllData();
        #endif
        #ifdef debugPrints
          Serial.println("***********\n");
          Serial.print("rcv info: ");Serial.println(rcvInfo);
          Serial.print("rcvId: ");Serial.println(devId);
          Serial.print(" msgId: "); Serial.println(msgId);
          Serial.print(" rssi: ");Serial.println(rssi);
          Serial.print(" snr: ");Serial.println(snr);
        #endif

        #ifdef calcRatio
          int numMessages;
          msgCounter++;
        
          for(int i = 0; i<numNodes;i++){
            numMessages += allNodeData[i].msgId;
          }
          if(msgId !=0){
            ratio = msgCounter/msgId;
          }else{ratio = 999;}
          Serial.print("recieved ratio: ");Serial.print(ratio);Serial.print(" of "); Serial.print(numMessages); Serial.print(" (");Serial.print(msgCounter);Serial.println(") recievd");
        #endif

        allNodeData[devId].msgId = msgId;

        allNodeData[devId].longitude =longitude;
        allNodeData[devId].latitude =latitude;
        allNodeData[devId].gps_altitude =gps_altitude;
        
        allNodeData[devId].temperature =temperature;
        allNodeData[devId].pressure =pressure;
        allNodeData[devId].humidity =humidity;
        allNodeData[devId].bme_altitude =bme_altitude;

        allNodeData[devId].rssi = rssi;
        allNodeData[devId].snr = snr;
        
        #ifdef screenEnabled
        printDisplay();
        #endif
        
        Serial.println("rebroadcasting this message");
        handleSend(0,devId);
      }
    }
  }  
}

#ifdef screenEnabled
bool displayToggle = false; //this shows every time display is updated, this changes
void printDisplay(){
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  for(int i = 0; i< numNodes;i++){
    display.drawString(0,i*15,String(allNodeData[i].devId) + " : " + String(allNodeData[i].msgId) + " : " + String(allNodeData[i].longitude,4) + " : " + String(allNodeData[i].rssi) + "/" + String(allNodeData[i].snr));
  }
  display.drawString(0,numNodes*15,"ID, mID,lng,rssi,snr");
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128,54,String(displayToggle));
  display.display();
  displayToggle = !displayToggle;
  lastScreenPrint = millis();
}
#endif

void handleSend(int addr,int nodeId){
  String text = 
    "S" + String(allNodeData[nodeId].devId) +
    "|" + String(allNodeData[nodeId].msgId) + 
    "|" + String(allNodeData[nodeId].longitude) + 
    "|" + String(allNodeData[nodeId].latitude) + 
    "|" + String(allNodeData[nodeId].gps_altitude) + 
    "|" + String(allNodeData[nodeId].temperature) + 
    "|" + String(allNodeData[nodeId].pressure) +
    "|" + String(allNodeData[nodeId].humidity) + 
    "|" + String(allNodeData[nodeId].bme_altitude) + 
    "E";
  
  String commandStr = "AT+SEND=" + String(addr) + "," + String(text.length()) + "," + text;
  lora.println(commandStr);
  #ifdef debugPrints
    Serial.print("sent: ");
    Serial.println(commandStr);
  #endif

  if(nodeId == thisNodeId){
   
    if(allNodeData[thisNodeId].msgId >= MAX_MESSAGE_ID_LENGTH){
      allNodeData[thisNodeId].msgId = 0;
    }
    allNodeData[thisNodeId].msgId+=1;
  }
}

#ifdef debugPrints
void printAllData(){
  for(int i = 0; i<numNodes; i++){
    Serial.println("__");
    if(i == thisNodeId){
      Serial.print("(self): ");
    }
    else{
      Serial.print("  devId: ");
    }
    Serial.println(allNodeData[i].devId);
    Serial.print("  MsgId: ");Serial.println(allNodeData[i].msgId);

    Serial.print("long ");Serial.println(allNodeData[i].longitude);
    Serial.print("lat ");Serial.println(allNodeData[i].latitude);
    Serial.print("gps_alt ");Serial.println(allNodeData[i].gps_altitude);

    Serial.print("bme_alt ");Serial.println(allNodeData[i].bme_altitude);
    Serial.print("temp ");Serial.println(allNodeData[i].temperature);
    Serial.print("pres ");Serial.println(allNodeData[i].pressure);
    Serial.print("hum ");Serial.println(allNodeData[i].humidity);

    Serial.print("  Rssi: ");Serial.println(allNodeData[i].rssi);
    Serial.print("  Snr: ");Serial.println(allNodeData[i].snr);
  }
  Serial.println("__");
  lastDebugPrint = millis();
}
#endif

