# rocket_lora_mesh_gps_env
 Rocket telemetry and tracking system which communicates via lora to homestations, other model rockets, and intermediary nodes

 This current code works for ESPwroom32 dev board, but can be adapted to any board. It relies on I2C and SoftwareSerial to communicate with a REYAX Lora module, GoouuuTech GT -U7 gps module, and BME280.
 The code is set so that you just have to uncomment rocket,basestation, or intermediary node and the code will work. Make sure to create a new node id for each node on your network, starting from 0 up to numNodes (exclusive). I will change this to rely on mac address written to eeprom in the future.

 Configure the pins to your setup. The names should be explanitory.
 
 To set up the Lora Module:
    1) Make a new file and set Rx/Tx pins to input(or make a softwareserial).
    2) Connect Board Rx -> Lora Rx, and Board Tx -> Lora Tx. 
    3) Use the AT commands for the lora module (AT+ADDRESS=0  AT+NETWORKID = 0 AT+BAND=915000000 AT+PARAMETER=12,4,1,7 AT+IPR=9600). 
          - I used 9600 BAUD as at the higher 115200 which should be supported, I was loosing a lot of data. Still have to figure out why, but 9600 immediatly resolved all issues. 
  
Relies on Adafruit bme280, TinyGPSPlus, SoftwareSerial, Wire.
