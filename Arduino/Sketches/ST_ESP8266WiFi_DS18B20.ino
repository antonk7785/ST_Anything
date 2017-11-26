//******************************************************************************************
//  File: ST_ESP8266WiFi_DS18B20.ino
//  Author: Anton K
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of one NodeMCU ESP8266 to 
//            implement a custom temperature mesurement device (Temperature from Dallas Semi 1-Wire DS18B20 device) for integration into SmartThings.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the NodeMCU ESP8266's WiFi.
//                                       _______
//                                      |       |----BLACK (GND)
//            DS18B20 Temerature Sensor |DS18B20|----YELLOW( D )
//                                      |_______|----RED   (3V )
//                    _____________________
//                   |Vin     |USB|     3V3|
//                   |GND               GND|
//                   |RST                TX|
//                   |EN                 RX|
//              RED--|3V3                D8|
//            BLACK--|GND                D7|
//                   |CLK                D6|
//                   |SD0                D5|
//                   |CMD               GND|
//                   |SD1               3V3|
//                   |SD2                D4|--YELLOW
//                   |SD3   _________    D3|
//                   |RSV  |   WiFi  |   D2|
//                   |RSV  | ESP8266 |   D1|
//                   |A0   |         |   D0|
//                    ---------------------
//******************************************************************************************
//******************************************************************************************
// SmartThings Library for ESP8266WiFi
//******************************************************************************************
#include <SmartThingsESP8266WiFi.h>
//******************************************************************************************
// ST_Anything Library 
//******************************************************************************************
#include <Constants.h>       //Constants.h is designed to be modified by the end user to adjust behavior of the ST_Anything library
#include <Device.h>          //Generic Device Class, inherited by Sensor and Executor classes
#include <Sensor.h>          //Generic Sensor Class, typically provides data to ST Cloud (e.g. Temperature, Motion, etc...)
#include <Executor.h>        //Generic Executor Class, typically receives data from ST Cloud (e.g. Switch)
#include <InterruptSensor.h> //Generic Interrupt "Sensor" Class, waits for change of state on digital input 
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications

#include <PS_TemperatureHumidity.h>  //Implements a Polling Sensor (PS) to measure Temperature and Humidity via DHT library
#include <PS_DS18B20_Temperature.h>  //Implements a Polling Sesnor (PS) to measure Temperature via DS18B20 libraries 

//*************************************************************************************************
//NodeMCU v1.0 ESP8266-12e Pin Definitions (makes it much easier as these match the board markings)
//*************************************************************************************************
#define LED_BUILTIN 16
#define BUILTIN_LED 16

//#define D0 16  //no internal pullup resistor
//#define D1  5
//#define D2  4
//#define D3  0  //must not be pulled low during power on/reset, toggles value during boot
  #define D4  2  //must not be pulled low during power on/reset, toggles value during boot
//#define D5 14
//#define D6 12
//#define D7 13
//#define D8 15  //must not be pulled high during power on/reset

//******************************************************************************************
//Define which Arduino Pins will be used for each device
//******************************************************************************************

#define PIN_TEMPERATURE_1 D4  //SmartThings Capabilty "Temperature Measurement" (Dallas Semiconductor DS18B20)


//******************************************************************************************
//ESP8266 WiFi Information
//******************************************************************************************
String str_ssid     = "ATT279";        // WiFi SSID
String str_password = "1506049138";    // WiFi Password
IPAddress ip(192, 168, 1, 227);        //Device IP Address. Must be static
IPAddress gateway(192, 168, 1, 254);   //Router gateway 
IPAddress subnet(255, 255, 255, 0);    //LAN subnet mask 
IPAddress dnsserver(8, 8, 8, 8);       //DNS server     
const unsigned int serverPort = 8090;  //Port to run the http server on

//******************************************************************************************
// Smartthings Hub Information
//******************************************************************************************
IPAddress hubIp(192, 168, 1, 84);      // SmartThings Hub ip. Must be static
const unsigned int hubPort = 39500;    // SmartThings Hub port

//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
  Serial.print(F("ST_Anything Callback: Sniffed data = "));
  Serial.println(msg);
  
  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
  //******************************************************************************************
  //Declare each Device that is attached to the Arduino
  //  Notes: - For each device, there is typically a corresponding "tile" defined in your 
  //           SmartThings Device Hanlder Groovy code, except when using new COMPOSITE Device Handler
  //         - For details on each device's constructor arguments below, please refer to the 
  //           corresponding header (.h) and program (.cpp) files.
  //         - The name assigned to each device (1st argument below) must match the Groovy
  //           Device Handler names.
  //         - The new Composite Device Handler is comprised of a Parent DH and various Child
  //           DH's.  The names used below MUST not be changed for the Automatic Creation of
  //           child devices to work properly.  Simply increment the number by +1 for each duplicate
  //           device (e.g. temperature1, temperature2, temperature3, etc...)  You can rename the Child Devices
  //           to match your specific use case in the ST Phone Application.
  //******************************************************************************************
  
  //Polling Sensors

  static st::PS_DS18B20_Temperature sensor1(F("temperature1"), 15, 0, PIN_TEMPERATURE_1, false, 10, 1); 
  
  st::Everything::debug=true;
  st::Executor::debug=true;
  st::Device::debug=true;
  st::PollingSensor::debug=true;
  st::InterruptSensor::debug=true;

  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************

  //Initialize the optional local callback routine (safe to comment out if not desired)
  //st::Everything::callOnMsgSend = callback;
  
  //Create the SmartThings ESP8266WiFi Communications Object
    //STATIC IP Assignment - Recommended
    st::Everything::SmartThing = new st::SmartThingsESP8266WiFi(str_ssid, str_password, ip, gateway, subnet, dnsserver, serverPort, hubIp, hubPort, st::receiveSmartString);
 
    //DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
    //st::Everything::SmartThing = new st::SmartThingsESP8266WiFi(str_ssid, str_password, serverPort, hubIp, hubPort, st::receiveSmartString);

  //Run the Everything class' init() routine which establishes WiFi communications with SmartThings Hub
  st::Everything::init();
  
  //*****************************************************************************
  //Add Temperature Sensor to the "Everything" Class
  //*****************************************************************************

  st::Everything::addSensor(&sensor1);
      
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();
  
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  st::Everything::run();
}
