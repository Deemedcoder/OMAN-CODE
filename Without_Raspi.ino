#include <Arduino.h>
#include <EEPROM.h>
#include <STM32FreeRTOS.h>
#include <HardwareSerial.h>
#include "tinySHT2x.h"    // Assuming you have this library for the sensor
#include <Wire.h>  
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SNMP.h>
#include <EthernetUDP.h>
#include <SPI.h>
#include <STM32RTC.h>

STM32RTC& rtc = STM32RTC::getInstance();
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 16;
const byte weekDay = 1;
const byte day = 15;
const byte month = 6;
const byte year = 15;






EthernetUDP udp;
SNMP::Manager snmp;

// Define the types we are dealing with
using SNMP::IntegerBER;
using SNMP::FloatBER;
using SNMP::NullBER;
using SNMP::VarBind;
using SNMP::VarBindList;



int SetHUMHIGH =0;
int SetTEMPHIGH =0;
int SetHUMLOW =0;
int SetTEMPLOW =0;
float TEMP_MIN;
float TEMP_MAX;
#define PWM_MIN 0 
#define PWM_MAX 255

#define PINLED          PC11       // Update with the GPIO pin you're usingl
#define NUM_LEDS       60         // Set the number of LEDs to 60
Adafruit_NeoPixel strip(NUM_LEDS, PINLED, NEO_GRB + NEO_KHZ800);  // GRB order for WS2812B

int OUT1 = PE10;
int OUT2 = PE11;
int OUT3 = PE12;
int OUT4 = PE13;
int OUT5 = PB1;
int Led1 = PD9;
int Led2 = PD10;
int Led3 = PD11; 
int ALARM = PE14;
int PWMTEMP;

int FIRE = PE7;

int IN1 = PE0;
int IN2 = PE1;
int IN3 = PE2;
int IN4 = PE3;
int IN5 = PE4;
float data1Value;  // Variable to store the value of data1
float data2Value;  // Variable to store the value of data2
int WLD = PB0;
int GlobalAlarm = 0;
tinySHT2x H_Sens;
HardwareSerial Serial2(PA3, PA2); // Communication with LCD via Serial2
HardwareSerial Serial1(PA10, PA9); // Logging communication
HardwareSerial Serial3(PB11,PB10);
// Define two tasks for LCD update & serial logging
void TaskLCDUpdate(void *pvParameters);
void TaskLCDInterface(void *pvParameters);


void initsensor(){


  Wire.begin();

}
// UPS CLASS 

class UPS {
public:
    // Structure for OID details (OID, type)
    struct OIDInfo {
        const char *oid;
        const char *type;  // 'integer' or 'float'
    };

    // Function to read values from SNMP device (Dynamic OIDs)
    SNMP::Message* read(const OIDInfo *oids, size_t oidsCount, const char* community) {
        SNMP::Message *message = new SNMP::Message(SNMP::Version::V2C, community, SNMP::Type::GetRequest);
        
        // Add all OIDs in the list to the request
        for (size_t i = 0; i < oidsCount; ++i) {
            if (strcmp(oids[i].type, "integer") == 0) {
                message->add(oids[i].oid, new NullBER); // We can modify this to IntegerBER if needed
            } else if (strcmp(oids[i].type, "float") == 0) {
                message->add(oids[i].oid, new NullBER); // We can modify this to FloatBER if needed
            }
        }
        return message;
    }

    // Function to parse the SNMP response and store values in variables
    bool parseMessage(const SNMP::Message *message, const OIDInfo *oids, size_t oidsCount) {
        unsigned int found = 0;
        VarBindList *varbindlist = message->getVarBindList();
        
        // Loop through variable bindings in the response
        for (unsigned int index = 0; index < varbindlist->count(); ++index) {
            VarBind *varbind = (*varbindlist)[index];
            const char *name = varbind->getName();
            
            for (size_t i = 0; i < oidsCount; ++i) {
                if (strcmp(oids[i].oid, name) == 0) {
                    if (strcmp(oids[i].type, "integer") == 0) {
                        uint64_t value = static_cast<IntegerBER*>(varbind->getValue())->getValue();
                        // Assign the value to the corresponding variable
                        if (i == 0) outputSource = value;
                        else if (i == 1) batteryStatus = value;
                        else if (i == 2) estimatedMinutesRemaining = value;
                        else if (i == 3) estimatedChargeRemaining = value;
                        
                        found++;
                    } else if (strcmp(oids[i].type, "float") == 0) {
                        float value = static_cast<FloatBER*>(varbind->getValue())->getValue();
                        // Assign the value to the corresponding variable
                        if (i == 0) outputSource = value;
                        else if (i == 1) batteryStatus = value;
                        else if (i == 2) estimatedMinutesRemaining = value;
                        else if (i == 3) estimatedChargeRemaining = value;
                        
                        found++;
                    }
                }
            }
        }
        return found > 0;
    }

    // Variables to store OID values
    uint64_t outputSource = 0;
    uint64_t batteryStatus = 0;
    uint64_t estimatedMinutesRemaining = 0;
    uint64_t estimatedChargeRemaining = 0;
};



UPS ups;



void onMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    // Define OID list and types
    UPS::OIDInfo oids[] = {
        {"1.3.6.1.4.1.318.1.4.9.2.1.0", "integer"},  // Example OID for Output Source (Integer)
        {"1.3.6.1.4.1.318.2.3.7.0", "integer"},      // Example OID for Battery Status (Integer)
        {"1.3.6.1.4.1.318.1.4.8.7.1.2.125", "integer"}, // Example OID for Estimated Minutes Remaining (Integer)
        {"1.3.6.1.4.1.318.1.4.8.7.1.2.120", "integer"}  // Example OID for Estimated Charge Remaining (Integer)
    };
    
    // Process the response and assign values to variables
    if (ups.parseMessage(message, oids, sizeof(oids) / sizeof(oids[0]))) {
        Serial1.println("SNMP Response Processed.");
    }

    // Print the values to the serial monitor
   
}




void sendSNMPRequestAndPrintValues() {
    // Define OID list and types
    UPS::OIDInfo oids[] = {
        {"1.3.6.1.4.1.318.1.4.9.2.1.0", "integer"},
        {"1.3.6.1.4.1.318.2.3.7.0", "integer"},
        {"1.3.6.1.4.1.318.1.4.8.7.1.2.125", "integer"},
        {"1.3.6.1.4.1.318.1.4.8.7.1.2.120", "integer"}
    };

    // Send SNMP request with community string and OIDs
    SNMP::Message *message = ups.read(oids, sizeof(oids) / sizeof(oids[0]), "apc-8932");
    snmp.send(message, IPAddress(192, 168, 1, 41), SNMP::Port::SNMP);
    delete message;

    // Print the fetched values
    
    
    data1Value = ups.estimatedMinutesRemaining;
    //data2Value = ups.estimatedChargeRemaining;

   
}









void updateLEDStrip(int temperature , int humidity) {


  

  int IN3_Status = digitalRead(IN3);
  

  int IN1_Status = digitalRead(IN1);
  int IN2_Status = digitalRead(IN2);
 // int IN3_Status = digitalRead(IN3);
  int IN4_Status = digitalRead(IN4);
  int  WLDValue = analogRead(WLD);
  int WLD_Status = (WLDValue > 300) ? 0 : 1;
  int FIRE_Status = digitalRead(FIRE);

  int SetTEMPHIGH = EEPROM.read(0);
  delay(100);
  int SetTEMPLOW = EEPROM.read(1);
  delay(100);
  int SetHUMHIGH = EEPROM.read(2);
  delay(100);
  int SetHUMLOW = EEPROM.read(3);
  delay(100);
  



  uint32_t color = strip.Color(0, 255, 0);  // Default color to green

  // Check for fire status first (highest priority)
  if (FIRE_Status == 1) {
    color = strip.Color(255, 0, 0);  // Red if fire detected
  }
  if (WLD_Status == 1) {
    color = strip.Color(255, 0, 0);  // Red if fire detected
  }


   if (IN1_Status == 0) {
    color = strip.Color(255, 165, 0);  // Orange if doorfront is open (status 1)
  } 
  else if (IN2_Status == 0) {
    color = strip.Color(255, 165, 0);  // Orange if door 2 is open (status 1)
  }
  // Then check for temperature condition (if no fire)
  else if (temperature <= SetTEMPLOW) {
    color = strip.Color(255, 255, 0);  // Yellow if temperature is below or equal to low point
  } else if (temperature >= SetTEMPHIGH) {
    color = strip.Color(255, 0, 0);  // Red if temperature is above or equal to high point
  }
  // Then check for humidity condition (if no fire or temperature condition met)
  else if (humidity <= SetHUMLOW) {
    color = strip.Color(255, 255, 0);  // Yellow if humidity is below or equal to low point
  } else if (humidity >= SetHUMHIGH) {
    color = strip.Color(255, 0, 0);  // Red if humidity is above or equal to high point
  }
  // If none of the conditions match, keep green
  else {
    color = strip.Color(0, 255, 0);  // Green as default
  }

  // Apply the color to all LEDs in the strip
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();  // Update the strip with the new color
}



void setup() {

  initsensor();
  int IN1 = PE0;
    int IN2 = PE1;
    int IN3 = PE2;
    int IN4 = PE3;
    int IN5 = PE4;
    int WLD = PB0;
    
    int FIRE = PE7;

  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  pinMode(IN3, INPUT);
  pinMode(IN4, INPUT);
  pinMode(IN5, INPUT);
  pinMode(WLD, INPUT);
  pinMode(FIRE, INPUT);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(OUT5, OUTPUT);
  pinMode(ALARM, OUTPUT);
  strip.begin();
 //Ethernet Settinggs 


IPAddress defaultIP(192, 168, 1, 103);
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  
  SPI.setMISO(PB4);
  SPI.setMOSI(PB5);
  SPI.setSCLK(PB3);
  SPI.begin();
  Ethernet.init(PB8);
  Ethernet.begin(mac, defaultIP);
  
  snmp.begin(udp);
  snmp.onMessage(onMessage);



// ethernet 


  // Initialize serial communication
  Serial2.begin(115200);  // Initialize Serial2 for LCD communication
  Serial1.begin(9600);    // Initialize Serial1 for logging
  Serial3.begin(9600);

  rtc.begin();


    if (rtc.getHours() == 0 && rtc.getMinutes() == 0 && rtc.getSeconds() == 0) {
    // RTC not set, set the initial time and date
    rtc.setHours(hours);
    rtc.setMinutes(minutes);
    rtc.setSeconds(seconds);
    rtc.setWeekDay(weekDay);
    rtc.setDay(day);
    rtc.setMonth(month);
    rtc.setYear(year);
    Serial1.println("RTC Initialized with default time!");
  } else {
    Serial1.println("RTC already running!");
  }

  Serial1.println("RTC Initialized!");


    if (EEPROM.read(0) == 255|| EEPROM.read(0) == 0) {
    // If blank, write 27 to the address
    EEPROM.write(0, 27);
    Serial1.println("EEPROM address was blank. For Temp HIGH Wrote 27 to it.");
  } else {
    // If not blank, read and print the value
    SetTEMPHIGH = EEPROM.read(0);
    TEMP_MAX = SetTEMPHIGH;
    Serial1.print("EEPROM address already had Temp HIGH value: ");
    Serial1.println(SetTEMPHIGH);
    
  }

  // Set default value for EEPROM address 1
  if (EEPROM.read(1) == 255 || EEPROM.read(1) == 0) {
    // If blank, write 18 to the address
    EEPROM.write(1, 18);
    Serial1.println("EEPROM address was blank. Temperature LOW  Wrote 18 to it.");
  } else {
    // If not blank, read and print the value
    SetTEMPLOW = EEPROM.read(1);
    TEMP_MIN = SetTEMPLOW;
    Serial1.print("EEPROM address already had Temperature LOW value: ");
    Serial1.println(SetTEMPLOW);
  }

  // Set default value for EEPROM address 2
  

  // Set default value for EEPROM address 3
  if (EEPROM.read(2) == 255 || EEPROM.read(2) == 0) {
    // If blank, write 65 to the address
    EEPROM.write(2, 65);
    Serial1.println("EEPROM address was blank. HUM HIGH Wrote 65 to it.");
  } else {
    // If not blank, read and print the value
    SetHUMHIGH = EEPROM.read(2);
    Serial1.print("EEPROM address already had Set HUM HIGH Value : ");
    Serial1.println(SetHUMHIGH);
  }


  if (EEPROM.read(3) == 255 || EEPROM.read(3) == 0) {
    // If blank, write 55 to the address
    EEPROM.write(3, 55);
    Serial1.println("EEPROM address was blank. HUM LOW Wrote 55 to it.");
  } else {
    // If not blank, read and print the value
    SetHUMLOW = EEPROM.read(3);
    Serial1.print("EEPROM address already had Set HUM LOW Value : ");
    Serial1.println(SetHUMLOW);
  }

 

  while (!Serial1) {
    ; // wait for serial port to connect.
  }

  // Parameters for the LCD task (initial random values)
  int params[] = {25, 65, 1, 1, 0, 1, 120.5, 250.3};


  // Create the LCD update task
  xTaskCreate(
    TaskLCDUpdate,               // Task function
    (const portCHAR *)"LCDUpdate", // Task name
    8192,                        // Stack size
    (void *)params,              // Parameters
    3,                           // Priority (higher number = higher priority)
    NULL                         // Task handle
  );


  xTaskCreate(
    TaskLCDInterface,               // Task function
    (const portCHAR *)"LCDINTERFACE", // Task name
    8192,                        // Stack size
    (void *)params,              // Parameters
    3,                           // Priority (higher number = higher priority)
    NULL                         // Task handle
  );




  // Create the serial logging task






  //  xTaskCreate(
  //   TaskSetCurrentSetValues,           // Task function
  //   (const portCHAR *)"SetCurrentValuesTH", // Task name
  //   1024,                        // Stack size
  //   NULL,                        // Parameters (none in this case)
  //   3,                           // Priority
  //   NULL                         // Task handle
  // );






  // Start the scheduler
  vTaskStartScheduler();
  Serial1.println("Insufficient RAM"); // If there's not enough RAM, print this
  while (1); // Stop execution if the scheduler fails to start
}

void loop() {
  
  // Empty. Things are done in tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/
void blinkLED() {
  // Turn the LED on
  digitalWrite(Led3, HIGH);
  delay(2000);  // Wait for 5 seconds (5000 milliseconds)

  // Turn the LED off
  digitalWrite(Led3, LOW);
  delay(2000);  // Wait for another 5 seconds (5000 milliseconds)
}




void fetchsnmp(){


  snmp.loop();
  sendSNMPRequestAndPrintValues();
}


void softwareRestart() {
  Serial1.println("Restarting the controller...");
  delay(1000);
  NVIC_SystemReset();  // Perform a proper system reset
}

// Task for updating the LCD with the data
void TaskLCDUpdate(void *pvParameters) {
  (void)pvParameters; // Prevent unused parameter warning

  // Initialize variables
 
  // Loop indefinitely to simulate updates
  for (;;) {

       Serial1.printf("%02d/%02d/%02d ", rtc.getDay(), rtc.getMonth(), rtc.getYear());
  Serial1.printf("%02d:%02d:%02d.%03d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), rtc.getSubSeconds());
  if (rtc.getHours() % 3 == 0 && rtc.getMinutes() == 0 && rtc.getSeconds() == 0) {
    softwareRestart();  // Call software restart function
}

    // Update temperature and humidity from the sensor
    int RTemp = H_Sens.getTemperature();
    int RHum = H_Sens.getHumidity();
    PWMTEMP = RTemp;
    delay(85);
    int IN1 = PE0;
    int IN2 = PE1;
    int IN3 = PE2;
    int IN4 = PE3;
    int IN5 = PE4;
    int WLD = PB0;
    
    int FIRE = PE7;



    

  int IN1_Status = digitalRead(IN1);
  int IN2_Status = digitalRead(IN2);
  int  IN3_Status = digitalRead(IN3);
  int IN4_Status = digitalRead(IN4);
  int WLDValue = analogRead(WLD);
  int  WLD_Status = (WLDValue > 400) ? 0 : 1;
   int FIRE_Status = digitalRead(FIRE);


  int SetTEMPHIGH = EEPROM.read(0);
  delay(100);
  int SetTEMPLOW = EEPROM.read(1);
  delay(100);
  int SetHUMHIGH = EEPROM.read(2);
  delay(100);
  int SetHUMLOW = EEPROM.read(3);
  delay(100);

   Serial1.print(IN1_Status);


   String fireStr = (FIRE_Status == 1) ? "YES" : "NO";  // Fire value: 1 -> YES, else NO
  String wldStr = (WLD_Status == 1) ? "YES" : "NO";  // WLD value: 1 -> YES, else NO
  String frontDoorStr = (IN1_Status == 1) ? "CLOSE" : "OPEN";  // Front door: 1 -> CLOSE, else OPEN
  String backDoorStr = (IN2_Status == 1) ? "CLOSE" : "OPEN";  // Back door: 1 -> CLOSE, else OPEN




    // Use the sensor data for the LCD updates
    String command1 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labeltemp\",\"text\":\"";
    command1 += String(RTemp);  // Use real temperature
    command1 += "\"}>ET";
    Serial2.println(command1); // Send temperature command to LCD

    String command2 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelhum\",\"text\":\"";
    command2 += String(RHum);  // Use real humidity
    command2 += "\"}>ET";
    Serial2.println(command2); // Send humidity command to LCD

      if (isnan(RTemp) || isnan(RHum) || RTemp < 0.0 || RTemp > 100.0 || RHum > 100.0) {
            initsensor();
            Serial1.println("Temperature or humidity out of range or failed reading, reinitializing sensor...");
   
        }



    String command3 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelfire\",\"text\":\"";
    command3 += fireStr;  // Add fire status (YES/NO)
    command3 += "\"}>ET";

    Serial2.println(command3); // Send humidity command to LCD
  
    String command4 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelwld\",\"text\":\"";
    command4 += wldStr;  // Add WLD status (YES/NO)
    command4 += "\"}>ET";

    Serial2.println(command4); // Send humidity command to LCD

    String command5 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelfrontdoor\",\"text\":\"";
    command5 += frontDoorStr;  // Add front door status (CLOSE/OPEN)
    command5 += "\"}>ET";
    Serial2.println(command5); // Send humidity command to LCD

    String command6 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelbackdoor\",\"text\":\"";
    command6 += backDoorStr;  // Add back door status (CLOSE/OPEN)
    command6 += "\"}>ET";
    Serial2.println(command6); // Send humidity command to LCD


    Serial2.print("ST<{\"cmd_code\":\"set_value\",\"type\":\"label\",\"widget\":\"labelpdu1\",\"value\":");
    Serial2.print(data1Value, 2);  // Send the actual value directly with 3 decimal places
    Serial2.println(",\"format\":\"%.2f\"}>ET");  // Close the command properly  // Close the command

    Serial2.print("ST<{\"cmd_code\":\"set_value\",\"type\":\"label\",\"widget\":\"labelpdu2\",\"value\":");
    Serial2.print(data2Value, 2);  // Send the actual value directly with 3 decimal places
    Serial2.println(",\"format\":\"%.2f\"}>ET");  // Close the command properly



  String colorCommand;
  if (RTemp < SetTEMPLOW || RTemp > SetTEMPHIGH) {
    colorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labeltemp\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    colorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labeltemp\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(colorCommand);  // Send the color change command for temperature
  //delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(colorCommand);  // Print the color change command for debugging
  
  // Now, set the color of the humidity label based on the humidity
  String humidityColorCommand;
  if (RHum < SetHUMLOW || RHum > SetHUMHIGH) {
    humidityColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelhum\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    humidityColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelhum\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(humidityColorCommand);  // Send the color change command for humidity
  //delay(100);  // Add a short delay for the color change to take effect  


  String fireColorCommand;
  if (FIRE_Status == 1) {
    fireColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfire\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    fireColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfire\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(fireColorCommand);  // Send the color change command for fire
  //delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(fireColorCommand);  // Print the color change command for debugging

  // Now, set the color of the WLD label based on the WLD status
  String wldColorCommand;
  if (WLD_Status == 1) {
    wldColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelwld\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    wldColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelwld\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(wldColorCommand);  // Send the color change command for WLD
  //delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(wldColorCommand);  // Print the color change command for debugging



  String frontdoorcolorcommand;
  if (IN1_Status == 1) {
    frontdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfrontdoor\",\"color_object\":\"normal:text_color\", \"color\":4278255360}>ET";  // Green color
  } else {
    frontdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfrontdoor\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  }
  Serial2.println(frontdoorcolorcommand);  // Send the color change command for WLD
  //delay(100);  // Add a short delay for the color change to take effect
 // Serial1.println(frontdoorcolorcommand);  // Print the color change command for debugging





  String backdoorcolorcommand;
  if (IN2_Status == 1) {
    backdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelbackdoor\",\"color_object\":\"normal:text_color\", \"color\":4278255360}>ET";  // Green color
  } else {
    backdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelbackdoor\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  }
  Serial2.println(backdoorcolorcommand);  // Send the color change command for WLD
 // delay(100);  // Add a short delay for the color change to take effect
 // Serial1.println(backdoorcolorcommand);  // Print the color change command for debugging


  
  if(IN3_Status == 1)
  {
    String openWindowCommand = "ST<{\"cmd_code\":\"open_win\",\"type\":\"window\",\"widget\":\"doors_page\"}>ET";
    Serial2.println(openWindowCommand);  // Send the command to the LCD
    Serial1.println(openWindowCommand);  // Optionally print to Serial1 for debugging


    
  }












    // Send "DONE" to Serial1 to indicate task completion
    Serial1.println("DONE");
    TaskSetCurrentSetValues();
    int pwmValue = mapTemperatureToPWM(PWMTEMP);
    Serial1.println(pwmValue);
    analogWrite(PA0, pwmValue);

    checkconditions(RTemp,RHum);

     
 

    // Simulate delay between updates (1000ms = 1 second)
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second



   

    delay(1000);
  }




}

// Task for logging data to Serial1





void TaskSetCurrentSetValues(){
 

  
  String sethightemp =String(EEPROM.read(0));
  String setlowtemp = String(EEPROM.read(1));
  String sethighhum = String(EEPROM.read(2));
  String setlowhum = String(EEPROM.read(3));




  String command1 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelcurrenht\",\"text\":\"";
  command1 += sethightemp;  // Add temperature value
  command1 += "\"}>ET";

  String command2 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelcurrentlt\",\"text\":\"";
  command2 += setlowtemp;  // Add humidity value
  command2 += "\"}>ET";

  String command3 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelcurrenhh\",\"text\":\"";
  command3 += sethighhum;  // Add fire status (YES/NO)
  command3 += "\"}>ET";
  
  String command4 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelcurrentlh\",\"text\":\"";
  command4 += setlowhum;  // Add WLD status (YES/NO)
  command4 += "\"}>ET";

    Serial2.println(command1);
   // delay(100);
    Serial2.println(command2);
    //delay(100);
    Serial2.println(command3);
   // delay(100);
    Serial2.println(command4);
    delay(100);

  

}









void checkconditions(int temp,int hum){


  
  int IN3_Status = digitalRead(IN3);
  if(IN3_Status == 1)
  {
    String openWindowCommand = "ST<{\"cmd_code\":\"open_win\",\"type\":\"window\",\"widget\":\"doors_page\"}>ET";
    Serial2.println(openWindowCommand);  // Send the command to the LCD
    Serial1.println(openWindowCommand);  // Optionally print to Serial1 for debugging


    
  }


  int IN1_Status = digitalRead(IN1);
  int IN2_Status = digitalRead(IN2);
 // int IN3_Status = digitalRead(IN3);
  int IN4_Status = digitalRead(IN4);
  int  WLDValue = analogRead(WLD);
  int WLD_Status = (WLDValue > 300) ? 0 : 1;
  int FIRE_Status = digitalRead(FIRE);

  int SetTEMPHIGH = EEPROM.read(0);
  delay(100);
  int SetTEMPLOW = EEPROM.read(1);
  delay(100);
  int SetHUMHIGH = EEPROM.read(2);
  delay(100);
  int SetHUMLOW = EEPROM.read(3);
  delay(100);

  
  



  



if (hum >= SetHUMHIGH || hum <= SetHUMLOW || temp >= SetTEMPHIGH || temp <= SetTEMPLOW || FIRE_Status == 1 || WLD_Status == 1 || IN1_Status == 0 || IN2_Status == 0) {
    // Trigger alarm and outputs if any condition is met
    digitalWrite(OUT1, HIGH);
    digitalWrite(OUT2, HIGH);
    digitalWrite(OUT3, HIGH);
    digitalWrite(OUT4, HIGH);
    digitalWrite(ALARM, HIGH);
    GlobalAlarm = 1;
    delay(1000);
    digitalWrite(ALARM, LOW);
    delay(500);
    digitalWrite(Led3, HIGH);

    Serial1.println("INSIDEEEEEEEE ALARMS");

    // Check each condition and print which one is causing the alarm
    if (hum >= SetHUMHIGH) {
        Serial1.println("Humidity is HIGH: " + String(hum) + " >= " + String(SetHUMHIGH));
    } 
    if (hum <= SetHUMLOW) {
        Serial1.println("Humidity is LOW: " + String(hum) + " <= " + String(SetHUMLOW));
    } 
    if (temp >= SetTEMPHIGH) {
        Serial1.println("Temperature is HIGH: " + String(temp) + " >= " + String(SetTEMPHIGH));
    } 
    if (temp <= SetTEMPLOW) {
        Serial1.println("Temperature is LOW: " + String(temp) + " <= " + String(SetTEMPLOW));
    } 
    if (FIRE_Status == 1) {
        Serial1.println("Fire alarm triggered: FIRE_Status == 1");
    } 
    if (WLD_Status == 1) {
        Serial1.println("Water leakage detected: WLD_Status == 1");
    } 
    if (IN1_Status == 0) {
        Serial1.println("Input 1 status is LOW: IN1_Status == 0");
    } 
    if (IN2_Status == 0) {
        Serial1.println("Input 2 status is LOW: IN2_Status == 0");
    }

    // Optionally print the full set of status values for debugging
    Serial1.print("RHum: "); Serial1.println(hum);
    Serial1.print("RTemp: "); Serial1.println(temp);
    Serial1.print("FIRE_Status: "); Serial1.println(FIRE_Status);
    Serial1.print("WLD_Status: "); Serial1.println(WLD_Status);
    Serial1.print("IN1_Status: "); Serial1.println(IN1_Status);
    Serial1.print("IN2_Status: "); Serial1.println(IN2_Status);
}




else{


    digitalWrite(OUT1, LOW);
    digitalWrite(OUT2, LOW);
    digitalWrite(OUT3, LOW);
    digitalWrite(OUT4, LOW);

}


delay(100);

if(IN1_Status==0 || IN2_Status==0 || IN3_Status==1 || IN4_Status==1 || WLD_Status==1 || FIRE_Status==1)
{
digitalWrite(ALARM, HIGH);  
delay(500);
digitalWrite(ALARM, LOW);
delay(500);

Serial1.print("INSIDEEEE TWOOOOO");
}



updateLEDStrip(temp,hum);
}

int mapTemperatureToPWM(float temperature) {
    temperature = constrain(temperature, TEMP_MIN, TEMP_MAX);
    return map(temperature, TEMP_MIN, TEMP_MAX, PWM_MIN, PWM_MAX);
}

void TaskLCDInterface(void *pvParameters){
 (void)pvParameters; // Prevent unused parameter warning




for (;;) {  



  #define MAX_BUFFER_SIZE 128
byte buffer[MAX_BUFFER_SIZE];
int bufferIndex = 0;
#define SEQUENCE_LENGTH 6
byte sequence[] =  {0x16, 0x22, 0x65, 0x64, 0x69, 0x74, 0x33, 0x5F, 0x63, 0x6F, 0x70, 0x79};  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature High Value 

byte sequence2[] = {0x16, 0x22, 0x65, 0x64, 0x69, 0x74, 0x34, 0x5F, 0x63, 0x6F, 0x70, 0x79};  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature LOW Value 

byte sequence3[] = {0x35, 0x5F, 0x63 ,0x6F, 0x70 ,0x79, 0x32 ,0x5F, 0x63, 0x6F, 0x70 ,0x79 };  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature LOW Value 

byte sequence4[] = {0x36, 0x5F, 0x63 ,0x6F, 0x70 ,0x79, 0x32 ,0x5F, 0x63, 0x6F, 0x70 ,0x79 };

byte sequence5[] = {0x70, 0x00, 0x0D, 0x22, 0x65, 0x64, 0x69, 0x74, 0x69, 0x70, 0x6F, 0x6E };

if (Serial2.available() > 0) {
    // Read incoming data byte-by-byte
    while (Serial2.available()) {
      byte incomingByte = Serial2.read();

      // Store the incoming byte in the buffer
      if (bufferIndex < MAX_BUFFER_SIZE) {
        buffer[bufferIndex] = incomingByte;
        bufferIndex++;
      }

      // Print each byte as hexadecimal to the Serial Monitor
      Serial1.print("0x");
      if (incomingByte < 0x10) {
        Serial1.print("0");  // Leading zero for single-byte hex values
      }
      Serial1.print(incomingByte, HEX);  // Print byte in HEX
      Serial1.print(" ");  // Space between bytes
    }
    Serial1.println();  // Print a newline after all bytes are printed

    // Check if the second-to-last byte is 0xC9
    if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0xD3) {



    

    String setTextCommand = "ST<{\"cmd_code\":\"set_text\",\"type\":\"edit\",\"widget\":\"edit2\",\"text\":\"\"}>ET";
    Serial2.println(setTextCommand);  // Send the command to the LCD
    Serial1.println(setTextCommand);  // Optionally print to Serial1 for debugging

    
    
    String openWindowCommand = "ST<{\"cmd_code\":\"open_win\",\"type\":\"window\",\"widget\":\"doors_page\"}>ET";
    Serial2.println(openWindowCommand);  // Send the command to the LCD
    Serial1.println(openWindowCommand);  // Optionally print to Serial1 for debugging








      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      Serial1.println("Second last byte was 0xC9, command sent to LCD.");
    }

     if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0x62 && buffer[bufferIndex - 1] == 0x45) {

    
   
      Serial1.println("D1");  // Optionally print to Serial1 for debugging


      //digitalWrite(OUT1, HIGH); 
      digitalWrite(OUT4, HIGH);
      delay(3000);
      //digitalWrite(OUT1, LOW);
      digitalWrite(OUT4, LOW);



      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      
    }


     if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0xA2 && buffer[bufferIndex - 1] == 0x78) {

    
   
     Serial1.println("D2");  // Optionally print to Serial1 for debugging

      //digitalWrite(OUT2, HIGH);
      digitalWrite(OUT5, HIGH);
      delay(3000);
      //digitalWrite(OUT2, LOW);
      digitalWrite(OUT5, LOW);



      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      
    }


    if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0x67 && buffer[bufferIndex - 1] == 0xFC) { //for the high temp setting 

     String command = "ST<{\"cmd_code\":\"get_text\",\"type\":\"edit\",\"widget\":\"edit3_copy1_copy1\"}>ET";
     Serial2.println(command);  // Send the command to LCD over Serial2
     Serial1.println("Set HIGH TEMP");

   
   


      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      
    }




    if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0x67 && buffer[bufferIndex - 1] == 0xB8) { //for the low temp setting 

     String command = "ST<{\"cmd_code\":\"get_text\",\"type\":\"edit\",\"widget\":\"edit4_copy2_copy2\"}>ET";
     Serial2.println(command);  // Send the command to LCD over Serial2
     Serial1.println("Set LOW TEMP");

   
   


      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      
    }



    if (bufferIndex >= SEQUENCE_LENGTH) {
      // Scan through the buffer to check if the sequence exists
      for (int i = 0; i <= bufferIndex - SEQUENCE_LENGTH; i++) {
        // Compare the current position in the buffer with the sequence
         if (buffer[i] == sequence[0] &&
            buffer[i + 1] == sequence[1] &&
            buffer[i + 2] == sequence[2] &&
            buffer[i + 3] == sequence[3] &&
            buffer[i + 4] == sequence[4] &&
            buffer[i + 5] == sequence[5] &&
            buffer[i + 6] == sequence[6] &&
            buffer[i + 7] == sequence[7] &&
            buffer[i + 8] == sequence[8] &&
            buffer[i + 9] == sequence[9] &&
            buffer[i + 10] == sequence[10] &&
            buffer[i + 11] == sequence[11]) {
          // Sequence found, perform action
          Serial1.println("Hex sequence detected For Temp High Setting !");

          // Extract the 7th and 6th last bytes, which are 7th and 6th before the end of the buffer
          if (bufferIndex > 7) {  // Ensure there's enough data to extract the values
            byte extractedValue1 = buffer[bufferIndex - 7];  // 7th last byte
            byte extractedValue2 = buffer[bufferIndex - 6];  // 6th last byte

            // Print out extracted values for debugging
            Serial1.print("Extracted Values: ");
            Serial1.print(extractedValue1, HEX);
            Serial1.print(" ");
            Serial1.println(extractedValue2, HEX);

            // Convert the extracted bytes to a single value (in this case, extract '26' from '0x32' and '0x36')
            int value = (extractedValue1 - 0x30) * 10 + (extractedValue2 - 0x30);  // Convert ASCII to numeric

            // Output the value to Serial1 for debugging
            Serial1.print("Extracted Value: ");
            Serial1.println(value);  // This should print 26 (from '0x32' and '0x36')

            // You can store the value in EEPROM, if needed
             EEPROM.write(0, value);  // For example, writing to EEPROM at address 0

            // Send the extracted value or perform other actions
          

            // Optionally clear the buffer after processing to prepare for the next packet
            bufferIndex = 0;
            break;  // Exit the loop once the sequence is found and processed
          }
        }
      }
    }



if (bufferIndex >= SEQUENCE_LENGTH) {
      // Scan through the buffer to check if the sequence exists
      for (int i = 0; i <= bufferIndex - SEQUENCE_LENGTH; i++) {
        // Compare the current position in the buffer with the sequence
         if (buffer[i] == sequence2[0] &&
            buffer[i + 1] == sequence2[1] &&
            buffer[i + 2] == sequence2[2] &&
            buffer[i + 3] == sequence2[3] &&
            buffer[i + 4] == sequence2[4] &&
            buffer[i + 5] == sequence2[5] &&
            buffer[i + 6] == sequence2[6] &&
            buffer[i + 7] == sequence2[7] &&
            buffer[i + 8] == sequence2[8] &&
            buffer[i + 9] == sequence2[9] &&
            buffer[i + 10] == sequence2[10] &&
            buffer[i + 11] == sequence2[11]) {
          // Sequence found, perform action
          Serial1.println("Hex sequence detected For Temp LOW Setting !");

          // Extract the 7th and 6th last bytes, which are 7th and 6th before the end of the buffer
          if (bufferIndex > 7) {  // Ensure there's enough data to extract the values
            byte extractedValue1 = buffer[bufferIndex - 7];  // 7th last byte
            byte extractedValue2 = buffer[bufferIndex - 6];  // 6th last byte

            // Print out extracted values for debugging
            Serial1.print("Extracted Values: ");
            Serial1.print(extractedValue1, HEX);
            Serial1.print(" ");
            Serial1.println(extractedValue2, HEX);

            // Convert the extracted bytes to a single value (in this case, extract '26' from '0x32' and '0x36')
            int value = (extractedValue1 - 0x30) * 10 + (extractedValue2 - 0x30);  // Convert ASCII to numeric

            // Output the value to Serial1 for debugging
            Serial1.print("Extracted Value: ");
            Serial1.println(value);  // This should print 26 (from '0x32' and '0x36')

            // You can store the value in EEPROM, if needed
             EEPROM.write(1, value);  // For example, writing to EEPROM at address 0

            // Send the extracted value or perform other actions
          

            // Optionally clear the buffer after processing to prepare for the next packet
            bufferIndex = 0;
            break;  // Exit the loop once the sequence is found and processed
          }
        }
      }
    }


    if (bufferIndex >= SEQUENCE_LENGTH) {
      // Scan through the buffer to check if the sequence exists
      for (int i = 0; i <= bufferIndex - SEQUENCE_LENGTH; i++) {
        // Compare the current position in the buffer with the sequence
         if (buffer[i] == sequence3[0] &&
            buffer[i + 1] == sequence3[1] &&
            buffer[i + 2] == sequence3[2] &&
            buffer[i + 3] == sequence3[3] &&
            buffer[i + 4] == sequence3[4] &&
            buffer[i + 5] == sequence3[5] &&
            buffer[i + 6] == sequence3[6] &&
            buffer[i + 7] == sequence3[7] &&
            buffer[i + 8] == sequence3[8] &&
            buffer[i + 9] == sequence3[9] &&
            buffer[i + 10] == sequence3[10] &&
            buffer[i + 11] == sequence3[11]) {
          // Sequence found, perform action
          Serial1.println("Hex sequence detected For Hum High Setting !");

          // Extract the 7th and 6th last bytes, which are 7th and 6th before the end of the buffer
          if (bufferIndex > 7) {  // Ensure there's enough data to extract the values
            byte extractedValue1 = buffer[bufferIndex - 7];  // 7th last byte
            byte extractedValue2 = buffer[bufferIndex - 6];  // 6th last byte

            // Print out extracted values for debugging
            Serial1.print("Extracted Values: ");
            Serial1.print(extractedValue1, HEX);
            Serial1.print(" ");
            Serial1.println(extractedValue2, HEX);

            // Convert the extracted bytes to a single value (in this case, extract '26' from '0x32' and '0x36')
            int value = (extractedValue1 - 0x30) * 10 + (extractedValue2 - 0x30);  // Convert ASCII to numeric

            // Output the value to Serial1 for debugging
            Serial1.print("Extracted Value: ");
            Serial1.println(value);  // This should print 26 (from '0x32' and '0x36')

            // You can store the value in EEPROM, if needed
             EEPROM.write(2, value);  // For example, writing to EEPROM at address 0

            // Send the extracted value or perform other actions
          

            // Optionally clear the buffer after processing to prepare for the next packet
            bufferIndex = 0;
            break;  // Exit the loop once the sequence is found and processed
          }
        }
      }
    }



      if (bufferIndex >= SEQUENCE_LENGTH) {
      // Scan through the buffer to check if the sequence exists
      for (int i = 0; i <= bufferIndex - SEQUENCE_LENGTH; i++) {
        // Compare the current position in the buffer with the sequence
         if (buffer[i] == sequence4[0] &&
            buffer[i + 1] == sequence4[1] &&
            buffer[i + 2] == sequence4[2] &&
            buffer[i + 3] == sequence4[3] &&
            buffer[i + 4] == sequence4[4] &&
            buffer[i + 5] == sequence4[5] &&
            buffer[i + 6] == sequence4[6] &&
            buffer[i + 7] == sequence4[7] &&
            buffer[i + 8] == sequence4[8] &&
            buffer[i + 9] == sequence4[9] &&
            buffer[i + 10] == sequence4[10] &&
            buffer[i + 11] == sequence4[11]) {
          // Sequence found, perform action
          Serial1.println("Hex sequence detected For Hum Low Setting !");

          // Extract the 7th and 6th last bytes, which are 7th and 6th before the end of the buffer
          if (bufferIndex > 7) {  // Ensure there's enough data to extract the values
            byte extractedValue1 = buffer[bufferIndex - 7];  // 7th last byte
            byte extractedValue2 = buffer[bufferIndex - 6];  // 6th last byte

            // Print out extracted values for debugging
            Serial1.print("Extracted Values: ");
            Serial1.print(extractedValue1, HEX);
            Serial1.print(" ");
            Serial1.println(extractedValue2, HEX);

            // Convert the extracted bytes to a single value (in this case, extract '26' from '0x32' and '0x36')
            int value = (extractedValue1 - 0x30) * 10 + (extractedValue2 - 0x30);  // Convert ASCII to numeric

            // Output the value to Serial1 for debugging
            Serial1.print("Extracted Value: ");
            Serial1.println(value);  // This should print 26 (from '0x32' and '0x36')

            // You can store the value in EEPROM, if needed
             EEPROM.write(3, value);  // For example, writing to EEPROM at address 0

            // Send the extracted value or perform other actions
          

            // Optionally clear the buffer after processing to prepare for the next packet
            bufferIndex = 0;
            break;  // Exit the loop once the sequence is found and processed
          }
        }
      }
    }

    // ip address one 
    // Optionally clear the buffer after processing to prepare for the next packet
    bufferIndex = 0;
   
  }

   fetchsnmp();
   //
}  





}
