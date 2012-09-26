/*
 9-23-2012
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 OpenSegment is an open source seven segment display. 

 This is example code that shows how to control OpenSegment via SPI.
 
 To get this code to work, attached an OpenSegment to an Arduino Uno using the following pins:
 Pin 10 on Uno (CS) to CS on OpenSegment
 Pin 11 to MOSI
 Pin 12 to MISO
 Pin 13 to SCK
 VIN to PWR
 GND to GND

*/

#include <SPI.h>

int csPin = 10; //You can use any IO pin but for this example we use 10

#define DISPLAY_SIZE 4

char tempFrame[DISPLAY_SIZE]; //This assumes we are attached to a 4 digit display

char tempString[100]; //Used for the sprintf based debug statements

int cycles = 1;

void setup() {
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH); //By default, don't be selecting OpenSegment
  
  Serial.begin(9600);
  Serial.println("OpenSegment Example Code");
  
  SPI.begin(); //Start the SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV8); //Slow down the master a bit
}

void loop() {
  Serial.print("Cycle: ");
  Serial.println(cycles++);

  Serial.println("Sending new data");
  spiTestSend(cycles); //Sends a series of characters
  delay(500);

  //serialRequestData(); //What is the display currently displaying?
  //delay(500);

  //serialSetBrightness(5);
  //delay(500);

  //serialRequestSettings(); //What are the current settings?
  //delay(500);
}

//Given a 4 digit number this function breaks up the number and loads it into
//tempFrame for easier sending over various protocols
//Does not print leading zeros
//Correctly prints negative numbers
void splitValue(int value) {
  boolean negative = false;
  int spot;
  
  if(value < 0) {
   value *= -1; //Get rid of minus sign for now
   negative = true;
  }

  //Here we split up and load the array with the four numbers
  for(spot = 0 ; spot < 4 ; spot++) {
    tempFrame[3 - spot] = (value % 10) + '0'; // = '2'
    value /= 10; // = 145
  }

  //Use this if you do not want to print leading zeros
  //We run until spot == 3 so that we print a zero if number we want to print is just 0
  for(spot = 0 ; tempFrame[spot] == '0' && spot < 3 ; spot++) //Spin through the leading zeros
    tempFrame[spot] = ' ';
  
  //Attach a negative sign if this number is less than 0 and less than 4 digits
  if(negative == true && spot > 0) tempFrame[spot - 1] = '-';

  //Return with tempFrame loaded up and ready to print
}

void spiTestSend(int toSend) {
  splitValue(toSend); //Divy up this number into sendable ASCII bytes

  digitalWrite(csPin, LOW); //Drive the CS pin low to select OpenSegment

  //SPI.transfer('\n'); //This forces the cursor to return to the beginning of the display
  
  //Push the array
  for(int x = 0 ; x < 4 ; x++) {
    delayMicroseconds(1);
    SPI.transfer(tempFrame[x]);
  }
  
  digitalWrite(csPin, HIGH); //Release the CS pin to de-select OpenSegment
}
/*
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
*/

