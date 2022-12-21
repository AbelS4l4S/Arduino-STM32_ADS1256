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
  if(Serial.available()>0) {
    
    char commandCharacter = Serial.read();
    switch(commandCharacter) {

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
void writeRegister(uint8_t registerAddress, uint8_t registerValueW) {
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


void reset_ADS1256() {
  
  // initialize SPI with  clock, MSB first, SPI Mode1
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1)); 

  digitalWrite(CS_pin, LOW);
  delayMicroseconds(7);
  //Reset command
  SPI.transfer(0xFE); 
  //Minimum 0.6ms required for Reset to finish.
  delay(2); 
  //Issue SDATAC (any 8-bit value would complete the RESET command)
  SPI.transfer(0x0F);
  delayMicroseconds(100);
  digitalWrite(CS_pin, HIGH);
  SPI.endTransaction();

}


//starting up the chip by making the necessary steps. This goes into the setup() later.
void initialize_ADS1256()	{
  //Chip select
  pinMode(CS_pin, OUTPUT); //Chip select is an output
  digitalWrite(CS_pin, LOW);

  //DRDY
  pinMode(DRDY_pin, INPUT);
  //Reset
  pinMode(RESET_pin, OUTPUT);
  //We do a manual chip reset on the ADS1256 - Datasheet Page 27/ RESET
  digitalWrite(RESET_pin, LOW);
  delay(100);
  digitalWrite(RESET_pin, HIGH); //RESET is set to high
  delay(500);

  SPI.begin();
}


//Reading a single value ONCE using the RDATA command
void readSingle() {
  registerData = 0; // every time we call this function, this should be 0 in the beginning!
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"

  while (dataReady == false) {
  //Wait for DRDY to go LOW
  } 
  SPI.transfer(B00000001); //Issue RDATA (0000 0001) command
  delayMicroseconds(7); //Wait t6 time (~6.51 us) REF: P34, FIG:30.

  //step out the data: MSB | mid-byte | LSB,
  registerData = SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated
  registerData <<= 8;					//MSB gets shifted LEFT by 8 bits
  registerData |= SPI.transfer(0x0F); //MSB | Mid-byte // '|=' compound bitwise OR operator
  registerData <<= 8;					//MSB | Mid-byte gets shifted LEFT by 8 bits
  registerData |= SPI.transfer(0x0F); //(MSB | Mid-byte) | LSB - final result
  //After this, DRDY should go HIGH automatically

  Serial.print("Raw data: ");
  Serial.println(registerData); //prints the raw data (24-bit number)
  Serial.print("Voltage: ");
  convertToVoltage(registerData); //prints the converted data (PGA should be 0!)

  digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
  SPI.endTransaction();
}


//Reads the recently selected channel using RDATAC
void readSingleContinuous() {
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"

  //These variables serve only testing purposes!
  //uint32_t loopcounter = 0;
  //StartTime = micros();
  //--------------------------------------------

  while (dataReady == false) {} //Wait until DRDY does low, then issue the command
  SPI.transfer(B00000011);  //Issue RDATAC (0000 0011) command after DRDY goes low
  delayMicroseconds(7); //Wait t6 time (~6.51 us) REF: P34, FIG:30.

  while (Serial.read() != 's') {
    //while (GPIOA->regs->IDR & 0x0004){} //direct port access to A2 (DRDY) pin - less reliable polling alternative
    while (dataReady == false) {} //waiting for the dataReady ISR
    //Reading a single input continuously using the RDATAC
    //step out the data: MSB | mid-byte | LSB
    outputBuffer[0] = SPI.transfer(0); // MSB comes in
    outputBuffer[1] = SPI.transfer(0); // Mid-byte
    outputBuffer[2] = SPI.transfer(0); // LSB - final conversion result
    //After this, DRDY should go HIGH automatically
    Serial.write(outputBuffer, sizeof(outputBuffer)); //this buffer is [3]
    dataReady = false; //reset dataReady manually

    /*
      //These variables only serve test purposes!
      loopcounter++;
      //if(micros() - StartTime >= 5000000) //5 s
      if(loopcounter >= 150000)
      {
             Serial.print(" Loops: ");
             Serial.println(loopcounter++);
             Serial.println(micros() - StartTime);
             break; //exit the whole thing
      }
    */
  }
  SPI.transfer(B00001111); //SDATAC stops the RDATAC - the received 's' just breaks the while(), this stops the acquisition
  digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
  SPI.endTransaction();
}


void cycleSingleEnded() {
  int cycle = 0;
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
  while (Serial.read() != 's') {
    for (cycle = 0; cycle < 8; cycle++) {
      //we cycle through all the 8 single-ended channels with the RDATAC
      //INFO:
      //RDATAC = B00000011
      //SYNC = B11111100
      //WAKEUP = B11111111
      //---------------------------------------------------------------------------------------------
      /*Some comments regarding the cycling:
        When we start the ADS1256, the preconfiguration already sets the MUX to [AIN0+AINCOM].
        When we start the RDATAC (this function), the default MUX ([AIN0+AINCOM]) will be included in the
        cycling which means that the first readout will be the [AIN0+AINCOM]. But, before we read the data
        from the [AIN0+AINCOM], we have to switch to the next register already, then start RDATA. This is
        demonstrated in Figure 19 on Page 21.

        Therefore, in order to get the 8 channels nicely read and formatted, we have to start the cycle
        with the 2nd input of the ADS1256 ([AIN1+AINCOM]) and finish with the first ([AIN0+AINCOM]).

         \ CH1 | CH2 CH3 CH4 CH5 CH6 CH7 CH8 \ CH1 | CH2 CH3 ...

        The switch-case is between the  two '|' characters
        The output (one line of values) is between the two '\' characters.
      */
      //-------------------------------------------------------------------------------------------
      //Steps are on Page 21 of the datasheet
      while (dataReady == false) {} //direct port access to A2 (DRDY) pin
      //Step 1. - Updating MUX
      switch (cycle)
      {
        //Channels are written manually
        case 0: //Channel 2
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00011000);  //AIN1+AINCOM
          break;

        case 1: //Channel 3
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00101000);  //AIN2+AINCOM
          break;

        case 2: //Channel 4
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00111000);  //AIN3+AINCOM
          break;

        case 3: //Channel 5
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01001000);  //AIN4+AINCOM
          break;

        case 4: //Channel 6
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01011000);  //AIN5+AINCOM
          break;

        case 5: //Channel 7
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01101000);  //AIN6+AINCOM
          break;

        case 6: //Channel 8
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01111000);  //AIN7+AINCOM
          break;

        case 7: //Channel 1
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00001000); //AIN0+AINCOM
          break;
      }
      //Step 2.
      SPI.transfer(B11111100); //SYNC
      delayMicroseconds(4); //t11 delay 24*tau = 3.125 us //delay should be larger, so we delay by 4 us
      SPI.transfer(B11111111); //WAKEUP

      //Step 3.
      //Issue RDATA (0000 0001) command
      SPI.transfer(B00000001);
      delayMicroseconds(7); //Wait t6 time (~6.51 us) REF: P34, FIG:30.

      //step out the data: MSB | mid-byte | LSB
      singleBuffer[(3 * cycle)] = SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated
      singleBuffer[(3 * cycle) + 1] = SPI.transfer(0x0F); //Mid-byte
      singleBuffer[(3 * cycle) + 2] = SPI.transfer(0x0F); //LSB - final result
      dataReady = false;
      //After this, DRDY should go HIGH automatically
    }
    //Dump the buffer after all 8 channels are read
    Serial.write(singleBuffer, 24);
  }
  SPI.transfer(B00001111); //SDATAC stops the RDATAC - the received 's' just breaks the while(), this stops the acquisition
  digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
  SPI.endTransaction();
}


void cycleDifferential() //APPROVED
{
  SPI.beginTransaction(SPISettings(1920000, MSBFIRST, SPI_MODE1));
  //Set the AIN0+AIN1 as inputs manually
  digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]
  SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
  SPI.transfer(0x00);
  SPI.transfer(B00000001);  //AIN0+AIN1
  digitalWrite(CS_pin, HIGH);
  delay(50);
  digitalWrite(CS_pin, LOW); //CS must stay LOW during the entire sequence [Ref: P34, T24]

  while (Serial.read() != 's')
  {
    for (int cycle = 0; cycle < 4; cycle++)
    {
      //Steps are on Page21
      //Step 1. - Updating MUX
      //DRDY has to go low
      while (dataReady == false) {}

      switch (cycle)
      {
        case 0: //Channel 2
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00100011);  //AIN2+AIN3
          break;

        case 1: //Channel 3
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01000101); //AIN4+AIN5
          break;

        case 2: //Channel 4
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B01100111); //AIN6+AIN7
          break;

        case 3: //Channel 1
          SPI.transfer(0x50 | 1); // 0x50 = WREG //1 = MUX
          SPI.transfer(0x00);
          SPI.transfer(B00000001); //AIN0+AIN1
          break;
      }

      SPI.transfer(B11111100); //SYNC
      delayMicroseconds(4); //t11 delay 24*tau = 3.125 us //delay should be larger, so we delay by 4 us
      SPI.transfer(B11111111); //WAKEUP

      //Step 3.
      SPI.transfer(B00000001); //Issue RDATA (0000 0001) command
      delayMicroseconds(7); //Wait t6 time (~6.51 us) REF: P34, FIG:30.

      differentialBuffer[(3 * cycle)] = SPI.transfer(0x0F); //MSB comes in, first 8 bit is updated // '|=' compound bitwise OR operator
      differentialBuffer[(3 * cycle) + 1] = SPI.transfer(0x0F); //Mid-byte
      differentialBuffer[(3 * cycle) + 2] = SPI.transfer(0x0F); //LSB - final result
      dataReady = false;
    }
    //Dump the buffer after all 4 channels are read
    Serial.write(differentialBuffer, 12);
  }
  SPI.transfer(B00001111); //SDATAC stops the RDATAC - the received 's' just breaks the while(), this stops the acquisition
  digitalWrite(CS_pin, HIGH); //We finished the command sequence, so we switch it back to HIGH
  SPI.endTransaction();
}

void sendDirectCommand(uint8_t directCommand)
{
  //Direct commands can be found in the datasheet Page 34, Table 24.
  SPI.beginTransaction(SPISettings(1700000, MSBFIRST, SPI_MODE1));

  digitalWrite(CS_pin, LOW); //REF: P34: "CS must stay low during the entire command sequence"
  delayMicroseconds(5);
  SPI.transfer(directCommand); //Send Command
  delayMicroseconds(5);
  digitalWrite(CS_pin, HIGH); //REF: P34: "CS must stay low during the entire command sequence"

  SPI.endTransaction();
}


void userDefaultRegisters()
{
  // This function is "manually" updating the values of the registers then reads them back.
  // This function can be used in the setup() after performing an initialization-reset process
  /*
  	REG   VAL     USE
  	0     54      Status Register, Everyting Is Default, Except Auto - Cal
  	1     1       Multiplexer Register, AIN0 POS, AIN1 POS
  	2     0       ADCON, Everything is OFF, PGA = 0
  	3     132      DataRate = 100 SPS
  */
  //We update the 4 registers that we are going to use

  delay(500);
  writeRegister(0x00, B00110110); //STATUS: bit1: bufen=1; bit2: acal=1; rest is not important or factory default
  delay(200);
  writeRegister(0x01, B00000001); //MUX AIN0+AIN1
  delay(200);
  writeRegister(0x02, B00000000); //ADCON - PGA = 0 (+/- 5 V)
  delay(200);
  writeRegister(0x03, B10000100); //100SPS
  delay(500);
  sendDirectCommand(B11110000); //Offset and self-gain calibration
}


void printInstructions()
{
  //This function should be in the setup() and it shows the commands - not used
  PrintMessage = "*Use the following letters to send a command to the device:" + String("\n")
                 + "*r - Read a register. Example: 'r1' - reads the register 1" + String("\n")
                 + "*w - Write a register. Example: 'w1 8' - changes the value of the 1st register to 8." + String("\n")
                 + "*O - Single readout. Example: 'O' - Returns a single value from the ADS1256." + String("\n")
                 + "*A - Single, continuous reading with manual MUX setting." + String("\n")
                 + "*C - Cycling the ADS1256 Input multiplexer in single-ended mode (8 channels). " + String("\n")
                 + "*D - Cycling the ADS1256 Input multiplexer in differential mode (4 channels). " + String("\n")
                 + "*R - Reset ADS1256. Example: 'R' - Resets the device, everything is set to default." + String("\n")
                 + "*s - SDATAC: Stop Read Data Continously." + String("\n")
                 + "*U - User Default Registers."  + String("\n")
                 + "*d - Send direct command.";

  Serial.println(PrintMessage);
  PrintMessage = ""; //Reset (empty) variable.
}


void convertToVoltage(int32_t registerData)
{
  if (registerData >> 23 == 1) //if the 24th bit (sign) is 1, the number is negative
  {
    registerData = registerData - 16777216;  //conversion for the negative sign
    //"mirroring" around zero
  }
  //This is only valid if PGA = 0 (2^0). Otherwise the voltage has to be divided by 2^(PGA)
  double voltage = ((2 * VREF) / 8388608) * registerData; //5.0 = Vref; 8388608 = 2^{23} - 1

  Serial.println(voltage, 8); //print it on serial
}