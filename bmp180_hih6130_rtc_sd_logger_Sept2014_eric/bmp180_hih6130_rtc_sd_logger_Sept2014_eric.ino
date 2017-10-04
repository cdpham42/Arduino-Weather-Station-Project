/*
 ATMS 315 - All Sensors Example Code
 August 2014
 AJB
 
 ==================================================================
 This sketch is designed to read and log sensor readings from the 
 tmp102 temperature sensor, bmp180 pressure sensor, and HIH6130
 humidity sensor with timestamps (chronodot) to a microSD card. 
 ==================================================================
 
 Designed for:
 - bmp180 pressure sensor
 - hih6130 humidity sensor
 - microSD breakout board
 - Chronodot real time clock
 
 
 microSD breakout board wiring:
 GND to ground
 5V to 5V
 CLK to pin 13
 DO to pin 12
 DI to pin 11
 CS to pin 10 (*for Arduino Uno only*)
 
 
 Adam J. Burns
 atmos@aburns.us
 */


#include <Wire.h> // needed for i2c communication

#define LOG_INTERVAL  500 // 60000 milliseconds (1 min) between entries
#define ECHO_TO_SERIAL   1 // print data to serial port (use 0 to disable)

// ========== Chronodot ==========
#include <RTClib.h>
RTC_DS1307 RTC; // create an instance called "RTC" for the chronodot
boolean initialrun = true;

// ========== microSD ==========
#include <SD.h>
const int chipSelect = 10; // SD cs line on D10
File logfile; // log file
long unixInitial=0;
long unixNet=0;

// ========== bmp180 ==========
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

// ========== HIH6130 ==========
#include <HIH61XX.h>
HIH61XX hih(0x27, 8);


//================================== setup function =========================================
void setup(){
  Serial.begin(9600); // initialize the serial monitor
  Wire.begin(); // initialize the wire library

  // ----------------- initialize bmp180 pressure sensor ------------------
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }

  // -----------initialize SD card------------------
  Serial.print("SD initialization ");
  pinMode(10, OUTPUT); // set pin 10 as an output (for microSD breakout board)
  if (!SD.begin(chipSelect)) {
    Serial.println("ERROR: SD card failed, or not present");
    Serial.println("Make sure SD card is properly inserted and wiring is correct");
    while(1); // halt the code
  }
  Serial.println("card initialized.");

  // make new log file
  // this is just a method to make sure the new log file has a unique file name
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); // open logfile on microSD card
      break; 
    }
  }

  if (! logfile) {
    Serial.println("ERROR: couldn't create file");
    Serial.println("Make sure SD card is properly inserted and wiring is correct");
    while(1);
  }

  Serial.print("Logging to: ");
  Serial.println(filename);
  //---------end SD card initialization------------


  //-------------initialize RTC-------------------
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("ERROR: RTC is NOT running!");
    Serial.println("Make sure chronodot wiring is correct");
    while(1);
  }
    RTC.adjust(DateTime(__DATE__, __TIME__));

  //----------end RTC initialization-----------------


  // --------- write header row to log file ---------
  logfile.println("date,time,seconds,bmpTemp[C],bmpTemp[F],Pressure[hPa],HIHtemp[C],HIHtemp[F],RH[%]");    
#if ECHO_TO_SERIAL
  Serial.println("date,time,seconds,bmpTemp[C],bmpTemp[F],Pressure[hPa],HIHtemp[C],HIHtemp[F],RH[%]");
#endif //ECHO_TO_SERIAL
}
//================================ end setup function =======================================



//================================ loop function ============================================
void loop(){
  // ========== get the time ==========
  DateTime now;
  now = RTC.now();

  // ========== check to see if this is the first run ==========
  if(initialrun){
    unixInitial=now.unixtime();
    initialrun=false;
  }

  // ========== write the date to the file ==========
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print("/");
  logfile.print(now.year(), DEC);
  logfile.print(",");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);

#if ECHO_TO_SERIAL
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print("/");
  Serial.print(now.year(), DEC);
  Serial.print(",");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
#endif //ECHO_TO_SERIAL

  // ====== log seconds since start using unixtime (see "initialrun" above) ==========
  unixNet=(now.unixtime());
  unixNet=unixNet-unixInitial;
  logfile.print(",");    
  logfile.print(unixNet);

#if ECHO_TO_SERIAL
  Serial.print(", ");
  Serial.print(unixNet);
  Serial.print(", ");
#endif //ECHO_TO_SERIAL

  // ======== read & log BMP180 pressure sensor readings (temp [C] & pressure [Pa]) ========
  float bmpTemp = bmp.readTemperature();
  float bmpPressure = bmp.readPressure();
  float bmpTempF = bmpTemp * 9/5 + 32;
  logfile.print(",");
  logfile.print(bmpTemp);
  logfile.print(",");
  logfile.print(bmpTempF);
  logfile.print(",");
  logfile.print(bmpPressure/100);

#if ECHO_TO_SERIAL

  Serial.print(bmpTemp); // print the temperature reading in F to the serial monitor
  Serial.print(" C, ");
  Serial.print(bmpTempF);
  Serial.print(" F, ");
  Serial.print(bmpPressure/100); // print the temperature reading in F to the serial monitor
  Serial.print(" hPa, ");
#endif //ECHO_TO_SERIAL


  // ========== read & log HIH6130 sensor readings (temp [C] & RH[%]) ==========
  hih.start(); //  start the sensor
  hih.update(); //  request an update of the humidity and temperature
  float HIHtemp=(hih.temperature());
  float humidity=((hih.humidity())*100);
  float HIHtempF = HIHtemp * 9/5 + 32;

  logfile.print(",");
  logfile.print(HIHtemp);
  logfile.print(",");
  logfile.print(HIHtempF);
  logfile.print(",");
  logfile.print(humidity);

#if ECHO_TO_SERIAL
  Serial.print(HIHtemp); 
  Serial.print(" C, ");
  Serial.print(HIHtempF); 
  Serial.print(" F, ");
  Serial.print(humidity);
  Serial.println(" %");
#endif //ECHO_TO_SERIAL



  logfile.println(); // start a new line in the log file
  logfile.flush(); // waits for outgoing data to complete writing to the microSD card

  delay(LOG_INTERVAL); // delay between each reading
}
//================================ end loop function =======================================






