//!
//! @file    LuxcityUvTonicControlSystem.ino
//! @author  Geoffrey Hunter <gbmhunter@gmail.com> (www.cladlab.com)
//! @date    03/10/2012
//! @brief   Control sequencer interface between Vixen software and
//!          Freetronics relay control shields.
//! @details
//!    <b>Last Modified:         </b> 27/10/2012       \n
//!    <b>Version:               </b> v1.0.0           \n
//!    <b>Company:               </b> CladLabs         \n
//!    <b>Project:               </b> Luxcity Tonic Control System \n
//!    <b>Language:              </b> C                \n
//!    <b>Compiler:              </b> GCC              \n
//!    <b>uC Model:              </b> Arduino Uno      \n
//!    <b>Computer Architecture: </b> ATMEGA           \n
//!    <b>Operating System:      </b> none             \n
//!    <b>License:               </b> GPLv3            \n
//!    <b>Documentation Format:  </b> Doxygen          \n
//!
//!  Vixen must be confired with the 'Generic Serial' output, at 56700 baud, and to send the 
//!  end character 'n'. Vixen must have 64 channels. Firmware will turn channel x relay on when
//!  the Vixen intensity for that channel is set to 255, otherwise off.
 
//===============================================================================================//
//========================================== INCLUDES ===========================================//
//===============================================================================================//
 
#include "Wire.h"

#include <LiquidCrystal.h>

//===============================================================================================//
//========================================== DEFINES ============================================//
//===============================================================================================//

LiquidCrystal lcd( 8, 9, 4, 5, 6, 7 );

#define UART_BAUD_RATE 57600          //!< Baud rate of serial between Arduino and computer
                                      //!< (configure Vixen Generic Serial output to same baud rate)

#define END_OF_MSG_CHAR 'n'           //!< The end of message character. This is set-up in
                                      //!< output serial plugin in Vixen
#define NUM_RELAY_CONTROL_BOARDS 8    //!< Number of control boards
#define NUM_CHANNELS 64               //!< Number of control channels

//===============================================================================================//
//=================================== PRIVATE TYPEDEFS ==========================================//
//===============================================================================================//

//! Enumeration of the 8 relay boards
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

//===============================================================================================//
//====================================== PRIVATE VARIABLES ======================================//
//===============================================================================================//

//! Array to hold the I2C addresses of the relay control boards
//! These are set-up using the PCB jumpers on the board themselves.
//! Up to 8-different addresses settable (3-bit)
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

uint8_t inputString[200] = {0};         //!< A string to hold incoming data
boolean stringComplete = false;         //!< Whether the string is complete
boolean _pass = true;                   //!< Set to false if CheckForConflict() finds any invalid
                                        //!< relay states

//===============================================================================================//
//=================================== FUNCTION PROTOTYPES =======================================//
//===============================================================================================//

void SetRelayStates(uint8_t relayDriverShieldNum, uint8_t relayStates);

//===============================================================================================//
//===================================== PRIVATE FUNCTIONS =======================================//
//===============================================================================================//

//! @brief    Setup (init) function
void setup() {
  
  // Initialize serial
  Serial.begin(UART_BAUD_RATE);
  //Serial.write("Test");
  
  // Setup LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Tonic Sequencer");
  lcd.setCursor(0, 1);
  lcd.print("S: Idle");
  
   // Setup I2C
  Wire.begin();
  uint8_t i = 0;
  for(i = 0; i < 8; i++)
  {
    /* This is commented out since the Freetronics example
    code didn't seem to do the right thing (this turned on
    relay 6).
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

//! @brief    Main loop
void loop() 
{
  //! Used to determine wether firmware should display idle or active on screen
  //! (by way of timeout)
  static uint32_t elaspedTimeLastMsgMs = 0;
  
  // Take action if new string has arrived (looks for END_OF_MSG_CHAR)
  if (stringComplete) 
  {
    // Process string
    int i;
    
    // Start with board 1
    uint8_t relayDriverShieldNum = RELAY_BOARD_1;
    uint8_t relayNum = 0;
    uint8_t relayStates = 0;
    uint8_t numRelaysOn = 0;
    _pass = true;
    
    char txBuff[100];
    
    for(i = 0; i < NUM_CHANNELS; i++)
    {
          //snprintf(txBuff, sizeof(txBuff), "Val = %u\r\n", inputString[i]);
          //Serial.write(txBuff);
          // Check for 255 to turn relay on. If anything else, leave off
          if(inputString[i] == 255)
          {
            //digitalWrite(13, HIGH);
            // Set relay on
            relayStates = relayStates | (1 << relayNum);
            // Increment relay on counter
            numRelaysOn++;
          }
          
          // Check if setting the last relay on the board (8 per board)
          if(relayNum == 7)
          {
            relayStates = CheckForConflict(relayStates); 
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
    
    //Serial.println(inputString); 
    // Clear the string for the next message
    memset(inputString, 0x00, sizeof(inputString));
    //inputString = "";
    
    // Display state
    lcd.setCursor(0, 1);
    if(_pass == true)
    {
      lcd.print("S: Active");
    }
    else
    {
      lcd.print("S: Error ");
    }
    
    // Display how many relays are on in lower bottom-right corner
    
    // Hack for padding number with 0 if single-digit
    if(numRelaysOn <= 9)
    {
      lcd.setCursor(14, 1);
      lcd.print(0, DEC);
      lcd.setCursor(15, 1);
      lcd.print(numRelaysOn, DEC);
    }
    else
    {
      lcd.setCursor(14, 1);
      lcd.print(numRelaysOn, DEC);
    }
    
    // Reset timeout counter
    elaspedTimeLastMsgMs = millis();
    
    stringComplete = false;
  }
  
  // Check for active state timeout
  if(millis() > elaspedTimeLastMsgMs + 1500)
  {
    lcd.setCursor(0, 1);
    lcd.print("S: Idle  ");
  }
  
}

//! @brief    Checks for relay state conflicts
//! @details  A conflict occurs if both air and water relays
//!            for the same pipe are on at the same time.
uint8_t CheckForConflict(uint8_t relayStates)
{
  // Relay 1, 2 check
  if((relayStates & 0x01) && (relayStates & 0x02))
  {
    relayStates = relayStates & 0b11111100;
    _pass = false;
  }
  
  // Relay 3, 4 check
  if((relayStates & 0x04) && (relayStates & 0x08))
  {
    relayStates = relayStates & 0b11110011;
    _pass = false;
  }
  
  // Relay 5, 6 check
  if((relayStates & 0x10) && (relayStates & 0x20))
  {
    relayStates = relayStates & 0b11001111;
    _pass = false;
  }
  
  // Relay 7, 8 check
  if((relayStates & 0x40) && (relayStates & 0x80))
  {
    relayStates = relayStates & 0b00111111;
    _pass = false;
  }
  
  return relayStates;
  
}

//! @brief    Sends the I2C commands to control the 8 relays
//!            on one of the relay shields.
void SetRelayStates(uint8_t relayDriverShieldNum, uint8_t relayStates)
{
  Wire.beginTransmission(i2cAddresses[relayDriverShieldNum]);
  Wire.write(0x12);        // Select GPIOA
  Wire.write(relayStates);  // Send value to bank A
  Wire.endTransmission();
}

//! @brief  SerialEvent occurs whenever a new data comes in the
//!         hardware serial RX
//! @note   Called by interrupt
void serialEvent() {
  
  static uint8_t inputStringPos = 0;
  
  while (Serial.available())
  {
    // get the new byte:
    uint8_t inChar = (uint8_t)Serial.read();
   
    // add it to the inputString:
    inputString[inputStringPos] = inChar;
    inputStringPos++;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == END_OF_MSG_CHAR)
    {
      //digitalWrite(13, HIGH);
      stringComplete = true;
      inputStringPos = 0;
    } 
  }
}

// EOF
