#include <SPI.h>


// Variables
volatile boolean dataReady;
double VREF = 2.50;


// Pins setup
const byte CS_pin = PA4;
const byte DRDY_pin = PA2;
const byte RESET_pin = PA3;
// const byte PDWN_pin = PA1; or Permanentle tied to 3.3v


// Registers
uint8_t registerAddress;
uint8_t registerValueR;
uint8_t registerValueW;
int32_t registerData;
uint8_t directCommand;
String PrintMessage;

byte outputBuffer[3];
byte differentialBuffer[12];
byte singleBuffer[24];


void setup() {
  
  Serial.begin(115200);
  delay(1000);
  initialize_ADS1256();
  delay(1000);
  reset_ADS1256();
  userDefaultRegisters();
  // printInstructions()
  attachInterrupt(digitalPin2Interrupt(PA2, checkDReady,FALLING));
  // FALLING - DRDY goes low
}

void loop() {
  if(Serial.available()>0){
    
    char commandCharacter = Serial.read();
    switch(commandCharacter){

      // case for reading the registers value
      case 'r':
        while(!Serial.available()) {
          registerAddres = Serial.parseInt();
        }
      break;

      // case for writing a register value
      case 'w':
        while(!Serial.available()) {
          registerAddres = Serial.parseInt();
          delay(100);
          writeRegister(registerAddres, registerValueW);
          delay(500); 
        }
      break;
      
      // Print a message to the serial terminal
      case 't':
        Serial.println("* Test message triggered by serial command")
      break;

      // Read a single value from the ADC
      case 'O':
        readSingle();
      break;

      case 'R':
      reset_ADS1256();
      break;

      // SDATAC - Stop Reading Data Continously
      case 's':
        SPI.transfer(B00001111);
      break;

      //Single channel continous reading - MUX is manual, can be single and differential too
      case 'A': 
        readSingleContinuous();
      break;

      //Single-ended mode cycling
      case 'C':
        cycleSingleEnded();
      break;

      //differential mode cycling
      case 'D':
        cycleDifferential();
      break;

      //direct command
      case 'd':
        while (!Serial.available());
        directCommand = Serial.parseInt();
        sendDirectCommand(directCommand);
      break;

      //Set everything back to default
      case 'U':
        userDefaultRegisters();
      break;
    }
  }
}


// Function for the ISR
void checkDReady() {
  dataReady = true;
}


// Function for reading selected register
unsigned long readRegister(uint8_t registerAddress) {
  
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  //SPI_MODE1 = output edge: rising, data capture: falling; clock polarity: 0, clock phase: 1.
  
  //CS must stay LOW during the entire sequence [Ref: P34, T24]
  digitalWrite(CS_pin, LOW);
  //0x10 = 0001000 = RREG - OR together the two numbers (command + address)
  SPI.transfer(0x10 | registerAddress);
  //2nd (empty) command byte
  SPI.transfer(0x00);
  //see t6 in the datasheet
  delayMicroseconds(5);
  //read out the register value
  registerValueR = SPI.transfer(0xFF);
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();

  return registerValueR;
}


// Function to write a register
void writeRegister(uint8_t registerAddress, uint8_t registerValueW){
  //SPI_MODE1 = output edge: rising, data capture: falling; clock polarity: 0, clock phase: 1.
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));

  //CS must stay LOW during the entire sequence [Ref: P34, T24]
  digitalWrite(CS_pin, LOW);
  //see t6 in the datasheet
  delayMicroseconds(5); 
  // 0x50 = 01010000 = WREG
  SPI.transfer(0x50 | registerAddress); 
  SPI.transfer(0x00);
  SPI.transfer(registerValueW);
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();
}