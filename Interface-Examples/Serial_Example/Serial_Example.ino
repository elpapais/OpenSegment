/*
 9-23-2012
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 OpenSegment is an open source seven segment display. 

 This is the test code we run on an Arduino to make sure the display is working. It also serves as an example
 of how to control OpenSegment using different protocols.
 
 To get this code to work, attached an OpenSegment to an Arduino Uno using the following pins:
 Pin 7 on Uno (software serial RX) to TX on OpenSegment
 Pin 8 on Uno to RX on OpenSegment
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

#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); //RX, TX

#define DISPLAY_SIZE 4

char tempFrame[DISPLAY_SIZE]; //This assumes we are attached to a 4 digit display

char tempString[100]; //Used for the sprintf based debug statements

int cycles = 3;

void setup() {

  Serial.begin(9600);
  Serial.println("OpenSegment Example Code");
  
  mySerial.begin(9600); //Talk to the OpenSegment at 9600 bps
  mySerial.print("0"); //These two lines help get the soft serial running correctly
  delay(10); //These two lines help get the soft serial running correctly
  //mySerial.print("0000");
}

void loop() {
  Serial.print("Cycle: ");
  Serial.println(cycles++);

  Serial.println("Sending new data");
  serialTestSend(cycles); //Sends a series of characters
  delay(500);

  serialRequestData(); //What is the display currently displaying?
  delay(500);

  serialSetBrightness(5);
  delay(500);

  serialRequestSettings(); //What are the current settings?
  delay(500);
}

//Sends a four digit value (cycles) to the display
void serialTestSend(int tempCycles) {
  mySerial.write('\n'); //This forces the cursor to return to the beginning of the display

  if(tempCycles / 1000 > 0)
    mySerial.print(tempCycles / 1000);
  else
    mySerial.print(' '); //This removes any leading zeros
  tempCycles %= 1000;

  if(tempCycles / 100 > 0)
    mySerial.print(tempCycles / 100);
  else
    mySerial.print(' '); //This removes any leading zeros
  tempCycles %= 100;

  if(tempCycles / 10 > 0)
    mySerial.print(tempCycles / 10);
  else
    mySerial.print(' '); //This removes any leading zeros
  tempCycles %= 10;

  mySerial.print(tempCycles);
}

//Gets the current settings from OpenSegment
void serialRequestData(void) {

  Serial.print("Visible Data: ");

  mySerial.write('\n'); //This forces the receive frame to reset allowing for commands to be read correctly
  
  while(mySerial.available()) mySerial.read(); //Get rid of anything setting in the incoming buffer

  mySerial.write(0x5C); //Sending 0x5C will give us visible data. 0x5E will give us the unit's settings.

  while(mySerial.available() < 4) ; //Wait for the characters to show up
  //delay(10); //Wait for the characters to show up

  //Collect the incoming 4 bytes
  for(int x = 0 ; x < 4 ; x++)
    tempFrame[x] = mySerial.read();
  
  //Display the data
  for(int x = 0 ; x < 4 ; x++) {
    sprintf(tempString, "0x%02X ", tempFrame[x]);
    Serial.print(tempString);
  }
  Serial.println();
}

//Sets the brightness (100 is pretty bright, 0 is brightest)
void serialSetBrightness(byte brightLevel) {

  mySerial.write('\n'); //This forces the receive frame to reset allowing for commands to be read correctly
  mySerial.write(0x7A); //Sending 0x7A will adjust the brightness level
  mySerial.write(brightLevel); //Set brightness
}

//Gets the current settings from OpenSegment
void serialRequestSettings(void) {
  
  Serial.print("Settings: ");

  mySerial.write('\n'); //This forces the receive frame to reset allowing for commands to be read correctly
  
  while(mySerial.available()) mySerial.read(); //Get rid of anything setting in the incoming buffer

  mySerial.write(0x5E); //Sending 0x5C will give us visible data. 0x5E will give us the unit's settings.

  while(mySerial.available() < 3) ; //Wait for the characters to show up
  //delay(10); //Wait for the characters to show up

  //Collect the incoming 3 bytes
  for(int x = 0 ; x < 3 ; x++)
    tempFrame[x] = mySerial.read();
  
  //Display the data
  for(int x = 0 ; x < 3 ; x++) {
    sprintf(tempString, "0x%02X ", tempFrame[x]);
    Serial.print(tempString);
  }
  Serial.println();
}


