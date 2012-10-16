/*
  Serial Event example
 
 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and 
 clears it.
 
 Inputs from the free PC-based Vixen sequencer.
 
 Outputs to FreeTronics Relay Shields via I2C.
 Address ranges froms 0x20 to 0x27 depending in jumper settings
 J3 J2 J1  I2C Add
 0  0  0   0x20
 0  0  1   0x21
 0  1  0   0x22
 0  1  1   0x23
 1  0  0   0x24
 1  0  1   0x25
 1  1  0   0x26
 1  1  1   0x27
 
 The 8 relays per board are controlled by a single byte, with one bit each corresonding to a relay, starting from the LSB.
 Relays are connected to bank A.
 
 
 Created 9 May 2011
 by Tom Igoe
 
 This example code is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/SerialEvent
 
 */
 
#include "Wire.h"
 
#define END_OF_MSG_CHAR 'n'
#define NUM_CHANNELS 64          //!< Number of control channels
#define NUM_RELAY_CONTROL_BOARDS 8

typedef enum
{
  RELAY_BOARD_1,
  RELAY_BOARD_2,
  RELAY_BOARD_3,
  RELAY_BOARD_4,
  RELAY_BOARD_5,
  RELAY_BOARD_6,
  RELAY_BOARD_7,
  RELAY_BOARD_8
} relayDriverShieldNum_t;

//! Array to hold the I2C addresses of the relay control boards
uint8_t i2cAddresses[NUM_RELAY_CONTROL_BOARDS] = 
{
  0x20,
  0x21,
  0x22,
  0x23,
  0x24,
  0x25,
  0x26,
  0x27
};

void SetRelayStates(uint8_t relayDriverShieldNum, uint8_t relayStates);

uint8_t inputString[200] = {0};         //!< A string to hold incoming data
boolean stringComplete = false;  //!< Whether the string is complete

void setup() {
  // Initialize serial
  Serial.begin(57600);
  //Serial.write("Test");
  // Reserve bytes for the inputString (+1 for END_OF_MSG_CHAR)
  //inputString.reserve(NUM_CHANNELS + 1);
  
  Wire.begin(); // Wake up I2C bus
  uint8_t i = 0;
  for(i = 0; i < 8; i++)
  {
    /*
    // Set addressing style
    Wire.beginTransmission(i2cAddresses[i]);
    Wire.write(0x12);
    Wire.write(0x20); // use table 1.4 addressing
    Wire.endTransmission();
    */
    // Set I/O bank A to outputs
    Wire.beginTransmission(i2cAddresses[i]);
    Wire.write(0x00); // IODIRA register
    Wire.write(0x00); // Set all of bank A to outputs
    Wire.endTransmission();
  }
  
  //! @debug
  pinMode(13, OUTPUT);
}



void loop() 
{
  // print the string when a newline arrives:
  if (stringComplete) 
  {
    // Process string
    int i;
    
    // Start with board 1
    uint8_t relayDriverShieldNum = RELAY_BOARD_1;
    uint8_t relayNum = 0;
    uint8_t relayStates = 0;
    
    char txBuff[100];
    
    for(i = 0; i < NUM_CHANNELS; i++)
    {
          snprintf(txBuff, sizeof(txBuff), "Val = %u\r\n", inputString[i]);
          //Serial.write(txBuff);
          // Check for 255 to turn relay on. If anything else, leave off
          if(inputString[i] == 255)
          {
            //digitalWrite(13, HIGH);
            // Set relay on
            relayStates = relayStates | (1 << relayNum);
          }
          
          // Check if setting the last relay on the board (8 per board)
          if(relayNum == 7)
          {
            //digitalWrite(13, LOW);
            // Send change relay state command to correct board
            SetRelayStates(relayDriverShieldNum, relayStates);
            // Reset relayStates for next 8 values to process
            relayStates = 0;
            // Reset relay number
            relayNum = 0;
            // Move to next board
            relayDriverShieldNum = relayDriverShieldNum +1;
            
          }
          else
          {
            // Increment board relay number
            relayNum++;
          }
          
          
     }
     
      /*
      if(inputString[i] != 0)
      {
        digitalWrite(13, HIGH);
        if(i == 0)
        {
          sendValueToLatch(1); 
        }
      }
      else
      {
        digitalWrite(13, LOW);
        if(i == 0)
        {
          sendValueToLatch(0); 
        }
      }
      */
   
    
    //Serial.println(inputString); 
    // Clear the string for the next message
    memset(inputString, 0x00, sizeof(inputString));
    //inputString = "";
    stringComplete = false;
  }
}

void SetRelayStates(uint8_t relayDriverShieldNum, uint8_t relayStates)
{
  Wire.beginTransmission(i2cAddresses[relayDriverShieldNum]);
  Wire.write(0x12);        // Select GPIOA
  Wire.write(relayStates);  // Send value to bank A
  Wire.endTransmission();
}

/*
void sendValueToLatch(int latchValue)
{
  Wire.beginTransmission(i2cAddresses[0]);
  Wire.write(0x12);        // Select GPIOA
  Wire.write(latchValue);  // Send value to bank A
  Wire.endTransmission();
}
*/

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  static uint8_t inputStringPos = 0;
  while (Serial.available()) {
    // get the new byte:
    uint8_t inChar = (uint8_t)Serial.read();
   
    // add it to the inputString:
    inputString[inputStringPos] = inChar;
    inputStringPos++;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == END_OF_MSG_CHAR) {
      //digitalWrite(13, HIGH);
      stringComplete = true;
      inputStringPos = 0;
    } 
  }
}


