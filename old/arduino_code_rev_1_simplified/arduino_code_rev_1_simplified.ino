//cancel the averaging
//0.1.6 go 500kHz, reduce exication voltage, merge pump control code


#include "Wire.h"

//Variables: serial communication

const byte numBytes = 32;     //bytes recieved buffer size
byte receivedBytes[numBytes]; //recieve buffer
byte sendBuf[10];             //send buffer
byte numReceived = 0;         //number of bytes recieved
boolean newData = false;      //new data comes in flag

//Variables: impedence measurement parameters from PC

int freqCom = 0;              //store the frequency setting from PC
int ch = 0;                   //store the channel setting from PC
boolean newMeasurement = false;           //flag: recieved new setting, need measurement
float start_freq = 8 * pow(10, 4);        // Set start freq, < 100Khz
const float MCLK = 16.776 * pow(10, 6);   // AD5933 Internal Clock Speed 16.776 MHz
const float incre_freq = 1 * pow(10, 4);  // Set freq increment, useless when repeat frequency
const int incre_num = 4;                 // Set number of increments or repeat; < 511

//Variables: rawdata return to serial port

int RcalRe = 0;
int RcalIm = 0;
int LoadRe = 0;
int LoadIm = 0;


//Macros: AD5933
#define AD5933_ADDRE      0x0D
#define AD5933_PTR        0xB0
#define CTRL_AD5933       0x80 // address arduino
#define CTRL2_AD5933      0x81
#define START_FREQ_R1     0x82
#define START_FREQ_R2     0x83
#define START_FREQ_R3     0x84
#define FREG_INCRE_R1     0x85
#define FREG_INCRE_R2     0x86
#define FREG_INCRE_R3     0x87
#define NUM_INCRE_R1      0x88
#define NUM_INCRE_R2      0x89
#define NUM_SCYCLES_R1    0x8A
#define NUM_SCYCLES_R2    0x8B
#define TEMP_R1           0x92
#define TEMP_R2           0x93
#define RE_DATA_R1        0x94
#define RE_DATA_R2        0x95
#define IMG_DATA_R1       0x96
#define IMG_DATA_R2       0x97
#define STATUS_AD5933     0x8F

//Macros: ADG715

//#define ADG715_ADDRa 0x48 // 10010 00 for ADG715
//#define ADG715_ADDRb 0x49 // changed names to ...ADDRa/b/c as ADDR arrays are named as ADDR1/2
//#define ADG715_ADDRc 0x50
// array declarations for channels and switch codes
uint8_t ADG715_ADDR1[96] = {0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49};
uint8_t ADG715_ADDR2[96] = {0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49,0x50,0x50,0x50,0x50,0x50,0x49,0x49,0x49};
uint8_t swCode1[96] = { 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 64, 64, 64, 64, 64, 64, 64, 64, 128, 128, 128, 128, 128, 128, 128, 128, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 16, 16, 16, 16, 16};
uint8_t swCode2[96] = { 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32, 16, 8, 4, 2, 1, 128, 64, 32};

void setup() {
  Wire.setClock(1600000);
  Wire.begin();
  Serial.begin(115200);
  delay(10);
}

void loop() {
  recvBytesWithStartEndMarkers();
  showNewData();
  microPumpSer();
}

void ImpedanceMeasure(int sFreq, int Channel) {
  short re, img, maxRe, maxImg, minRe, minImg, avgRe, avgImg;
  long sumRe, sumImg;
  double mag;
  byte fAD5933Status, R1, R2;
  int cSample = 0;
  CloseSwitches(0); //measuring Rcal
  meansureInit(sFreq);
  maxRe = 0;
  minRe = 0;
  maxImg = 0;
  minImg = 0;
  sumRe = 0;
  sumImg = 0;
  do  {
    fAD5933Status = readData_AD5933(STATUS_AD5933) & 2; // measuring Rcal
  } while (fAD5933Status != 2);      //wait until results are ready
  sendBuf[0] = 1;
  sendBuf[1] = 8;
  sendBuf[2] = (byte)readData_AD5933(RE_DATA_R1);
  sendBuf[3] = (byte)readData_AD5933(RE_DATA_R2);
  sendBuf[4] = (byte)readData_AD5933(IMG_DATA_R1);
  sendBuf[5] = (byte)readData_AD5933(IMG_DATA_R2);
//
  CloseSwitches((uint8_t)Channel); //measuring Z
  meansureInit(sFreq);
  maxRe = 0;
  minRe = 0;
  maxImg = 0;
  minImg = 0;
  sumRe = 0;
  sumImg = 0;
  do  {
    fAD5933Status = readData_AD5933(STATUS_AD5933) & 2;
  } while (fAD5933Status != 2);      //wait until results are ready
  sendBuf[6] = (byte)readData_AD5933(RE_DATA_R1);
  sendBuf[7] = (byte)readData_AD5933(RE_DATA_R2);
  sendBuf[8] = (byte)readData_AD5933(IMG_DATA_R1);
  sendBuf[9] = (byte)readData_AD5933(IMG_DATA_R2);
  Serial.write(sendBuf, 10);
}

void recvBytesWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;    //number of bytes recieved
  static byte cNdx = 0;
  const byte startMarker = '<';
  const byte endMarker = '>';
  byte rb;

  while ((Serial.available() > 0) && (newData == false)  /*&&(newMeasurement == false)*/) {
    rb = Serial.read();

    if (recvInProgress == true) {
      if (rb != endMarker) {
        receivedBytes[ndx] = rb;
        ndx++;
        if (ndx >= numBytes) ndx = numBytes - 1;
      }
      else { /*if (ndx > 2)*/
        receivedBytes[ndx] = '\0'; // terminate the string
        for (cNdx = 0; cNdx < ndx; cNdx++)
          //          Serial.write(receivedBytes[cNdx]);
          recvInProgress = false; //for loop is ending
        numReceived = ndx;  // save the number for use when printing
        ndx = 0;
        newData = true;     //end the recieve progress
      }
    }
    else if (rb == startMarker) {
      recvInProgress = true;
    }
  }
}

// what does this do?
void showNewData() {
  if (newData == true) {
    switch (receivedBytes[0]) {
      case 0:
        Serial.println("Nothing!!");
        newData = false;
        ImpedanceMeasure(10, 1);
        break;
      case 1:
        freqCom = (int)receivedBytes[1] * 256 + receivedBytes[2];
        ch = (int)receivedBytes[3];
        ImpedanceMeasure(freqCom, ch);
        newData = false;
        newMeasurement = true;
        break;
      case 2:
        Wire.beginTransmission(MP6_ADDR);
        Wire.write(MP6_POWERMODE); // start adress
        Wire.write(receivedBytes[1]); // enable pumps
        Wire.write(receivedBytes[2]); // frequency 800 Hz
        Wire.write(0x00); //Adress 0x03 = 0x00 (sine wave)
        Wire.write(0x00); //Adress 0x04 = 0x00 (800kHz)
        Wire.write(0x00); //Adress 0x05 = 0x00 (audio off)
        Wire.write(receivedBytes[3]); //Adress 0x06 = 0x1F (VO1)
        Wire.write(receivedBytes[3]); //Adress 0x07 = 0x1F (VO2)
        Wire.write(receivedBytes[3]); //Adress 0x08 = 0x1F (VO3)
        Wire.write(receivedBytes[3]); //Adress 0x09 = 0x1F (VO4)
        Wire.write(0x01); //Adress 0x0A = 0x00 (update)
        Wire.endTransmission();
        newData = false;
        break;
      case 3:
        Serial.println("Reserved for motor control!!");
        newData = false;
        break;
    }
  }
}

// done
void meansureInit(int sFreq) {
  writeData_AD5933(0x81, 0x10); // internal system clock, reset
  writeData_AD5933(0x81, 0x00); // 2 V pp, x5 mode
  writeData_AD5933(0x80, 0xB5); // standby_mode
  // Start frequency
  start_freq = (float)sFreq * 1000;
  writeData_AD5933(START_FREQ_R1, getFrequency(start_freq, 1)); // 0x82
  writeData_AD5933(START_FREQ_R2, getFrequency(start_freq, 2)); // 0x83
  writeData_AD5933(START_FREQ_R3, getFrequency(start_freq, 3)); // 0x84
  // Increment
  writeData_AD5933(FREG_INCRE_R1, 0);
  writeData_AD5933(FREG_INCRE_R2, 0);
  writeData_AD5933(FREG_INCRE_R3, 0);
  // Points in frequency sweep (100), max 511... this sets it to 0x4... aka 4
  writeData_AD5933(NUM_INCRE_R1, (incre_num & 0x001F00) >> 0x08 );
  writeData_AD5933(NUM_INCRE_R2, (incre_num & 0x0000FF));
  // Set settling cycles (longest cycle setting, wait for signal to stable and start measureing)
  writeData_AD5933(NUM_SCYCLES_R1, 0x00);
  writeData_AD5933(NUM_SCYCLES_R2, 0x0C);
  //Stand by
  writeData_AD5933(0x80, 0x15);
  delay(4);
  //Start
  writeData_AD5933(0x80, 0x25);//10k||100k feedback resistance, 200mV-pp exciting voltage
}

// what's this for?
 uint8_t CloseSwitches(uint8_t ch_num) { //insert ch_num instead of SwCode
  Wire.beginTransmission(ADG715_ADDR1[ch_num]); // transmit to device ADG715 (0x48)
  Wire.write (swCode1[ch_num]);
  Wire.endTransmission();

  uint8_t res1 = Wire.requestFrom(ADG715_ADDR1, 1);
  res1 = Wire.read(); // just a read data confirmation;

  Wire.beginTransmission(ADG715_ADDR2[ch_num]); // transmit to device ADG715 (0x48)
  Wire.write (swCode2[ch_num]);
  Wire.endTransmission();
   uint8_t res2 = Wire.requestFrom(ADG715_ADDR2, 1);
  res2 = Wire.read(); // just a read data confirmation;

  // old code //Wire.write(1 << SwCode);              // close specific switch ( switch selection )(for single channel in old prototype)
  //Wire.endTransmission();               // stop transmitting


  //uint8_t res = Wire.requestFrom(ADG715_ADDR, 1);
  //res = Wire.read(); // just a read data confirmation
  //return res;
}

// done
byte getFrequency(float freq, int n) {
  long val = long((freq / (MCLK / 4)) * pow(2, 27));
  byte code;
  code = (val >> (24 - 8 * n));
  return code;
}

// done
void writeData_AD5933(int addr, int data) {
  Wire.beginTransmission(AD5933_ADDRE);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
  delay(1);
}

// done
int readData_AD5933(int addr) {
  int data;

  Wire.beginTransmission(AD5933_ADDRE);
  Wire.write(0xB0);
  Wire.write(addr);
  Wire.endTransmission();
  delay(1);
  Wire.requestFrom(AD5933_ADDRE, 1);
  if (Wire.available() >= 1) {
    data = Wire.read();
  }
  else {
    data = -1;
  }
  delay(1);
  return data;
}
