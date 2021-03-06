/*

 Casey Pham
 www.github.com/cdpham42
 
 Original code from ATMS 315, University of Illinois, 2014
 Adam J. Burns
 atmos@aburns.us
 
 ==================================================================
 This sketch is designed to read and log sensor readings from the 
 tmp102 temperature sensor, bmp180 pressure sensor, and HIH6130
 humidity sensor with timestamps (chronodot) to a microSD card. 

 Additions beyond this are separate from the original project code.
 Additions include RGB LCD Shield output, variable calculations
 beyond sensor reading, and output formatting.
 ==================================================================
 
 Designed for:
 - bmp180 pressure sensor
 - hih6130 humidity sensor
 - microSD breakout board
 - Chronodot real time clock
 - RGB LCD Shield
 
 
 microSD breakout board wiring:
 GND to ground
 5V to 5V`+
 
 CLK to pin 13
 DO to pin 12
 DI to pin 11
 CS to pin 10 (*for Arduino Uno only*)
 
 */


#include <Wire.h> // needed for i2c communication

// RGB LCD Shield Library
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Define colors for LCD Shield
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
#define BLACK 0x0

#define LOG_INTERVAL  50 // 60000 milliseconds (1 min) between entries
#define LOG_TO_FILE   0 // Log data to file (use 0 to disable)
#define ECHO_TO_SERIAL   0 // print data to serial port (use 0 to disable)
#define LCD_PRINT   1 // Print to LCD

// ========== Chronodot ==========
#include <RTClib.h>
RTC_DS1307 RTC; // create an instance called "RTC" for the chronodot
boolean initialrun = true;

// ========== microSD ==========
  #include <SD.h>
  const int chipSelect = 10; // SD cs line on D10
  File logfile; // log file
  long unixInitial = 0;
  long unixNet = 0;

// ========== bmp180 ==========
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

// ========== HIH6130 ==========
#include <HIH61XX.h>
HIH61XX hih(0x27, 8);

// ========== RGB LCD Shield ==========
long start_time = 0;    // start and end times to auto turn-off the display
long end_time = 0;
bool on = false;        // bool for on/off for updating while on
int button = 0;         // to know which button was pressed to correctly update display while on

// ========== Initialize Variables ==========

float bmpTemp = 0;
float bmpPressure = 0;
float bmpTempF = 0;

float HIHtemp = 0;
float humidity = 0;
float HIHtempF =0;

// ======== Calculate Dew Point using Arden Buck Equation using HIHtemp ========

// Constant values from Arden Buck for temperature range 0C <= T <= 50C
float a = 6.1121;
float b = 17.368;
float c = 238.88;
float d = 234.5;

float gamma_m = 0;
float dewpoint = 0;
float dewpointF = 0;


//================================== START SETUP =========================================
void setup(){
  
  Serial.begin(9600); // initialize the serial monitor
  Wire.begin(); // initialize the wire library
  lcd.begin(16,2);

  lcd.print("Hello!"); // Test LCD Shield print when not LCD print is not included in loop.

  // ----------------- initialize bmp180 pressure sensor ------------------
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }

  // ----------- initialize SD card if LOG_TO_FILE ------------------
#if LOG_TO_FILE

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
  
  // --------- write header row to log file ---------
  logfile.println("seconds,date,time,bmpTemp[C],bmpTemp[F],Pressure[hPa],HIHtemp[C],HIHtemp[F],RH[%],Dew_Point[C],Dew_Point[F]");
  
#endif
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



  // --------- write header row to serial ---------
#if ECHO_TO_SERIAL
  Serial.println("seconds, date, time, bmpTemp[C], bmpTemp[F], Pressure[hPa], HIHtemp[C], HIHtemp[F], RH[%], Dew Point [C], Dew Point [F]");
#endif //ECHO_TO_SERIAL

}


//================================ END SETUP =======================================



//================================ START LOOP ============================================

void loop(){

  // ========== get the time ==========
  DateTime now;
  now = RTC.now();

  // ========== check to see if this is the first run ==========
  if(initialrun){
    unixInitial = now.unixtime();
    start_time = now.unixtime();
    initialrun = false;
  }

  // ================================ Read Sensor Data and Calculate Values ================================

  // ======== read BMP180 pressure sensor readings (temp [C] & pressure [Pa]) ========

if (on){
  bmpTemp = bmp.readTemperature();
  bmpPressure = bmp.readPressure();
  bmpTempF = bmpTemp * 9/5 + 32;
  
  // ======== read HIH6130 sensor readings (temp [C] & RH[%]) ========
  hih.start();
  hih.update(); //  request an update of the humidity and temperature
  HIHtemp=(hih.temperature());
  humidity=((hih.humidity())*100);
  HIHtempF = HIHtemp * 9/5 + 32;

  // ======== Calculate Dew Point using Arden Buck Equation using HIHtemp ========

  // Constant values from Arden Buck for temperature range 0C <= T <= 50C

  gamma_m = log( (humidity / 100) * exp( (b-HIHtemp/d) *  (HIHtemp/(c + HIHtemp)) ) );
  dewpoint = (c * gamma_m) / (b - gamma_m);
  dewpointF = dewpoint * 9/5 + 32;
}
  // ================================ LCD Shield Input and Output ================================

  // Use Chronodot second recordign to create timer for display auto-off instead of attempting to iterate through loops

#if LCD_PRINT

  uint8_t buttons = lcd.readButtons();
  int lcd_off = 10000 / LOG_INTERVAL;     // 10 seconds divided by log_interval to get number of loops until screen off

  // Show Temp and Humidity in Fahrenheit
  if (buttons & BUTTON_UP)  {

    start_time = now.unixtime();
    end_time = start_time + 10;
    button = 1;
    
    lcd.clear();
    lcd.setBacklight(WHITE);
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.print(HIHtempF);
    lcd.print("F");
  
    lcd.setCursor(0,1);
    lcd.print("DewPoint: ");
    lcd.print(dewpointF);
    lcd.print("F");

    on = true;
    
  }

  // Show Temp and Humidity in Celsius
  if (buttons & BUTTON_LEFT){
    
    start_time = now.unixtime();
    end_time = start_time + 10;
    button = 2;
    
    lcd.clear();
    lcd.setBacklight(BLUE);
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.print(HIHtemp);
    lcd.print("C");
  
    lcd.setCursor(0,1);
    lcd.print("DewPoint: ");
    lcd.print(dewpoint);
    lcd.print("C");

    on = true;
    
  }

  // Show HIH Temp and BMP Temp in Fahrenheit
  if (buttons & BUTTON_RIGHT){
    
    start_time = now.unixtime();
    end_time = start_time + 10;
    button = 3;
    
    lcd.clear();
    lcd.setBacklight(RED);
    lcd.setCursor(0,0);
    lcd.print("HIH Temp: ");
    lcd.print(HIHtemp);
    lcd.print("C");
  
    lcd.setCursor(0,1);
    lcd.print("BMP Temp: ");
    lcd.print(bmpTemp);
    lcd.print("C");

    on = true;
    
  }

  // Show Pressure
  if (buttons & BUTTON_DOWN){
    
    start_time = now.unixtime();
    end_time = start_time + 10;
    button = 4;
    
    lcd.clear();
    lcd.setBacklight(GREEN);
    lcd.setCursor(0,0);
    lcd.print("Pressure: ");
    lcd.print(bmpPressure/100);

    on = true;
    
  }

  if (buttons & BUTTON_SELECT){

    if (on){ 
       
    lcd.clear();
    lcd.setBacklight(0x0);

    on = false;
    }
    
  }

  if (on){
    if (now.unixtime() != end_time){

      // If BUTTON_UP is on
      if (button == 1){
        lcd.setCursor(0,0);
        lcd.print("Temp: ");
        lcd.print(HIHtempF);
        lcd.print("F");
        
        lcd.setCursor(0,1);
        lcd.print("DewPoint: ");
        lcd.print(dewpointF);
        lcd.print("F");
      }

      // If BUTTON_LEFT is on
      if (button == 2){
        lcd.setCursor(0,0);
        lcd.print("Temp: ");
        lcd.print(HIHtemp);
        lcd.print("C");
    
        lcd.setCursor(0,1);
        lcd.print("DewPoint: ");
        lcd.print(dewpoint);
        lcd.print("C");
      }

      // If BUTTON_RIGHT is on
      if (button == 3){
        lcd.setCursor(0,0);
        lcd.print("HIH Temp: ");
        lcd.print(HIHtempF);
        lcd.print("F");
  
        lcd.setCursor(0,1);
        lcd.print("BMP Temp: ");
        lcd.print(bmpTempF);
        lcd.print("F");
      }

      // If BUTTON_DOWN is on
      if (button == 4){
        lcd.setCursor(0,0);
        lcd.setCursor(0,0);
        lcd.print("Pressure: ");
        lcd.print(bmpPressure/100);
      }

      
      
    }
    
    if (now.unixtime() == end_time){
      lcd.clear();
      lcd.setBacklight(BLACK);
      on = false;
    }
    
  } 

# endif
 
  // ================================ Write to File ================================

#if LOG_TO_FILE

  // ====== log seconds since start using unixtime (see "initialrun" above) ==========
  unixNet=(now.unixtime());
  unixNet=unixNet-unixInitial;
  logfile.print(",");    
  logfile.print(unixNet);
  
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

  logfile.print(",");
  logfile.print(bmpTemp);
  logfile.print(",");
  logfile.print(bmpTempF);
  logfile.print(",");
  logfile.print(bmpPressure/100);

  logfile.print(",");
  logfile.print(HIHtemp);
  logfile.print(",");
  logfile.print(HIHtempF);
  logfile.print(",");
  logfile.print(humidity);
  logfile.print(",");

  logfile.print(dewpoint);

  logfile.println(); // start a new line in the log file
  logfile.flush(); // waits for outgoing data to complete writing to the microSD card

#endif

  // ================================ Echo to Serial ================================

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
  Serial.print(", ");
  
  Serial.print(unixNet);
  Serial.print(", ");
  
  Serial.print(bmpTemp); // print the temperature reading in F to the serial monitor
  Serial.print(" C, ");
  Serial.print(bmpTempF);
  Serial.print(" F, ");
  Serial.print(bmpPressure/100); // print the temperature reading in F to the serial monitor
  Serial.print(" hPa, ");
  
  Serial.print(HIHtemp); 
  Serial.print(" C, ");
  Serial.print(HIHtempF); 
  Serial.print(" F, ");
  Serial.print(humidity);
  Serial.print(" %, ");

  Serial.print(dewpoint);
  Serial.print(" C, ");
  Serial.print(dewpointF);
  Serial.print(" F, ");
  
  Serial.print(start_time);
  Serial.print(", ");
  Serial.println(end_time);
  
#endif //ECHO_TO_SERIAL

  delay(LOG_INTERVAL); // delay between each reading
  
}
//================================ END LOOP =======================================





