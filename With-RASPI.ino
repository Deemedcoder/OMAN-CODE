#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include<SHT25.h>
#include <DHT.h>;
#define DHTPIN PE8     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
SHT25 H_Sens;
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
#include <EEPROM.h>
#include <ArduinoJson.h> // Include the ArduinoJson library

// Define custom Serial1 and Serial2 instances
HardwareSerial Serial1(PA10, PA9); // Debugging via Serial1
HardwareSerial Serial2(PA3, PA2); // Communication with LCD via Serial2
HardwareSerial Serial3(PB11,PB10);


String receivedData;  // To store incoming data
int RTemp=0;
int RHum=0;
int pdu1 = 0;
// Define the GPIO pin and the number of LEDs
#define PINLED            PC11       // Update with the GPIO pin you're using
#define NUM_LEDS       60         // Set the number of LEDs to 60
Adafruit_NeoPixel strip(NUM_LEDS, PINLED, NEO_GRB + NEO_KHZ800);  // GRB order for WS2812B


// Variables For The RIM360

int GlobalAlarm = 0;
int SetHUMHIGH =0;
int SetTEMPHIGH =0;
int SetHUMLOW =0;
int SetTEMPLOW =0;
int IN1 = PE0;
int IN2 = PE1;
int IN3 = PE2;
int IN4 = PE3;
int IN5 = PE4;
int WLD = PB0;
int WLDValue=0; 
int FIRE = PE7;
int SMOKE = PA1;
int ALARM = PE14;
int IN1_Status = 0;
int IN2_Status = 0;
int IN3_Status = 0;
int IN4_Status = 0;
int WLD_Status = 0;
int FIRE_Status = 0;
int SMOKE_Status = 0;
int OUT1 = PE10;
int OUT2 = PE11;
int OUT3 = PE12;
int OUT4 = PE13;
int OUT5 = PB1;
int Led1 = PD9;
int Led2 = PD10;
int Led3 = PD11; 
uint64_t uid = 0;
int ChipID;
// Variables For The Rim360 End 
float data1Value;  // Variable to store the value of data1
float data2Value;  // Variable to store the value of data2
// Buffer to store incoming bytes
#define MAX_BUFFER_SIZE 128
byte buffer[MAX_BUFFER_SIZE];
int bufferIndex = 0;
byte sequence[] =  {0x16, 0x22, 0x65, 0x64, 0x69, 0x74, 0x33, 0x5F, 0x63, 0x6F, 0x70, 0x79};  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature High Value 

byte sequence2[] = {0x16, 0x22, 0x65, 0x64, 0x69, 0x74, 0x34, 0x5F, 0x63, 0x6F, 0x70, 0x79};  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature LOW Value 

byte sequence3[] = {0x35, 0x5F, 0x63 ,0x6F, 0x70 ,0x79, 0x32 ,0x5F, 0x63, 0x6F, 0x70 ,0x79 };  // 0x33 0x5F 0x63 0x6F 0x70 0x79   This Is The Sequence For The Temperature LOW Value 

byte sequence4[] = {0x36, 0x5F, 0x63 ,0x6F, 0x70 ,0x79, 0x32 ,0x5F, 0x63, 0x6F, 0x70 ,0x79 };

byte sequence5[] = {0x70, 0x00, 0x0D, 0x22, 0x65, 0x64, 0x69, 0x74, 0x69, 0x70, 0x6F, 0x6E };





#define SEQUENCE_LENGTH 6


// Variable for value that will be sent in the command
int value = 23; // Example initial value
int value2 = 55;
String value3 = "NO";

// Function to send home data to the LCD
void sendHomeData(int temperature, int humidity, int fire, int wld, int frontDoor, int backDoor , float pdu1value , float pdu2value) {
  // Define the temperature, humidity, fire, and wld ranges
  int minTemp = EEPROM.read(1);  // Example minimum temperature (can be adjusted)
  int maxTemp = EEPROM.read(0);  // Example maximum temperature (can be adjusted)
  int minHumidity = EEPROM.read(3);  // Example minimum humidity (can be adjusted)
  int maxHumidity = EEPROM.read(2);  // Example maximum humidity (can be adjusted)

  // Convert the int values to Strings
  String tempStr = String(temperature);  // Temperature value as String
  String humidityStr = String(humidity);  // Humidity value as String
  String fireStr = (fire == 1) ? "YES" : "NO";  // Fire value: 1 -> YES, else NO
  String wldStr = (wld == 1) ? "YES" : "NO";  // WLD value: 1 -> YES, else NO
  String frontDoorStr = (frontDoor == 1) ? "CLOSE" : "OPEN";  // Front door: 1 -> CLOSE, else OPEN
  String backDoorStr = (backDoor == 1) ? "CLOSE" : "OPEN";  // Back door: 1 -> CLOSE, else OPEN

  // Construct commands with dynamic values for each label
  String command1 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labeltemp\",\"text\":\"";
  command1 += tempStr;  // Add temperature value
  command1 += "\"}>ET";

  String command2 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelhum\",\"text\":\"";
  command2 += humidityStr;  // Add humidity value
  command2 += "\"}>ET";

  String command3 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelfire\",\"text\":\"";
  command3 += fireStr;  // Add fire status (YES/NO)
  command3 += "\"}>ET";
  
  String command4 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelwld\",\"text\":\"";
  command4 += wldStr;  // Add WLD status (YES/NO)
  command4 += "\"}>ET";

  String command5 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelfrontdoor\",\"text\":\"";
  command5 += frontDoorStr;  // Add front door status (CLOSE/OPEN)
  command5 += "\"}>ET";

  String command6 = "ST<{\"cmd_code\":\"set_text\",\"type\":\"label\",\"widget\":\"labelbackdoor\",\"text\":\"";
  command6 += backDoorStr;  // Add back door status (CLOSE/OPEN)
  command6 += "\"}>ET";




Serial2.print("ST<{\"cmd_code\":\"set_value\",\"type\":\"label\",\"widget\":\"labelpdu1\",\"value\":");
Serial2.print(pdu1value, 2);  // Send the actual value directly with 3 decimal places
Serial2.println(",\"format\":\"%.2f\"}>ET");  // Close the command properly  // Close the command

Serial2.print("ST<{\"cmd_code\":\"set_value\",\"type\":\"label\",\"widget\":\"labelpdu2\",\"value\":");
Serial2.print(pdu2value, 2);  // Send the actual value directly with 3 decimal places
Serial2.println(",\"format\":\"%.2f\"}>ET");  // Close the command properly



  // Send the command to the LCD over Serial2
  Serial2.println(command1);
  delay(100);  // Add a short delay between commands
  Serial2.println(command2);
  delay(100);
  Serial2.println(command3);
  delay(100);
  Serial2.println(command4);
  delay(100);
  Serial2.println(command5);
  delay(100);
  Serial2.println(command6);
  delay(100);
  


  // Optionally, print the sent commands to Serial1 for debugging
  // Serial1.println(command1);
  // Serial1.println(command2);
  // Serial1.println(command3);
  // Serial1.println(command4);
  // Serial1.println(command5);
  // Serial1.println(command6);

  // Now, set the color of the temperature label based on the temperature
  String colorCommand;
  if (temperature < minTemp || temperature > maxTemp) {
    colorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labeltemp\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    colorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labeltemp\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(colorCommand);  // Send the color change command for temperature
  delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(colorCommand);  // Print the color change command for debugging
  
  // Now, set the color of the humidity label based on the humidity
  String humidityColorCommand;
  if (humidity < minHumidity || humidity > maxHumidity) {
    humidityColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelhum\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    humidityColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelhum\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(humidityColorCommand);  // Send the color change command for humidity
  delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(humidityColorCommand);  // Print the color change command for debugging
  
  // Now, set the color of the fire label based on the fire status
  String fireColorCommand;
  if (fire == 1) {
    fireColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfire\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    fireColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfire\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(fireColorCommand);  // Send the color change command for fire
  delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(fireColorCommand);  // Print the color change command for debugging

  // Now, set the color of the WLD label based on the WLD status
  String wldColorCommand;
  if (wld == 1) {
    wldColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelwld\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  } else {
    wldColorCommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelwld\",\"color_object\":\"normal:text_color\", \"color\":4294967295 }>ET";  // White color
  }
  Serial2.println(wldColorCommand);  // Send the color change command for WLD
  delay(100);  // Add a short delay for the color change to take effect
  //Serial1.println(wldColorCommand);  // Print the color change command for debugging



  String frontdoorcolorcommand;
  if (frontDoor == 1) {
    frontdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfrontdoor\",\"color_object\":\"normal:text_color\", \"color\":4278255360}>ET";  // Green color
  } else {
    frontdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelfrontdoor\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  }
  Serial2.println(frontdoorcolorcommand);  // Send the color change command for WLD
  delay(100);  // Add a short delay for the color change to take effect
 // Serial1.println(frontdoorcolorcommand);  // Print the color change command for debugging





  String backdoorcolorcommand;
  if (backDoor == 1) {
    backdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelbackdoor\",\"color_object\":\"normal:text_color\", \"color\":4278255360}>ET";  // Green color
  } else {
    backdoorcolorcommand = "ST<{\"cmd_code\":\"set_color\",\"type\":\"widget\",\"widget\":\"labelbackdoor\",\"color_object\":\"normal:text_color\", \"color\":4294901760}>ET";  // Red color
  }
  Serial2.println(backdoorcolorcommand);  // Send the color change command for WLD
  delay(100);  // Add a short delay for the color change to take effect
 // Serial1.println(backdoorcolorcommand);  // Print the color change command for debugging


}

void setcurrentsetvalues (){



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
    delay(100);
    Serial2.println(command2);
    delay(100);
    Serial2.println(command3);
    delay(100);
    Serial2.println(command4);
    delay(100);



}











void setup() {

// PUTTING THE UID 

  for (int i = 0; i < 12; i++) {
    uid |= ((uint64_t)*((uint8_t*)0x1FFF7A10 + i) << (i * 8));
  }
  // Truncate to 16 digits
  ChipID = uid % 10000000000000000;



  // PUTTING THE UID 
  // Initialize debugging on Serial1 (the USB serial port for debugging)
  Serial1.begin(115200);
  Serial1.println("Hello from Serial1 (USB Debugging)");

  // Initialize hardware serial communication on Serial2 (connected to LCD on pins PA3, PA2)
  Serial2.begin(115200);  // Set baud rate to 115200 for Serial2
  Serial1.println("Initialized Serial2 (LCD) at 115200 baud.");
  dht.begin();


   if(H_Sens.begin()){
    Serial1.println("SHT25 initialization successeful!");
    }else{
    Serial1.println("Initialization failed check for connections!");     //sht25
    }


  Serial3.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to "off"

  // Wait for serial communication to initialize
  while (!Serial2) {
    ; // Wait for the serial port to open (important for hardware serial initialization)
  }

  // Wait for Serial Monitor to initialize (useful if debugging over USB Serial)
  while (!Serial) {
    ;
  }

// PINS CONFIGURATION 



  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  pinMode(IN3, INPUT);
  pinMode(IN4, INPUT);
  pinMode(IN5, INPUT);
  pinMode(WLD, INPUT);
  pinMode(FIRE, INPUT);
  pinMode(SMOKE, INPUT);  
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(OUT5, OUTPUT);
  pinMode(ALARM, OUTPUT);

  pinMode(Led1, OUTPUT);
  pinMode(Led2, OUTPUT);
  pinMode(Led3, OUTPUT);

  digitalWrite(Led1, HIGH);
  digitalWrite(Led2, HIGH);
  digitalWrite(Led3, HIGH);
  delay(2000);
  digitalWrite(Led1, LOW);
  digitalWrite(Led2, LOW);
  digitalWrite(Led3, LOW);
  digitalWrite(Led1, HIGH);

  digitalWrite(OUT1, LOW);
  digitalWrite(OUT2, LOW);
  digitalWrite(OUT3, LOW);
  digitalWrite(OUT4, LOW);
  digitalWrite(OUT5, LOW);
  
 for (int i = 0; i < 10; i++) {
    digitalWrite(ALARM, HIGH); // Set the pin HIGH
    delay(300); // Delay for 300 milliseconds
    digitalWrite(ALARM, LOW); // Set the pin LOW
    delay(300); // Delay for 300 milliseconds
  }
  digitalWrite(ALARM, HIGH);
  delay(2000);
  digitalWrite(ALARM, LOW);

//PINS CONFIGURATION 


// SETTING VALUES FOR TEMP HIGH , TEMP LOW , HUMIDITY HIGH , HUMIDITY LOW IF NOT PRESENT OTHERWISE JUST INITIATING THEM TO VAIRABLES

   // Set default value for EEPROM address 0
  if (EEPROM.read(0) == 255) {
    // If blank, write 27 to the address
    EEPROM.write(0, 27);
    Serial1.println("EEPROM address was blank. For Temp HIGH Wrote 27 to it.");
  } else {
    // If not blank, read and print the value
    SetTEMPHIGH = EEPROM.read(0);
    Serial1.print("EEPROM address already had Temp HIGH value: ");
    Serial1.println(SetTEMPHIGH);
    
  }

  // Set default value for EEPROM address 1
  if (EEPROM.read(1) == 255) {
    // If blank, write 18 to the address
    EEPROM.write(1, 18);
    Serial1.println("EEPROM address was blank. Temperature LOW  Wrote 18 to it.");
  } else {
    // If not blank, read and print the value
    SetTEMPLOW = EEPROM.read(1);
    Serial1.print("EEPROM address already had Temperature LOW value: ");
    Serial1.println(SetTEMPLOW);
  }

  // Set default value for EEPROM address 2
  

  // Set default value for EEPROM address 3
  if (EEPROM.read(2) == 255) {
    // If blank, write 65 to the address
    EEPROM.write(2, 65);
    Serial1.println("EEPROM address was blank. HUM HIGH Wrote 65 to it.");
  } else {
    // If not blank, read and print the value
    SetHUMHIGH = EEPROM.read(2);
    Serial1.print("EEPROM address already had Set HUM HIGH Value : ");
    Serial1.println(SetHUMHIGH);
  }


  if (EEPROM.read(3) == 255) {
    // If blank, write 55 to the address
    EEPROM.write(3, 55);
    Serial1.println("EEPROM address was blank. HUM LOW Wrote 55 to it.");
  } else {
    // If not blank, read and print the value
    SetHUMLOW = EEPROM.read(3);
    Serial1.print("EEPROM address already had Set HUM LOW Value : ");
    Serial1.println(SetHUMLOW);
  }


// SETTING VALUES FOR TEMP HIGH , TEMP LOW , HUMIDITY HIGH , HUMIDITY LOW IF NOT PRESENT OTHERWISE JUST INITIATING THEM TO VAIRABLES


//PRINTING THE UID OF THE DEVICE 
  Serial1.println(ChipID);
//PRINTING THE UID OF THE DEVICE 


 




}

void loop() {

 



   if (Serial1.available()) {
    // Read the incoming byte
    String receivedData = Serial1.readStringUntil('\n');  // Read until newline character
    //Serial.println(receivedData);
    RTemp = receivedData.toInt();
    // Print the received data to the Serial Monitor
    

    // Optional: You can parse and use the received data
    // For example, if the data is numeric, you can convert it
    // float value = receivedData.toFloat();
    // Serial.print("Parsed Value: ");
    // Serial.println(value);
  }
  // Check if data is available to read on Serial2
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

    String openWindowCommand = "ST<{\"cmd_code\":\"open_win\",\"type\":\"window\",\"widget\":\"doors_page\"}>ET";
    Serial2.println(openWindowCommand);  // Send the command to the LCD
    Serial1.println(openWindowCommand);  // Optionally print to Serial1 for debugging



      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      Serial1.println("Second last byte was 0xC9, command sent to LCD.");
    }

     if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0x62 && buffer[bufferIndex - 1] == 0x45) {

    
   
    Serial1.println("D1");  // Optionally print to Serial1 for debugging


      digitalWrite(OUT1, HIGH); 
      digitalWrite(OUT4, HIGH);
     
      delay(3000);
      digitalWrite(OUT1, LOW);
      digitalWrite(OUT4, LOW);



      // If second-to-last byte is 0xC9, call sendHomeData function
       // Pass the current value to send in the command
      
    }


     if (bufferIndex > 1 && buffer[bufferIndex - 2] == 0xA2 && buffer[bufferIndex - 1] == 0x78) {

    
   
    Serial1.println("D2");  // Optionally print to Serial1 for debugging

      digitalWrite(OUT2, HIGH);
      digitalWrite(OUT5, HIGH);
     
      delay(3000);
      digitalWrite(OUT2, LOW);
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



  
  // Example: Change the value dynamically for testing purposes (this can be modified as needed)
  // You can update value based on other conditions, sensors, or other inputs
 
 sendHomeData(RTemp, RHum, FIRE_Status, WLD_Status, IN1_Status, IN2_Status,data1Value,data2Value);
 setcurrentsetvalues();


  if (Serial3.available() > 0) {
    // Read the incoming byte from Serial3 until a newline character
    receivedData = Serial3.readStringUntil('\n');  // Read until newline

    // Print the received data to Serial1 (Serial Monitor for debugging)
    Serial1.println("Received Data from Serial3: ");
    Serial1.println(receivedData);

    // Assign the received data to the global tempStr
    
    // Parse the JSON data
    StaticJsonDocument<200> doc;  // Create a JSON document to hold the parsed data
    DeserializationError error = deserializeJson(doc, receivedData);  // Parse the JSON data

    if (!error) {
      // Extract the values of data1 and data2 if present
      if (doc.containsKey("data2")) {
        data1Value = doc["data1"].as<String>().toFloat();
        data2Value = doc["data2"].as<String>().toFloat();


        Serial1.println(data1Value);

        Serial1.print("Data1: ");
        Serial1.println(data1Value);
        Serial1.print("Data2: ");
        Serial1.println(data2Value);
      } else {
        data1Value = doc["data1"].as<String>().toFloat();
        Serial1.print("Data1: ");
        Serial1.println(data1Value);
      }
    } else {
      Serial1.println("Failed to parse JSON data");
    }

    // Clean the buffer (flush the incoming data to prevent overflow)
    Serial3.flush();  // Clear the Serial3 buffer

    blinkLED();
  }

  
   // Example of changing the value to 10, this will update the command sent to LCD
  delay(1000);  // Wait for 1 second before checking again


  IN3_Status = digitalRead(IN3);
  if(IN3_Status == 1)
  {
    String openWindowCommand = "ST<{\"cmd_code\":\"open_win\",\"type\":\"window\",\"widget\":\"doors_page\"}>ET";
    Serial2.println(openWindowCommand);  // Send the command to the LCD
    Serial1.println(openWindowCommand);  // Optionally print to Serial1 for debugging


    
  }


IN1_Status = digitalRead(IN1);
IN2_Status = digitalRead(IN2);
IN3_Status = digitalRead(IN3);
IN4_Status = digitalRead(IN4);
WLDValue = analogRead(WLD);
WLD_Status = (WLDValue > 400) ? 0 : 1;
FIRE_Status = digitalRead(FIRE);




if (RHum >= SetHUMHIGH || RHum <= SetHUMLOW || RTemp >= SetTEMPHIGH || RTemp <= SetTEMPLOW || FIRE_Status == 1 || WLD_Status == 1 || IN1_Status == 0 || IN2_Status == 0) {
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
    digitalWrite(Led3,HIGH);
} else {
    // Turn off outputs if no alarm condition is met
    digitalWrite(OUT1, LOW);
    digitalWrite(OUT2, LOW);
    digitalWrite(OUT3, LOW);
    digitalWrite(OUT4, LOW);
    digitalWrite(ALARM, LOW);
    GlobalAlarm = 0;
    digitalWrite(Led3,LOW);
}

delay(100);

if(IN1_Status==0 || IN2_Status==0 || IN3_Status==1 || IN4_Status==1 || WLD_Status==1 || FIRE_Status==1)
{
digitalWrite(ALARM, HIGH);  
delay(500);
digitalWrite(ALARM, LOW);
delay(500);
}



SetTEMPHIGH = EEPROM.read(0);
SetTEMPLOW = EEPROM.read(1);
SetHUMHIGH = EEPROM.read(2);
SetHUMLOW = EEPROM.read(3); 


  RHum = H_Sens.getHumidity();
  RTemp = H_Sens.getTemperature();

  



updateLEDStrip(RTemp, RHum, FIRE_Status, WLD_Status,
                 SetTEMPLOW, SetTEMPHIGH, SetTEMPLOW, SetHUMHIGH,IN1_Status,IN2_Status);


}




void updateLEDStrip(float currentTemperature, float currentHumidity, int fireStatus, int wldStatus,
                    float setLowTempPoint, float setHighTempPoint, float setLowHumidityPoint, float setHighHumidityPoint, int doorFrontStatus, int doorBackStatus) {

  uint32_t color = strip.Color(0, 255, 0);  // Default color to green

  // Check for fire status first (highest priority)
  if (fireStatus == 1) {
    color = strip.Color(255, 0, 0);  // Red if fire detected
  }



  if (wldStatus == 1) {
    color = strip.Color(255, 0, 0);  // Red if fire detected
  }


   if (doorFrontStatus == 0) {
    color = strip.Color(255, 165, 0);  // Orange if doorfront is open (status 1)
  } 
  else if (doorBackStatus == 0) {
    color = strip.Color(255, 165, 0);  // Orange if door 2 is open (status 1)
  }
  // Then check for temperature condition (if no fire)
  else if (currentTemperature <= setLowTempPoint) {
    color = strip.Color(255, 255, 0);  // Yellow if temperature is below or equal to low point
  } else if (currentTemperature >= setHighTempPoint) {
    color = strip.Color(255, 0, 0);  // Red if temperature is above or equal to high point
  }
  // Then check for humidity condition (if no fire or temperature condition met)
  else if (currentHumidity <= setLowHumidityPoint) {
    color = strip.Color(255, 255, 0);  // Yellow if humidity is below or equal to low point
  } else if (currentHumidity >= setHighHumidityPoint) {
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

void blinkLED() {
  // Turn the LED on
  digitalWrite(Led3, HIGH);
  delay(2000);  // Wait for 5 seconds (5000 milliseconds)

  // Turn the LED off
  digitalWrite(Led3, LOW);
  delay(2000);  // Wait for another 5 seconds (5000 milliseconds)
}









