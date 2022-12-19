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
  // put your main code here, to run repeatedly:

}
