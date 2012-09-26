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
#define LOCATION_I2CADDRESS_SETTING	0x03

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

//For digits with NPN control, off is digital low
#define DIG_OFF			LOW
#define DIG_ON			HIGH

//For segments with PNP control, off is digital high
#define SEG_OFF			HIGH
#define SEG_ON			LOW

//Define all the hardware connections
#define SEGA			14
#define SEGB			2
#define SEGC			8
#define SEGD			6
#define SEGE			7
#define SEGF			15
#define SEGG			4
#define SEGDP			5

#define DIG1			9
#define DIG2			16
#define DIG3			17
#define DIG4			3

#define SPI_CS			10
#define SPI_MOSI		11
#define SPI_MISO		12
#define SPI_SCK			13

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
char tempString[100]; //Used for the sprintf based debug statements

int frameSpot; //Keeps track of which digit should receive the byte being received
byte incomingFrame[DISPLAYSIZE];
byte visibleFrame[DISPLAYSIZE];

byte settingUartSpeed;
byte settingBrightness;
byte settingCommunication; //Controls unit's communcation type (I2C, Serial, SPI, counter)
byte settingI2CAddress; //This is the I2C address for this device, default is 0x3C.
byte thingToSend; //Controls what to respond with, either visible data or the unit's settings

boolean UARTbyteReceived = false; //These variables track when the UART interrupt is called
boolean UARTbyteRequested = false;

boolean I2CbyteReceived = false; //These variables track when the I2C interrupt is called
boolean I2CbyteRequested = false;

/*char SPIinBuffer[16]; //We should only need about 10 spots
 byte SPIin_head = 0;
 byte SPIin_tail = 0;
 char SPIoutBuffer[16]; //We should only need about 10 spots
 byte SPIout_head = 0;
 byte SPIout_tail = 0;*/

//byte SPIoutgoing_spot = 0;
byte SPIhead = 0;
byte SPItail = 0;
byte SPIbuffer[16]; //We should only need about 10 spots
boolean SPIframeReceived = false;

//byte SPIcommand = 0; //This is the first byte that the master sends us
//boolean SPIbyteReceived = false; //This variable tracks when the SPI interrupt is called
int myCounter;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup() {

  //Setup all the digit control
  digitalWrite(DIG1, DIG_OFF);
  digitalWrite(DIG2, DIG_OFF);
  digitalWrite(DIG3, DIG_OFF);
  digitalWrite(DIG4, DIG_OFF);
  pinMode(DIG1, OUTPUT);
  pinMode(DIG2, OUTPUT);
  pinMode(DIG3, OUTPUT);
  pinMode(DIG4, OUTPUT);

  digitalWrite(SEGA, SEG_OFF);
  digitalWrite(SEGB, SEG_OFF);
  digitalWrite(SEGC, SEG_OFF);
  digitalWrite(SEGD, SEG_OFF);
  digitalWrite(SEGE, SEG_OFF);
  digitalWrite(SEGF, SEG_OFF);
  digitalWrite(SEGG, SEG_OFF);
  digitalWrite(SEGDP, SEG_OFF);
  pinMode(SEGA, OUTPUT);
  pinMode(SEGB, OUTPUT);
  pinMode(SEGC, OUTPUT);
  pinMode(SEGD, OUTPUT);
  pinMode(SEGE, OUTPUT);
  pinMode(SEGF, OUTPUT);
  pinMode(SEGG, OUTPUT);
  pinMode(SEGDP, OUTPUT);

  checkEmergencyReset(); //Look to see if the RX pin is being pulled low

  readSystemSettings(); //Load all the system settings from EEPROM
//  settingCommunication = COMM_UART; //On power up unit assumes UART communication
  settingCommunication = COMM_SPI;

  //Setup I2C
  Wire.begin(settingI2CAddress); //Join the bus and respond to calls addressed to 0x3C
  Wire.onReceive(I2CreceiveEvent); //receiveEvent gets called when data is sent to the display
  Wire.onRequest(I2CrequestEvent); //requestEvent gets called when data is requested from the display

  //Setup SPI - remember, we can't use the built-in library for slave mode
  pinMode(SPI_CS, INPUT);
  pinMode(SPI_MISO, OUTPUT);
  pinMode(SPI_MOSI, INPUT);
  pinMode(SPI_SCK, INPUT);
  SPCR |= _BV(SPE); //Turn on SPI in slave mode
  SPCR |= _BV(SPIE); //Turn on SPI interrupts

  //Setup UART
  if(settingUartSpeed == BAUD_2400) Serial.begin(2400);
  if(settingUartSpeed == BAUD_4800) Serial.begin(4800);
  if(settingUartSpeed == BAUD_9600) Serial.begin(9600);
  if(settingUartSpeed == BAUD_19200) Serial.begin(19200);
  if(settingUartSpeed == BAUD_38400) Serial.begin(38400);
  if(settingUartSpeed == BAUD_57600) Serial.begin(57600);
  if(settingUartSpeed == BAUD_115200) Serial.begin(115200);

  //Clean out the frames
  resetVisibleFrame();
  resetIncomingFrame();

#ifdef DEBUG
  Serial.println("OpenSegment online! Yay!");
#endif  

  myCounter = 0;
}

void loop() {
  int myCounter;
  //displayFrame(); //Illuminate the current frame
  //delay(50);

  if(settingCommunication == COMM_I2C) { //If we are communicating via I2C
    if(I2CbyteRequested == true) {
      //Serial.println("Tx");
      I2CbyteRequested = false;
    }
    if(I2CbyteReceived == true) {
      //Serial.println("Rx");
      I2CbyteReceived = false;
    }
  }

  if(settingCommunication == COMM_SPI) { //If we are communicating via SPI
    //if(digitalRead(SPI_CS) == HIGH) SPIspot = 0; //If SPI is not active, reset our spot in the frame

    //if(SPIframeReceived == true) { //Record the SPI frame
      //SPIspot = 0;

      while(SPItail != SPIhead) {
        recordToFrame(SPIbuffer[SPItail++]); //Push this byte of the buffer to the incomingFrame
        SPItail %= 16; //Wrap the tail if need be
      }

      //SPIframeReceived = false;

      //For debugging
      //for(int x = 0 ; x < DISPLAYSIZE ; x++)
      //  Serial.write(SPIbuffer[x]);
    //}
  }
}

//The Arduino SPI library doesn't support slave mode so we need our own ISR (interrupt service routine)
//Originally from http://www.gammon.com.au/forum/?id=10892
//SPI interrupt routine
ISR (SPI_STC_vect) {
  SPIbuffer[SPIhead++] = SPDR; //Load the incoming character into the SPI buffer
  SPIhead %= 16;

  /*if(SPIspot >= DISPLAYSIZE){
    SPIframeReceived = true; //Let the main loop know we've got a complete frame
    SPIspot = 0; //Wrap the SPIspot just in case
  }*/
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

      //For debugging
      for(int x = 0 ; x < DISPLAYSIZE ; x++)
        Serial.write(visibleFrame[x]);
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
      //You could potentially have brightness control in inFrame[0] and brightness setting of 10 (10 = \n)
      if(incomingFrame[0] != CMD_BRIGHTNESS_CONTROL) { //This is a super weird case where we avoid a false reset
        resetIncomingFrame();
        frameSpot = 0; //Do nothing but reset the frame spot
        return;
      }
    }
  }

  //Data request for the visible frame
  if(incomingFrame[0] == CMD_REQ_VISIBLE) {
    if(settingCommunication == COMM_UART) {
      for(int x = 0 ; x < DISPLAYSIZE ; x++)
        Serial.write(visibleFrame[x]);

      Serial.write('\n'); //New line termination

      UARTbyteRequested = true;
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

      UARTbyteRequested = true;
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
  }
  char tempFrame[10];

  int x = 0;
  while(Serial.available())
    tempFrame[x++] = Serial.read();

  //Once we have a pause, move the frame over
  for(int j = 0; j < x ; j++)
    recordToFrame(tempFrame[j]);

  UARTbyteReceived = true;
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//SPI Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//I2C Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//receiveEvent gets called when data is sent to the display
void I2CreceiveEvent(int howMany) {

  //Check to see if the user is changing communication methods
  if(settingCommunication != COMM_I2C) {
    settingCommunication = COMM_I2C;
    frameSpot = 0; //Reset our spot within the frame just to be safe
  }

  while(Wire.available())
    recordToFrame(Wire.read());

  I2CbyteReceived = true;
}

//requestEvent gets called when data is requested from the display
//Respond with the data of choice
void I2CrequestEvent(void) {

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

  I2CbyteRequested = true;
}
//End I2C Specific functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//These are internal system functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Check to see if we need an emergency reset
//Scan the RX pin for 2 seconds
//If it's low the entire time, then resort to factory defaults
void checkEmergencyReset(void) {
  pinMode(0, INPUT); //Turn the RX pin into an input
  digitalWrite(0, HIGH); //Push a 1 onto RX pin to enable internal pull-up

  //Quick pin check
  if(digitalRead(0) == HIGH) return;

  //Wait 2 seconds, blinking LEDs while we wait
  digitalWrite(SEGDP, SEG_ON); //Turn on the decimal point
  digitalWrite(DIG1, DIG_ON); //Turn on the digits
  digitalWrite(DIG2, DIG_ON); //Turn on the digits
  digitalWrite(DIG3, DIG_ON); //Turn on the digits
  digitalWrite(DIG4, DIG_ON); //Turn on the digits

  for(byte i = 0 ; i < 40 ; i++) {
    delay(25);
    digitalWrite(SEGDP, SEG_OFF); //Turn off the decimal points
    if(digitalRead(0) == HIGH) return; //Check to see if RX is not low anymore

    delay(25);
    digitalWrite(SEGDP, SEG_ON); //Turn on the decimal points
    if(digitalRead(0) == HIGH) return; //Check to see if RX is not low anymore
  }		

  //If we make it here, then RX pin stayed low the whole time
  setDefaultSettings(); //Reset baud, brightness

    //Now sit in forever loop indicating system is now at 9600bps
  while(1) {
    delay(500);
    digitalWrite(SEGDP, SEG_ON); //Turn on the decimal points
    delay(500);
    digitalWrite(SEGDP, SEG_OFF); //Turn off the decimal points
  }
}


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
  settingI2CAddress = EEPROM.read(LOCATION_I2CADDRESS_SETTING);
  if(settingI2CAddress == 255) {
    settingI2CAddress = 0x3C; //By default, unit's address is 0x3C
    EEPROM.write(LOCATION_I2CADDRESS_SETTING, settingI2CAddress);
  }

}

//Resets all the system settings to safe values
void setDefaultSettings(void) {
  //Reset UART to 9600bps
  EEPROM.write(LOCATION_BAUD_SETTING, BAUD_9600);

  //Reset system brightness to the brightest level
  EEPROM.write(LOCATION_BRIGHTNESS_SETTING, 0);

  //Reset the I2C address to the default 0x3C
  EEPROM.write(LOCATION_I2CADDRESS_SETTING, 0x3C);
}

//End internal system functions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=




