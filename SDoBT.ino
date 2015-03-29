//SDoBT - Slouch Detection over Bluetooth, For Arduino Uno
//Written by Ethan Goff
//March 2015, For HackPSU

/***********************************************************************/
/***********************************************************************/
/*    ACKS  --  Full licences can be found in licences.txt

  I2Cdev device library code [i.e. I2Cdev.h/.cpp and MPU6050.h/.cppp] 
  is placed under the MIT license
  Copyright (c) 2011 Jeff Rowberg
  ===============================================
  Serial communications code heavily influenced by Arduino, C#, and 
  Serial Interfaces by Marco Bertschi
  http://www.codeproject.com/Articles/473828/Arduino-Csharp-and-Serial-Interface

*/
/***********************************************************************/
/***********************************************************************/




/****************** External Libraries ******************************/
  //For the Accelerometer
  #include "I2Cdev.h"
  #include "MPU6050.h"
  
  // Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
  // is used in I2Cdev.h
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
  #endif

  //For the BLE Shield
  

  //Other Libraries
  #include <WString.h> //Official Arduino string library 

/************** END External Libraries ******************************/





/****************** Constants / Literals ******************************/

  //I2C Bus Addresses
  #define ACC_ADDR 0x68
  #define BLES_ADDR 0x80

  //Component Constants
  #define LED_PIN 13
  #define SERIAL_BAUDRATE 9600            //Baud-Rate of the serial Port

  //ASCII Transmission Codes
  #define STX "\x02"                      //ASCII-Code 02, text representation of the STX code
  #define ETX "\x03"                      //ASCII-Code 03, text representation of the ETX code
  #define RS  "$"                         //Used as RS code

  //WARNING, ERROR AND STATUS CODES
  #define MSG_METHOD_SUCCESS 0            //Code which is used when an operation terminated  successfully
  #define WRG_NO_SERIAL_DATA_AVAILABLE 250    //Code indicates that no new data is AVAILABLE at the serial input buffer
  #define ERR_SERIAL_IN_COMMAND_NOT_TERMINATED -1   //Code is used when a serial input commands' last char is not a '#'

/*************** END Constants / Literals ******************************/





/********************** Data Structures ******************************/

//encapsulates a reading of current coordinate states for the 
//  accelerometer and gyroscope
struct AccGyro_Reading
{
  public:
  AccGyro_Reading()
  {
    ax = ay = az = gx = gy = gz = 0;
  }  
  
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  
  void UpdateCoordinates(MPU6050 * accelgyro)
  {
    accelgyro->getMotion6(&(this->ax), &(this->ay), &(this->az), &(this->gx), &(this->gy), &(this->gz));
  }  
};

/********************** END Data Structures ******************************/





/******************* Method Declarations *******************************/

int readSerialInputString(String *command);
void WriteDummyWeatherData();

/******************* End Method Declarations ***************************/





/*************************** Globals ************************************/

//Our Accelerometer object
MPU6050 accelgyro(ACC_ADDR);

//Global coordinate variables 
int16_t gl_ax, gl_ay, gl_az;
int16_t gl_gx, gl_gy, gl_gz;

unsigned long StartTime;

/*************************** End Globals ************************************/












void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(SERIAL_BAUDRATE);

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
    

    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    StartTime = millis();
}



/*****************************************************************
The loop method is executed forever right after the setup method
is finished.
******************************************************************/
void loop()
{
  char command[5] = {'\0','\0','\0','\0','\0'};  //Used to store the latest received command
  char * commandPointer = (char*)command;
  int serialResult = 0; //return value for reading operation method on serial in put buffer


  unsigned long ElapsedTime = millis() - StartTime;

  Serial.write((uint8_t *)&ElapsedTime, 4); 
  Serial.write('\n');
  
  

  if(Serial.available())
  {
    int commandLength = Serial.readBytesUntil('#', command, 5);
    if( commandPointer[0] == 'C' && commandPointer[1] == 'R')
    {
     //Request for sending weather data via Serial Interface
     //For demonstration purposes this only writes dummy data
        SendCurrentReading();
    }
  }
  
  

}









/*****************************************************************
Description:
This method reads the serial input buffer and writes the content 
of the serial input buffer to the variable which is adressed by the
pointer *command.

Return-Values: MSG_METHOD_SUCCESS (command is valid) 
               or 
               ERR_SERIAL_IN_COMMAND_NOT_TERMINATED (command is invalid)

******************************************************************/
int readSerialInputCommand(char * command){
  
  int operationStatus = MSG_METHOD_SUCCESS;//Default return is MSG_METHOD_SUCCESS reading data from com buffer.
  
  //check if serial data is available for reading
  if (Serial.available()) {     
     int commandLength = Serial.readBytesUntil('#', command, 5);

     if(commandLength < 1) {
       operationStatus = ERR_SERIAL_IN_COMMAND_NOT_TERMINATED;
     }
  }
  else{//If not serial input buffer data is AVAILABLE, operationStatus becomes WRG_NO_SERIAL_DATA_AVAILABLE (= No data in the serial input buffer AVAILABLE)
    operationStatus = WRG_NO_SERIAL_DATA_AVAILABLE;
  }
  
  return operationStatus;
}







/*****************************************************************
Description:
This method writes data to the serial interface output buffer. 

FORMAT:
[STX]
STUFF INVOLVING THINGS [ETX]

******************************************************************/
void SendCurrentReading()
{
  // read raw accel/gyro measurements from device
  accelgyro.getMotion6(&gl_ax, &gl_ay, &gl_az, &gl_gx, &gl_gy, &gl_gz);

  unsigned long ElapsedTime = millis() - StartTime;

  
  Serial.print(STX);

  Serial.write('B');

  Serial.write((uint8_t)(ElapsedTime >> 24)); 
  Serial.write((uint8_t)(ElapsedTime >> 16)); 
  Serial.write((uint8_t)(ElapsedTime >> 8)); 
  Serial.write((uint8_t)(ElapsedTime & 0xFF));

  Serial.print(RS);  

  Serial.write((uint8_t)(gl_ax >> 8)); Serial.write((uint8_t)(gl_ax & 0xFF));
  Serial.print(RS);  

  Serial.write((uint8_t)(gl_ay >> 8)); Serial.write((uint8_t)(gl_ay & 0xFF));
  Serial.print(RS);  

  Serial.write((uint8_t)(gl_az >> 8)); Serial.write((uint8_t)(gl_az & 0xFF));
  Serial.print(RS);  

  Serial.write((uint8_t)(gl_gx >> 8)); Serial.write((uint8_t)(gl_gx & 0xFF));
  Serial.print(RS);  

  Serial.write((uint8_t)(gl_gy >> 8)); Serial.write((uint8_t)(gl_gy & 0xFF));
  Serial.print(RS);  

  Serial.write((uint8_t)(gl_gz >> 8)); Serial.write((uint8_t)(gl_gz & 0xFF));
  Serial.print(RS);  
  
  Serial.print(ETX);
}

void Test()
{
  Serial.print(STX);

  Serial.print("hello!");
  
  Serial.print(ETX);
}
