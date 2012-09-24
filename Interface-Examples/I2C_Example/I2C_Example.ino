/*
 9-19-2012
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 OpenSegment is an open source seven segment display. 

 This is the test code we run on an Arduino to make sure the display is working. It also serves as an example
 of how to control OpenSegment using different protocols.
 
 To get this code to work, attached an OpenSegment to an Arduino Uno using the following pins:
 A5 to SCL
 A4 to SDA
 VIN to PWR
 GND to GND
 
 ToDo:
 I2C test sending - done 9/19/12
 I2C test receving 
 I2C test sending to multiple displays
 Serial testing
 SPI test sending
 SPI test sending to multiple displays
 
 Test UART change
 Test display data request
 Test unit setting request
*/

#include <Wire.h>

#define DISPLAY_ADDRESS1 0x3C //This is the default address of the OpenSegment with both solder jumpers open

#define DISPLAY_SIZE 4

byte tempFrame[DISPLAY_SIZE]; //This assumes we are attached to a 4 digit display

char tempString[100]; //Used for the sprintf based debug statements

int cycles = 0;

void setup() {

  Wire.begin(); //Join the bus as master

  Serial.begin(115200);
  Serial.println("OpenSegment Test Code");
  
}

void loop() {
  Serial.print("Cycle: ");
  Serial.println(cycles++);

  Serial.println("Sending new data");
  i2cTestSend(); //Sends a series of i2c characters
  delay(500);

  i2cRequestSettings(); //What is the display currently displaying?
  delay(500);

  i2cRequestData(); //What is the display currently displaying?
  delay(500);
}

//Gets the current settings from OpenSegment
void i2cRequestSettings(void) {
  
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x71); //Sending 0x70 will give us visible data. 0x71 will give us the unit's settings.
  Wire.endTransmission();

  //Go get three bytes from display #1
  Wire.requestFrom(DISPLAY_ADDRESS1, 3);

  //Collect the incoming 3 bytes
  int x = 0;
  while(Wire.available()) {
    tempFrame[x++] = Wire.read();
    if(x == 3) {Serial.print("!"); break;}
  }
  Wire.endTransmission();

  //Display the data
  Serial.print("Settings: ");
  for(int x = 0 ; x < 3 ; x++) {
    sprintf(tempString, "0x%02X ", tempFrame[x]);
    Serial.print(tempString);
  }
  Serial.println();
}

//Gets the current settings from OpenSegment
void i2cRequestData(void) {

  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x70); //Sending 0x70 will give us visible data. 0x71 will give us the unit's settings.
  Wire.endTransmission();

  //Go get three bytes from display #1
  Wire.requestFrom(DISPLAY_ADDRESS1, 4);

  //Collect the incoming 4 bytes
  int x = 0;
  while(Wire.available()) {
    tempFrame[x++] = Wire.read();
    if(x == 4) {Serial.print("!"); break;}
  }
  Wire.endTransmission();
  
  //Display the data
  Serial.print("Data: ");
  for(int x = 0 ; x < 4 ; x++) {
    sprintf(tempString, "0x%02X ", tempFrame[x]);
    Serial.print(tempString);
  }
  Serial.println();
  
}

//Sends two pieces of data to OpenSegment and then the clear command
//This should leave the display with a 6 in the first position
void i2cTestSend(void) {
  int tempCycles = cycles;

  Wire.beginTransmission(DISPLAY_ADDRESS1); // transmit to device #1
  Wire.write(tempCycles / 1000);
  tempCycles %= 1000;
  Wire.write(tempCycles / 100);
  tempCycles %= 100;
  Wire.write(tempCycles / 10);
  tempCycles %= 10;
  Wire.write(tempCycles);
  Wire.endTransmission(); //Stop I2C transmission
}
