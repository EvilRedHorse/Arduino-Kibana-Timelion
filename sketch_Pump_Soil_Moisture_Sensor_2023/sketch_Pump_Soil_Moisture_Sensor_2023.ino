// load Arduino_JSON to use JSON format.
#include <Arduino_JSON.h> // include Arduino_JSON library
#include "Arduino_JSON.h" // include Arduino_JSON header 

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"
RTC_DS3231 rtc;
// define days of the week, for serial console display, etc.
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
volatile unsigned int roundYear;
String roundMonth;
String roundDay;
String roundHour;
String roundMinute;
String roundSecond;

#include <DHT.h> // include DHT library 
#include "DHT.h" // include DHT header
volatile float h;
volatile float t;
#define DHTPIN 38     // what pin we're connected to
#define DHTTYPE DHT11   // we are using the DHT11 sensor
DHT dht(DHTPIN, DHTTYPE); // initialize DHT on pin and choose DHT sensor type

#define sensorPin A0
// using correct data types for longer delay periods is critical
unsigned int boardLEDPin= 13;
unsigned int wateringPeriod= 60000;
// cast timeoutPeriodDecay for use in responsivedelay() function
volatile unsigned long timeoutPeriodDecay = 1;
// Wikipedia: "the most sensitive games ... achieve response times of 67 ms (excluding display lag)"
// this delay() interval is used within responsivedelay() to decrease while loop intervals and to not create lag
// *without this interval, Arduino will check as often as possible to execute the while loop.
unsigned int timeoutPeriodDecayInterval = 67;
volatile unsigned long pollingPeriod = 3600000;
//volatile unsigned int pollingPeriod = 36000;
unsigned int measuringPeriod = 1000;
unsigned int sensorValue;
unsigned int sensorPowerPin = 43;
// lowering threshold for capacitive sensor 
unsigned int limitWarn = 640;
unsigned int limitMid = 448;
unsigned int limitWet = 256;
unsigned int pumpPowerPin = 45;
unsigned int bluePin = 49;
unsigned int redPin = 51;
unsigned int greenPin = 53;

void setup() {

    // DHT Sensor - take an initial reading before serial is started
    pinMode(DHTPIN, INPUT);
    // Start the DHT sensor
    dht.begin();
    // Reading temperature or humidity <- minimum: 250 ms
    delay(measuringPeriod);

    // Start serial at 57600 baud rate
    // bash: `screen /dev/ttyUSB0 57600`
    Serial.begin(57600);

    #ifndef ESP8266
        while (!Serial); // wait for serial port to connect. Needed for native USB
    #endif

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    // Soil Moisture Sensor
    pinMode(sensorPin, INPUT);

    // Soil Moisture Sensor Power Switch
    pinMode(sensorPowerPin, OUTPUT);

    // Water Pump - with pump off
    digitalWrite(pumpPowerPin, HIGH);
    pinMode(pumpPowerPin, OUTPUT);

    // Blue LED
    pinMode(bluePin, OUTPUT);

    // Red LED
    pinMode(redPin, OUTPUT);

    // Green LED
    pinMode(greenPin, OUTPUT);

    // turn off the onboard LED.
    digitalWrite(boardLEDPin, LOW);
    pinMode(boardLEDPin, OUTPUT);
}

void loop() {

    /// Our main loop() is used for constant plant monitoring

    ////// activate the soil moisture sensor //////
    digitalWrite(sensorPowerPin, HIGH);
    // wait measuringPeriod ms for soil moisture sensor to take measurement 
    responsivedelay(measuringPeriod);
    sensorValue = analogRead(sensorPin);
    //sensorValue = digitalRead(sensorPin);
    /////// end of soil moisture sensor activation //////

    // Blue LED - indicate wet soil moisture
    if (sensorValue<limitWet) {
        LEDcycle(bluePin);
    }
    // No LED - indicate above average soil moisture
    if ((sensorValue>limitWet) && (sensorValue<=limitMid)) {
        LEDcycle(boardLEDPin);
    }
    // Green LED - indicate optimum soil moisture
    if ((sensorValue>limitMid) && (sensorValue<=limitWarn)) {
        LEDcycle(greenPin);
    }
    // Red LED - indicate dry soil and start irrigation
    if (sensorValue>limitWarn) {
        irrigationcycle(wateringPeriod);
    }

    ////// Safety Power-down <- begins //////
    // Make sure to power down the soil moisture sensor. 
    digitalWrite(sensorPowerPin, LOW);
    // Make sure to power down pump.
    digitalWrite(pumpPowerPin, HIGH);
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

void irrigationcycle(unsigned int irrigationPeriod) {
// turning on LED for wateringPeriod ms
    digitalWrite(redPin, HIGH);
    // power down the soil moisture sensor before any watering takes place. 
    digitalWrite(sensorPowerPin, LOW);
    // water for wateringPeriod ms using pin pumpPowerPin, bringing low when finished. 
    digitalWrite(pumpPowerPin, LOW);
    responsivedelay(irrigationPeriod);
    digitalWrite(redPin, LOW);
    digitalWrite(pumpPowerPin, HIGH);}

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

    JSONVar climateArray;
    climateArray["Temperature_RTC"] = (unsigned long) rtc.getTemperature();
    climateArray["Temperature_DHT"] = dht.readTemperature();  // Read temperature as Celsius (the default)
    climateArray["Humidity_DHT"] = dht.readHumidity();

    JSONVar soilArray;
    soilArray["Moisture"] = sensorValue;
    soilArray["Cycle"] = wateringPeriod;

    // add JSON reports to JSON mainArray
    JSONVar mainArray;
    mainArray["@timestamp"] = rfc3339;
    mainArray["Temperature_RTC"] = (unsigned long) rtc.getTemperature();
    mainArray["Temperature_DHT"] = dht.readTemperature();  // Read temperature as Celsius (the default)
    mainArray["Humidity_DHT"] = dht.readHumidity();
    mainArray["Moisture"] = sensorValue;
    mainArray["Cycle"] = wateringPeriod;
    mainArray["Alarm1"] = alarm1Array;
    mainArray["Alarm2"] = alarm2Array;

    //mainArray["Time"] = timeArray;
    //mainArray["Alarm"] = alarmArray;
    //mainArray["Climate"] = climateArray;
    //mainArray["Soil"] = soilArray;
    
    Serial.print(mainArray);
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
    String privateString = "{\"pass\": \"TJxgrWlh8vtakYdIkK+Vq+XJ+tJcc1792zyrO2ZjCNfNmgjavSz24OztNjeVRNMp28K5vzoyiDZO+98xES2MFJXOZvjAIXQ3LUYC4VM7h5XOgH/Sae0Lhz1nfedenhCOA1g/JGNjtb8XUGrBsG/rc3jW39NmhWns5FzZqoPM8pI=\"}";
    // parse String to JSON
    JSONVar privateJSON = JSON.parse(privateString);
    // load array of keys from testJSON
    JSONVar keys = testJSON.keys();
    // extract 0th key and stringify value <- value
    String value = JSON.stringify(testJSON[keys[0]]);
    // load array of keys from privateJSON <- reuse keys Var
    keys = privateJSON.keys();
    // extract 0th key value and strinify value <- pass
    String pass = JSON.stringify(privateJSON[keys[0]]);

    // evaluate if value is the same as pass and run report if matching
    if (value == pass ) {
        ////// activate the soil moisture sensor //////
        digitalWrite(sensorPowerPin, HIGH);
        // wait measuringPeriod ms for soil moisture sensor to take measurement 
        responsivedelay(measuringPeriod);
        sensorValue = analogRead(sensorPin);
        //sensorValue = digitalRead(sensorPin);
        /////// end of soil moisture sensor activation //////
        sensorReport();
        // Make sure to power down the soil moisture sensor. 
        digitalWrite(sensorPowerPin, LOW);
        return;
    } 
    if (value != pass && value != 0) {
         String openJSON = "{\"invalidPassword\": \""; 
         String closeJSON = "\"}";
         String formedJSON = openJSON+value+closeJSON;
         Serial.print(formedJSON);
        //
        //Serial.print("{\"nullCommand\": \"");
        //Serial.print(stringJSON);
        //Serial.print("\"}");
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
