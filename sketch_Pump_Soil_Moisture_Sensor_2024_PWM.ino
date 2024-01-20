// load Arduino_JSON to use JSON format.
#include <Arduino_JSON.h> // include Arduino_JSON library
#include "Arduino_JSON.h" // include Arduino_JSON header 

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
RTC_DS3231 rtc;
// define days of the week, for serial console display, etc.
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
volatile unsigned int roundYear;
String roundMonth;
String roundDay;
String roundHour;
String roundMinute;
String roundSecond;

#include <DHT.h> // include DHT library 
#include "DHT.h" // include DHT header
#define DHTPIN 38     // what pin we're connected to
#define DHTTYPE DHT22   // we are using the DHT22 sensor
DHT dht(DHTPIN, DHTTYPE); // initialize DHT on pin and choose DHT sensor type
// define sensor-data-input-pins 
#define sensorPin1 50
#define sensorPin2 48
#define sensorPin3 46
#define sensorPin4 44

// using correct data types for longer delay periods is critical
// Serial Number for Arduino FT232R USB UART
const String SerialNumber = "A50285BI";
// onboard Arduino LED
#define boardLEDPin 13
// default watering/irrigation period
unsigned int wateringPeriod= 60000;
// cast timeoutPeriodDecay for use in responsivedelay() function
volatile unsigned long timeoutPeriodDecay = 1;
// Wikipedia: "the most sensitive games ... achieve response times of 67 ms (excluding display lag)"
// this delay() interval is used within responsivedelay() to decrease while loop intervals and to not create lag
// *without this interval, Arduino will check as often as possible to execute the while loop.
unsigned int timeoutPeriodDecayInterval = 67;
// time to wait before automatically starting another irrigation check.
// irrigation occurs when moisture sensor level is above limitWarn.
volatile unsigned long pollingPeriod = 3600000;
//volatile unsigned int pollingPeriod = 36000;
unsigned int measuringPeriod = 2000;
unsigned int sensorValue1;
unsigned int sensorValue2;
unsigned int sensorValue3;
unsigned int sensorValue4;
unsigned int sensorValueAverage;

#define sensorPowerPin 43
/* 
sensorPowerPin enables +5V POE, wired in 10/100 mode B, DC on spares, at the RJ45 port. 
The wires for Rx+, Rx,- Tx+, & Tx- are used for 4x digital sensor connections

WEMOS PIN    T568A colour    RJ45 Pin    Arduino Pin    Variable
D5           white/green     1           44             sensorValue1
D6           green           2           46             sensorValue2
D7           white/orange    3           48             sensorValue3
D8           orange          6           50             sensorValue4
5V           white/blue      4           5V             sensorPowerPin (12V MOSFET - Contoller-side )
5V           blue            5           5V
GND          white/brown     7           GND
GND          brown           8           GND
 */
// lowering threshold for capacitive sensor 
unsigned int limitWarn = 512;
unsigned int limitMid = 416;
unsigned int limitWet = 320;

// continue to use #define for all permanent pin locations.
// relay-pump pins - these pins could also be used for pwm-MOSFET control
#define pump1MOSFETPin 5
#define pump2MOSFETPin 2
#define pump3MOSFETPin 6
#define pump4MOSFETPin 7
volatile unsigned int pumpxMOSFETPin = pump1MOSFETPin;

// motor controller table - set forward direction
// use ENA1 & ENA2 with pwm adjust speed
/*
IN1-IN3    IN2-IN4    Direction
LOW        LOW        OFF
HIGH       LOW        FORWARD
LOW        HIGH       REVERSE
HIGH       HIGH       OFF
*/

// use digital pins for motor controller direction
#define pumpControllerIN1 29
#define pumpControllerIN2 31
#define pumpControllerIN3 33
#define pumpControllerIN4 35

// indicator light pins
#define bluePin        49
#define greenPin       51
#define redPin         53

const int pwmPin1 = 10; // Timer 2 "A" output: PB4 ( OC2A/PCINT4 )
const int pwmPin2 = 9;  // Timer 2 "B" output: PH6 ( OC2B )
// time between stages when stepping up/down pwm
unsigned int pwmIntervalTime = 10000;

void setup() {
    // setup pins for Relays (not MOSFETs) at the pump box
    pinMode(boardLEDPin, OUTPUT);
    pinMode(pump1MOSFETPin, OUTPUT);
    pinMode(pump2MOSFETPin, OUTPUT);
    pinMode(pump3MOSFETPin, OUTPUT);
    pinMode(pump4MOSFETPin, OUTPUT);

    // setup pins for motor controller direction
    pinMode(pumpControllerIN1, OUTPUT);
    pinMode(pumpControllerIN2, OUTPUT);
    pinMode(pumpControllerIN3, OUTPUT);
    pinMode(pumpControllerIN4, OUTPUT);


    // setting both motor controller to: OFF
    digitalWrite(pumpControllerIN1, HIGH);
    digitalWrite(pumpControllerIN2, HIGH);
    digitalWrite(pumpControllerIN3, LOW);
    digitalWrite(pumpControllerIN4, LOW);

    // Do not change timer 0
    // set pump pwm power pins as output
    pinMode(pwmPin1, OUTPUT);
    pinMode(pwmPin2, OUTPUT);

    // DHT Sensor - take an initial reading before serial is started
    pinMode(DHTPIN, INPUT);
    // Start the DHT sensor
    dht.begin();
    // Reading temperature or humidity <- minimum: 250 ms
    delay(measuringPeriod);

    // Start serial at 38400 baud rate
    // bash: `screen /dev/ttyACM0 38400`
    Serial.begin(9600, SERIAL_8N1);

    #ifndef ESP8266
        while (!Serial); // wait for serial port to connect. Needed for native USB
    #endif

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    // Soil Moisture Sensors
    pinMode(sensorPin1, INPUT);
    pinMode(sensorPin2, INPUT);
    pinMode(sensorPin3, INPUT);
    pinMode(sensorPin4, INPUT);

    // Soil Moisture Sensor Power
    pinMode(sensorPowerPin, OUTPUT);

    // Water Pump - with pump off
    digitalWrite(pump1MOSFETPin, HIGH);
    digitalWrite(pump2MOSFETPin, HIGH);
    digitalWrite(pump3MOSFETPin, HIGH);
    digitalWrite(pump4MOSFETPin, HIGH);

    // Blue LED
    pinMode(bluePin, OUTPUT);

    // Red LED
    pinMode(redPin, OUTPUT);

    // Green LED
    pinMode(greenPin, OUTPUT);

    // turn off the onboard LED.
    digitalWrite(boardLEDPin, LOW);
    }


void loop() {

    /// Our main loop() is used for constant plant monitoring
    ////// activate the soil moisture sensor //////
    digitalWrite(sensorPowerPin, HIGH);
    // wait measuringPeriod ms for soil moisture sensor to take measurement 
    responsivedelay(measuringPeriod);
    sensorValue1 = analogRead(sensorPin1);
    sensorValue2 = analogRead(sensorPin2);
    sensorValue3 = analogRead(sensorPin3);
    sensorValue4 = analogRead(sensorPin4);
    /////// end of soil moisture sensor activation //////

    // Blue LED - indicate wet soil moisture
    if (sensorValue1<limitWet) {
        LEDcycle(bluePin);
    }
    // No LED - indicate above average soil moisture
    if ((sensorValue1>limitWet) && (sensorValue1<=limitMid)) {
        LEDcycle(boardLEDPin);
    }
    // Green LED - indicate optimum soil moisture
    if ((sensorValue1>limitMid) && (sensorValue1<=limitWarn)) {
        LEDcycle(greenPin);
    }
    // Red LED - indicate dry soil and start irrigation on Pump 1
    if (sensorValue1>limitWarn) {
        //pumpxMOSFETPin=pump1MOSFETPin;
        irrigationcycle(pump1MOSFETPin);
    }
    //  dry soil starts irrigation on Pump 2
    if (sensorValue2>limitWarn) {
        //pumpxMOSFETPin=pump2MOSFETPin;
        irrigationcycle(pump2MOSFETPin);
    }
    
    if (sensorValue3>limitWarn) {
        //pumpxMOSFETPin=pump3MOSFETPin;
        irrigationcycle(pump3MOSFETPin);
    }
    
    if (sensorValue4>limitWarn) {
        //pumpxMOSFETPin=pump4MOSFETPin;
        irrigationcycle(pump4MOSFETPin);
    }
    ////// Safety Power-down <- begins //////
    // Make sure to power down the soil moisture sensor. 
    digitalWrite(sensorPowerPin, LOW);
    digitalWrite(pwmPin1, LOW);
    digitalWrite(pwmPin2, LOW);
    // Make sure to power down pump.
    digitalWrite(pump1MOSFETPin, HIGH);
    digitalWrite(pump2MOSFETPin, HIGH);
    digitalWrite(pump3MOSFETPin, HIGH);
    digitalWrite(pump4MOSFETPin, HIGH);
    ////// Safety Power-down <- ends //////

    // Wait and recheck moisture after pollingPeriod 
    //delay(pollingPeriod);
    responsivedelay(pollingPeriod);
    
    // append comma to json before printing the next JSON array to Serial
    // Serial.print(",");
}


void LEDcycle(unsigned int LEDPin) {

    // turning on LED for wateringPeriod ms:
    digitalWrite(LEDPin, HIGH);
    responsivedelay(wateringPeriod);
    digitalWrite(LEDPin, LOW);
}


void irrigationcycle(unsigned int pumpxMOSFETPin) {

    // turning on LED for wateringPeriod ms (dry soil indicator)
    digitalWrite(redPin, HIGH);
    
    // power down the soil moisture sensor before any watering takes place. 
    digitalWrite(sensorPowerPin, LOW);

    // setting both motor controller to: forward-direction
    digitalWrite(pumpControllerIN1, HIGH);
    digitalWrite(pumpControllerIN2, LOW);
    digitalWrite(pumpControllerIN3, HIGH);
    digitalWrite(pumpControllerIN4, LOW);

    // water for wateringPeriod ms using pin pwmPin1/pwmPin2, bringing low when finished. 
    // set up Timer 2
    // WGM21 = set Timer mode to PWM
    // COM2A/2B = clear Output on compare match
    TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    // set prescaler
    TCCR2B = _BV(WGM22) | _BV(CS22);
    // adjust bits to control duty cycle, when watering
    
    // OCR2A – Output Compare Register A
    //The Output Compare Register A contains an 8-bit value that is continuously compared with the
    //counter value (TCNT2). A match can be used to generate an Output Compare interrupt, or to
    //generate a waveform output on 
    ICR1 = 1;

    OCR2A = 180;
    // OCR2A – Output Compare Register B with TCNT2
    
    // water for wateringPeriod ms using pin pumpxMOSFETPin, bringing low when finished. (for relay use)
    digitalWrite(pumpxMOSFETPin, LOW);

    // OCRB2B will control Pin 9 PMW
    // Pump @ 110mL/minute
    // OCR2B = 245;
    // responsivedelay(wateringPeriod);

    // Pump @ 110mL/minute with noise.
    // OCR2B = 180;
    // responsivedelay(wateringPeriod);

    // Pump @ 0mL/minute with noise with tone.
    // OCR2B = 10;
    // responsivedelay(wateringPeriod);

    // Pump @ mL/minute, little noise
    OCR2B = 140;
    responsivedelay(wateringPeriod/2);


    //responsivedelay(wateringPeriod/1000);
//    OCR2B = 9;
    //responsivedelay(wateringPeriod/1000);
//    OCR2B = 100;
//    responsivedelay(wateringPeriod/60);
//    OCR2B = 50
    //responsivedelay(wateringPeriod/10000);

    // bring low pumpxMOSFETPin, (turn off relay)
    digitalWrite(pumpxMOSFETPin, HIGH);

    // setting both motor controller to: OFF
    digitalWrite(pumpControllerIN1, HIGH);
    digitalWrite(pumpControllerIN2, HIGH);
    digitalWrite(pumpControllerIN3, LOW);
    digitalWrite(pumpControllerIN4, LOW);
    
    // turns off Timer 2
    TCCR2A = 0;
    TCCR2B = 0;

    // turn off led indicator
    digitalWrite(redPin, LOW);
    }
void sensorReport() {
    ////// create JSON output for serial or other modes of communication //////
    // get current time for timeArray
    DateTime roundtime = rtc.now();
    volatile unsigned long unixTime = roundtime.unixtime();

    // variables for RFC3339 timestamp construction
    roundYear = roundtime.year();
    roundMonth = roundtime.month();
    roundDay = roundtime.day();
    roundHour = roundtime.hour();
    roundMinute = roundtime.minute();
    roundSecond = roundtime.second();

    if (roundtime.month() < 10) {
    roundMonth = String("0" + roundMonth);
    }

    if (roundtime.day() < 10) {
    roundDay = String("0" + roundDay);
    }

    if (roundtime.hour() < 10) {
    roundHour = String("0" + roundHour);
    }

    if (roundtime.minute() < 10) {
    roundMinute = String ("0" + roundMinute);
    }

    if (roundtime.second() < 10) {
    roundSecond = String("0" + roundSecond);
    }

    String neg = "-";
    // eg. 2016-12-01T19:06:50.000-05:00
    String rfc3339 = roundYear + neg + roundMonth + neg + roundDay + "T" + roundHour + ":" + roundMinute + ":" + roundSecond + ".000-05:00";   
    /*
    JSONVar timeArray;
    timeArray["Hour"] = roundHour;
    timeArray["Minute"] = roundMinute;
    timeArray["Second"] = roundSecond;
    timeArray["DayofWeek"] = daysOfTheWeek[roundtime.dayOfTheWeek()];
    timeArray["Month"] = roundMonth;
    timeArray["Day"] = roundDay;
    timeArray["Year"] = roundYear;
    timeArray["UNIX"] = unixTime;
    timeArray["@timestamp"] = rfc3339;
    */

    // get current alarms for the alarm arrays
    ////// the stored alarm1 value + mode //////
    DateTime alarm1 = rtc.getAlarm1();
    Ds3231Alarm1Mode alarm1mode = rtc.getAlarm1Mode();
    char alarm1Date[10] = "hh:mm:ss";
    alarm1.toString(alarm1Date);
    /////// end of alarm1 value + mode storing //////

    ////// the stored alarm2 value + mode //////
    DateTime alarm2 = rtc.getAlarm2();
    Ds3231Alarm2Mode alarm2mode = rtc.getAlarm2Mode();
    char alarm2Date[10] = "hh:mm:ss";
    alarm2.toString(alarm2Date);
    /////// end of alarm2 value + mode storing //////

    JSONVar alarm1Array;
    alarm1Array["Alarm"] = daysOfTheWeek[alarm1.dayOfTheWeek()];
    alarm1Array["AlarmDate"] = alarm1Date;
    switch (alarm1mode) {
        case DS3231_A1_PerSecond: alarm1Array["Mode"] = "PerSecond"; break;
        case DS3231_A1_Second: alarm1Array["Mode"] = "Second"; break;
        case DS3231_A1_Minute: alarm1Array["Mode"] = "Minute"; break;
        case DS3231_A1_Hour: alarm1Array["Mode"] = "Hour"; break;
        case DS3231_A1_Date: alarm1Array["Mode"] = "Date"; break;
        case DS3231_A1_Day: alarm1Array["Mode"] = "Day"; break;
    };

    JSONVar alarm2Array;
    alarm2Array["Alarm"] = daysOfTheWeek[alarm2.dayOfTheWeek()];
    alarm2Array["AlarmDate"] = alarm2Date;
    switch (alarm2mode) {
        case DS3231_A1_PerSecond: alarm2Array["Mode"] = "PerSecond"; break;
        case DS3231_A1_Second: alarm2Array["Mode"] = "Second"; break;
        case DS3231_A1_Minute: alarm2Array["Mode"] = "Minute"; break;
        case DS3231_A1_Hour: alarm2Array["Mode"] = "Hour"; break;
        case DS3231_A1_Date: alarm2Array["Mode"] = "Date"; break;
        case DS3231_A1_Day: alarm2Array["Mode"] = "Day"; break;
    };

    JSONVar alarmArray;
    alarmArray["Alarm1"] = alarm1Array;
    alarmArray["Alarm2"] = alarm2Array;

    // add JSON reports to JSON mainArray
    JSONVar mainArray;
    mainArray["@timestamp"] = rfc3339;
    mainArray["Temperature_RTC_"+SerialNumber] = (unsigned long) rtc.getTemperature();
    mainArray["Temperature_DHT_"+SerialNumber] = dht.readTemperature();  // Read temperature as Celsius (the default)
    mainArray["Humidity_DHT_"+SerialNumber] = dht.readHumidity();
    mainArray["Moisture_Sensor_1_"+SerialNumber] = sensorValue1;
    mainArray["Moisture_Sensor_2_"+SerialNumber] = sensorValue2;
    mainArray["Moisture_Sensor_3_"+SerialNumber] = sensorValue3;
    mainArray["Moisture_Sensor_4_"+SerialNumber] = sensorValue4;
    mainArray["Moisture_Sensor_Average_"+SerialNumber] = (sensorValue1+sensorValue2+sensorValue3+sensorValue4)/4;
    mainArray["Cycle_"+SerialNumber] = wateringPeriod;
    mainArray["Dry_"+SerialNumber] = limitWarn;
    mainArray["Optimum_"+SerialNumber] = limitMid;
    mainArray["Wet_"+SerialNumber] = limitWet;
    mainArray["Alarm1_"+SerialNumber] = alarm1Array;
    mainArray["Alarm2_"+SerialNumber] = alarm2Array;

    //mainArray["Time"] = timeArray;
    //mainArray["Alarm"] = alarmArray;
    //mainArray["Climate"] = climateArray;
    //mainArray["Soil"] = soilArray;
    
    Serial.print(mainArray);
    Serial.println();
////// End of JSON output //////
}

void JSONresponder(String stringJSON){
    // Responses:
    // 0) error JSON - when input is invalid
    // 1) trigger irrigation - when JSON input has value... 
    // 2) output JSON report - when JSON input has value...
    // 3) ...

    // remove any \r \n whitespace at the end String
    stringJSON.trim();
    // parse String to JSON
    JSONVar testJSON = JSON.parse(stringJSON);
    // Hard-coded 128-byte-password cast to String in JSON form <- for JSON key pattern matching
    String privateString = "{\"pass\": \"3fb77e036133e2fd69f8510d35ca86342cd2b9872594bbf4645e378f8c4a2b3e27700c75a6ac228278193f129bef2cf47ba03f08a1e93656a939491ca26ece50\"}";
    // parse String to JSON
    JSONVar privateJSON = JSON.parse(privateString);
    // load array of keys from testJSON
    JSONVar keys = testJSON.keys();
    // extract 0th key and stringify value <- value
    String value = JSON.stringify(testJSON[keys[0]]);
    // load array of keys from privateJSON <- reuse keys Var
    keys = privateJSON.keys();
    // extract 0th key value and stringify value <- pass
    String pass = JSON.stringify(privateJSON[keys[0]]);

    // evaluate if value is the same as pass and run report if matching
    if (value == pass ) {
        ////// activate the soil moisture sensor //////
        digitalWrite(sensorPowerPin, HIGH);
        // wait measuringPeriod ms for soil moisture sensor to take measurement 
        responsivedelay(measuringPeriod);
        sensorValue1 = analogRead(sensorPin1);
        sensorValue2 = analogRead(sensorPin2);
        sensorValue3 = analogRead(sensorPin3);
        sensorValue4 = analogRead(sensorPin4);
        //sensorValue = digitalRead(sensorPin);
        /////// end of soil moisture sensor activation //////
        sensorReport();
        // Make sure to power down the soil moisture sensor. 
        digitalWrite(sensorPowerPin, LOW);
        //Serial.flush();
        return;
    } 
    if (value != pass && value != 0) {
         String openJSON = "{\"invalidPassword\": \""; 
         String closeJSON = "\"}";
         String formedJSON = openJSON+value+closeJSON;
         Serial.print(formedJSON);
        //
        //Serial.print("{\"nullCommand\": \"");
        //Serial.flush();
        //Serial.print(stringJSON);
        //Serial.flush();
        //Serial.print("\"}");
        //Serial.flush();
        return;    
    }
    // will execute at ms intervals
    else {
    Serial.flush();
    return;
    }
}

void responsivedelay(unsigned long timeoutPeriod) {
    DateTime timeoutPeriodEndDate = rtc.now();
    unsigned long timeoutPeriodSeconds = timeoutPeriod / 1000;
    volatile unsigned long timeoutPeriodEnd = timeoutPeriodEndDate.unixtime() + timeoutPeriodSeconds;
    //Serial.print(timeoutPeriodEnd);
    // change unsigned long timeoutPeriod timePeriod from the usual ms to seconds for calculations
    timeoutPeriodDecay = timeoutPeriodSeconds;
    //Serial.print(timeoutPeriodDecay);

    while (timeoutPeriodDecay > 0) {
        // decay timeout period
        DateTime decayDate = rtc.now();
        timeoutPeriodDecay = timeoutPeriodEnd - decayDate.unixtime();
        //Serial.print(timeoutPeriodDecay);
        Serial.setTimeout(timeoutPeriodDecay);
        String serialString = Serial.readString();
        // initial response to input is to send back the same data as a confirmation <- should be packed in JSON format
        //Serial.print(serialString);
        //////
        ///      This is our opportunity to ingest data and perform actions
        ///          - this could include setting timeout to 0 to start loop or trigger irrigation
        JSONresponder(serialString);
        //////  
       }

    //reset timeoutPeriodDecay
    timeoutPeriodDecay = 0;
}
