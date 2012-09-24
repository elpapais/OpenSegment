/*
 9-19-2012
 Spark Fun Electronics
 Nathan Seidle
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 OpenSegment is an open source seven segment display. 
 
 This is the I2C testing of the communication.
 
 ToDo:
 Get I2C address to read the solder jumper settings
 Get UART baud rate from EEPROM
 
 */

#include <Wire.h>
#include <EEPROM.h>

//This code was written for a 4 digit display but should work for other sizes as well
#define DISPLAYSIZE 4

//This turns on a bunch of print statements
//Comment out for production
#define DEBUG

//Internal EEPROM locations for the user settings
#define LOCATION_BAUD_SETTING		0x01
#define LOCATION_BRIGHTNESS_SETTING	0x02
#define LOCATION_ADDRESS_SETTING	0x03

#define BAUD_2400	0
#define BAUD_4800	1
#define BAUD_9600	2
#define BAUD_19200	3
#define BAUD_38400	4
#define BAUD_57600	5
#define BAUD_115200	6

#define MODE_UART	0
#define MODE_SPI	1
#define MODE_I2C	2
#define MODE_COUNTER	3

#define CMD_RESET_DISPLAY	0x76 //'v'
#define CMD_BAUD_CONTROL	0x7F //DEL
#define CMD_BRIGHTNESS_CONTROL	0x7A //'z'
#define CMD_DPOINT_CONTROL	0x77 //'w'
#define CMD_DIGIT1_CONTROL	0x7B //'{'
#define CMD_DIGIT2_CONTROL	0x7C //'|'
#define CMD_DIGIT3_CONTROL	0x7D //'}'
#define CMD_DIGIT4_CONTROL	0x7E //'~'
#define CMD_REQ_VISIBLE		0x5C //'\'
#define CMD_REQ_SETTINGS	0x5E //'^'
#define CMD_RETURN		0x0D //'\r' This is carrage return \r
#define CMD_NEWLINE		0x0A //'\n' This is new line \n

#define DATA_VISIBLE		0
#define DATA_SETTINGS		1

#define COMM_UART		0
#define COMM_SPI		1
#define COMM_I2C		2
#define COMM_COUNTER		3


//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
char tempString[100]; //Used for the sprintf based debug statements

int frameSpot; //Keeps track of which digit should receive the byte being received
byte incomingFrame[DISPLAYSIZE];
byte visibleFrame[DISPLAYSIZE];

byte settingUartSpeed;
byte settingBrightness;
byte settingCommunication; //Controls unit's communcation type (I2C, Serial, SPI, counter)
byte settingAddress; //This is the I2C address for this device, default is 0x3C.
byte thingToSend; //Controls what to respond with, either visible data or the unit's settings

boolean byteReceived = false;
boolean byteRequested = false;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup() {
  //check_emergency_reset(); //Look to see if the RX pin is being pulled low

  readSystemSettings(); //Load all the system settings from EEPROM

  Wire.begin(settingAddress); //Join the bus and respond to calls addressed to 0x3C
  Wire.onReceive(receiveEvent); //receiveEvent gets called when data is sent to the display
  Wire.onRequest(requestEvent); //requestEvent gets called when data is requested from the display

  //Setup UART
  if(settingUartSpeed == BAUD_2400) Serial.begin(2400);
  if(settingUartSpeed == BAUD_4800) Serial.begin(4800);
  if(settingUartSpeed == BAUD_9600) Serial.begin(9600);
  if(settingUartSpeed == BAUD_19200) Serial.begin(19200);
  if(settingUartSpeed == BAUD_38400) Serial.begin(38400);
  if(settingUartSpeed == BAUD_57600) Serial.begin(57600);
  if(settingUartSpeed == BAUD_115200) Serial.begin(115200);

  Serial.begin(9600);

  //Clean out the frames
  resetVisibleFrame();
  resetIncomingFrame();
  
  //settingCommunication = COMM_I2C; //For this testing sketch let's use I2C as default communication type
  settingCommunication = COMM_UART; //For this testing sketch let's use I2C as default communication type

#ifdef DEBUG
  Serial.println("OpenSegment online! Yay!");
#endif  

}

void loop() {
  displayFrame(); //Illuminate the current frame
  delay(50);
  
  if(byteRequested == true) {
    //Serial.println("Tx");
    byteRequested = false;
  }
  if(byteReceived == true) {
    //Serial.println("Rx");
    byteReceived = false;
  }
}

//This displays the current digits and letters on the seven segment display
void displayFrame(void) {

  /*
  Serial.print(" Visible frame: ");
  for(int x = 0 ; x < DISPLAYSIZE ; x++) {
    sprintf(tempString, "0x%02X ", visibleFrame[x]);
    Serial.print(tempString);
  }
  Serial.println();
  */

}

//Takes an incoming character and records it to the data frame
void recordToFrame(byte theNewThang) {
  incomingFrame[frameSpot++] = theNewThang; //Store the incoming data

  checkFrame(); //Check for command bytes in the frame. If found, framespot is reset

  if(frameSpot >= DISPLAYSIZE) {
    frameSpot = 0; //Wrap the frame spot
    
    //If we've received a full display of data, record it to the visible frame
    for(int x = 0 ; x < DISPLAYSIZE ; x++)
      visibleFrame[x] = incomingFrame[x];
  }
}

//Checks the incoming frame for command bytes
void checkFrame(void) {

  //Look for reset command
  for(int x = 0 ; x < DISPLAYSIZE ; x++) {
    if(incomingFrame[x] == CMD_RESET_DISPLAY) {
      resetIncomingFrame(); //Clears the incoming frame
      resetVisibleFrame(); //Clears the display
      frameSpot = 0;
      return;
    }
    else if(incomingFrame[x] == CMD_NEWLINE || incomingFrame[x] == CMD_RETURN) {
      resetIncomingFrame();
      frameSpot = 0; //Do nothing but reset the frame spot
      return;
    }
  }

  //Data request for the visible frame
  if(incomingFrame[0] == CMD_REQ_VISIBLE) {
    if(settingCommunication == COMM_UART) {
      for(int x = 0 ; x < DISPLAYSIZE ; x++)
        Serial.write(visibleFrame[x]);
      
      Serial.write('\n'); //New line termination

      byteRequested = true;
    }
    else if(settingCommunication == COMM_I2C) {
      thingToSend = DATA_VISIBLE; //The parent is requesting that we push the visible frame
      //The actual values are sent from the requestEvent() function
    }

    frameSpot = 0;
    return;
  }

  //Data request for the unit settings
  if(incomingFrame[0] == CMD_REQ_SETTINGS) {
    if(settingCommunication == COMM_UART) {
      Serial.write(settingUartSpeed);
      Serial.write(settingBrightness);
      Serial.write(settingCommunication);
      Serial.write('\n'); //New line termination

      byteRequested = true;
    }
    else if(settingCommunication == COMM_I2C) {
      thingToSend = DATA_SETTINGS; //The parent is requesting that we push the unit's settings
      //The actual values are sent from the requestEvent() function
    }

    frameSpot = 0;
    return;
  }

  //All the following commands require a minimum of two bytes in the incoming frame
  if(frameSpot < 2) return;

  //Baud rate change
  if(incomingFrame[0] == CMD_BAUD_CONTROL) {
    switch(incomingFrame[1]) {
    case BAUD_2400:
      settingUartSpeed = BAUD_2400;
      break;
    case BAUD_4800:
      settingUartSpeed = BAUD_4800;
      break;
    case BAUD_9600:
      settingUartSpeed = BAUD_9600;
      break;
    case BAUD_19200:
      settingUartSpeed = BAUD_19200;
      break;
    case BAUD_38400:
      settingUartSpeed = BAUD_38400;
      break;
    case BAUD_57600:
      settingUartSpeed = BAUD_57600;
      break;
    case BAUD_115200:
      settingUartSpeed = BAUD_115200;
      break;
    default:
      settingUartSpeed = BAUD_9600;
    }

    //Record this new baudrate to EEPROM
    EEPROM.write(LOCATION_BAUD_SETTING, settingUartSpeed);
    
    frameSpot = 0; //Reset the incoming frame
    return;
  }

  //Brightness change
  if(incomingFrame[0] == CMD_BRIGHTNESS_CONTROL) {
    settingBrightness = incomingFrame[1];
    if(settingBrightness == 255) settingBrightness = 0; //Sanity check

    //Record this new brightness to EEPROM
    EEPROM.write(LOCATION_BRIGHTNESS_SETTING, settingBrightness);
    
    frameSpot = 0; //Reset the incoming frame
    return;
  }
}

//Reset all data in the data/incoming frame to 'x' (nothing)
//Reset the spot variable
void resetIncomingFrame(void) {
  for(int x = 0 ; x < DISPLAYSIZE ; x++)
    incomingFrame[x] = 'x';

  frameSpot = 0;
}

//Reset all data in the visible frame to 'x' (nothing)
//Reset the spot variable
void resetVisibleFrame(void) {
  for(int x = 0 ; x < DISPLAYSIZE ; x++)
    visibleFrame[x] = 'x';

  frameSpot = 0;
}

//Serial Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void serialEvent() {

  //Check to see if the user is changing communication methods
  if(settingCommunication != COMM_UART) {
    settingCommunication = COMM_UART;
    frameSpot = 0; //Reset our spot within the frame just to be safe
    Serial.println("$");
  }
  char tempFrame[10];

  int x = 0;
  while(Serial.available())
    tempFrame[x++] = Serial.read();
  
  //Once we have a pause, move the frame over
  for(int j = 0; j < x ; j++)
    recordToFrame(tempFrame[j]);

  byteReceived = true;
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//I2C Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//receiveEvent gets called when data is sent to the display
void receiveEvent(int howMany) {

  //Check to see if the user is changing communication methods
  if(settingCommunication != COMM_I2C) {
    settingCommunication = COMM_I2C;
    frameSpot = 0; //Reset our spot within the frame just to be safe
  }

  while(Wire.available())
    recordToFrame(Wire.read());

  byteReceived = true;
}

//requestEvent gets called when data is requested from the display
//Respond with the data of choice
void requestEvent(void) {

  if(thingToSend == DATA_VISIBLE) {
    Wire.write(visibleFrame, DISPLAYSIZE); //Send the current frame back to the requester
  }
  else if(thingToSend == DATA_SETTINGS) {
    //Send all the units settings
    //Wire.write(settingUartSpeed);
    //Wire.write(settingBrightness);
    //Weird lesson learned here: You can't send individual byte toeh Wire.write, you have to send an array.

    byte tempFrame[3];
    tempFrame[0] = settingUartSpeed;
    tempFrame[1] = settingBrightness;
    tempFrame[2] = settingCommunication;
    Wire.write(tempFrame, 3);
  }
  
  byteRequested = true;
}


//End I2C Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//These are internal system functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void) {
  //Read what the current UART speed is from EEPROM memory
  //Default is 9600
  settingUartSpeed = EEPROM.read(LOCATION_BAUD_SETTING);
  if(settingUartSpeed > BAUD_115200) {
    settingUartSpeed = BAUD_9600; //Reset UART to 9600 if there is no speed stored
    EEPROM.write(LOCATION_BAUD_SETTING, settingUartSpeed);
  }

  //Determine the display brightness
  //Default is max brightness (0)
  settingBrightness = EEPROM.read(LOCATION_BRIGHTNESS_SETTING);
  if(settingBrightness == 255) {
    settingBrightness = 0; //By default, unit will be brightest
    EEPROM.write(LOCATION_BRIGHTNESS_SETTING, settingBrightness);
  }

  //Look up the address for this device
  //The OpenSegment I2C address can be anything really (but we should probably be following some convention)
  //Remember, this is only 7 bits. The 8th bit is the read/write flag
  //Also, the lest two bits are controlled by the solder jumpers on the board
  //Default is 0x3C
  settingAddress = EEPROM.read(LOCATION_ADDRESS_SETTING);
  if(settingAddress == 255) {
    settingAddress = 0x3C; //By default, unit's address is 0x3C
    EEPROM.write(LOCATION_ADDRESS_SETTING, settingAddress);
  }

}

//End internal system functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


