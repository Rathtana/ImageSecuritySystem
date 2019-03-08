#include <LiquidCrystal.h>

//For PIR Sensor
#define PIR_LEAVING 24
#define PIR_ENTER 32

#define BUZZER 22
#define LED_ARLARM 43

//Ultrosonic sensor 1
#define TRIG_PIN 4
#define ECHO_PIN 5

//Ultrosonic sensor 2
#define TRIG_PIN2 12
#define ECHO_PIN2 13

//human detected within this range
//10cm for now
#define HUMAN_RANGE_CM 40

int photocellPin = 13;     // the cell and 10K pulldown are connected to a0
int photocellReading;     // the analog reading from the analog resistor divider

//for Ultrosonic sensor 1
int distance1_cm;
bool curState1 = false;

//for Ultrosonic sensor 2
int distance2_cm;
bool curState2 = false;

//Keep track of the number of people
int headCount = 0;

//Keep track of the state of PIR sensor
int sensorEnter = 0;
int sensorLeaving = 0;


int messageNumber = 0;  //keep track on message number
int recieveCode = 0;  //code from the host

bool onPeriodic = true; //true = periodic mode
unsigned long startTime_ms, endTime_ms;
unsigned long periodicTime_ms = 10000; //default for 20 seconds

//security mode
bool securityMode = false;

bool connectedToWifi = false;

int tempPin = 10; //the analog pin the TMP36's Vout (sense) pin is connected to

float temperatureF = 0;

// initialize the library by associating any needed LCD interface pin
//(RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(33, 31, 29, 27, 25, 23);

bool arlarmStatus = false;

float samplingRate_hz = 10; //set default sampling rate = 10Hz = 100ms

String message;
String pirSensor;
String photoSensor;
String distanceSensor;
String result;

int recieveCount = 0;
void distanceSensorUpdate(bool someoneEnter) {
  messageNumber++;
  message = ';' + String(messageNumber) + ';';
  result = someoneEnter + message + String(headCount);
  Serial1.print(result);
}

void sensorInformation() {
  messageNumber++;
  message += String(messageNumber) + ';';
  pirSensor = String(sensorEnter) + ';' + String(sensorLeaving) + ';';
  photoSensor = String(photocellReading) + ';';
  distanceSensor =  String(distance1_cm) + ';' + String(distance2_cm) + ';';

  result = message + pirSensor + distanceSensor + photoSensor + String(headCount);
  Serial1.print(result);
}

int getDistance(int trigPin, int echoPin) {
  //making sure the trigPin is cleared before sending
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  //send ultrosonic sound wave for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  //set trigpin back to low
  digitalWrite(trigPin, LOW);

  //counting the duration for the echo pin to go from HIGH to LOW
  //return in microsecond
  int duration = pulseIn(echoPin, HIGH);

  //return the distance in cm
  //d = time * v (speed of sound)
  return duration * 0.034 / 2;
}

void startCountingPeople() {

  //getting distance for the 2 sensors
  distance1_cm = getDistance(TRIG_PIN, ECHO_PIN);
  distance2_cm = getDistance(TRIG_PIN2, ECHO_PIN2);

  //Checking for entering the room
  if (distance1_cm <= HUMAN_RANGE_CM && distance1_cm > 0)
    curState1 = true;
  else
    curState1 = false;

  //if detect someone coming in
  if (curState1) {
    int noiseTriggered1 = 0;
    while (curState1 && noiseTriggered1 < 50L) {
      noiseTriggered1++;
      distance2_cm = getDistance(TRIG_PIN2, ECHO_PIN2);
      if (distance2_cm <= HUMAN_RANGE_CM) {
        curState2 = true;
        //Serial.println("setting true for curState2");
      }
      else
        curState2 = false;
      if (curState2) {
        headCount++; //increament human count

        if (securityMode) {
          arlarmStatus = true;
        }
        //send new headCount to client
        //someone enter
        distanceSensorUpdate(1);
        lcd.setCursor(4, 3); //line 4
        lcd.print("head Count: ");
        lcd.print(headCount);
        Serial.print("head Count: ");
        Serial.println(headCount);
        delay(1500);
        return; //reset and collect again
      }
    }
  }

  if (distance2_cm <= HUMAN_RANGE_CM && distance2_cm > 0)
    curState2 = true;
  else
    curState2 = false;

  //if detect someone leaving room
  if (curState2) {
    int noiseTriggered2 = 0;
    while (curState2 && noiseTriggered2 < 50) {
      noiseTriggered2++;
      distance1_cm = getDistance(TRIG_PIN, ECHO_PIN);
      //      Serial.print("distance1: ");
      //      Serial.println(distance1_cm);
      if (distance1_cm <= HUMAN_RANGE_CM)
        curState1 = true;
      else
        curState1 = false;

      if (curState1) {
        headCount--; //decrement human count
        //send new headCount client
        distanceSensorUpdate(0);
        lcd.setCursor(4, 3); //line 4
        lcd.print("head Count: ");
        lcd.print(headCount);
        Serial.print("head Count: ");
        Serial.println(headCount);
        delay(1500); //wait for the person to pass through to reset
        return; //reset and collect again
      }
    }
  }

}

void getTempData() {
  //getting the voltage reading from the temperature sensor
  int reading = analogRead(tempPin);

  // converting that reading to voltage, for 3.3v arduino use 3.3
  float voltage = reading * 5.0;
  voltage /= 1024.0;

  // now print out the temperature
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
  // now convert to Fahrenheit
  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
}

void checkArlarm() {
  //on arlarm for security and fire
  if (arlarmStatus || temperatureF >= 100) {
    digitalWrite(LED_ARLARM, HIGH);
    tone(BUZZER, 2500);
    delay(200);
    digitalWrite(LED_ARLARM, LOW);
    noTone(BUZZER);     // Stop sound...
    delay(200);
  }
}

void checkPeriodicMode() {
  if (onPeriodic) {
    endTime_ms = millis(); //get an end time
    //periodic cycle
    if (abs(endTime_ms - startTime_ms) >= periodicTime_ms) {
      startTime_ms = endTime_ms; //start timer again for next period
      message = "1;";
      sensorInformation();  //send sensor data periodically.
    }

  }
}


void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_ARLARM, OUTPUT);

  //defining pin for Ultrosonic sensor 1
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  //defining pin for Ultrosonic sensor 2
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  //defining PIR Sensors
  pinMode(PIR_ENTER, INPUT);
  pinMode(PIR_LEAVING, INPUT);

  //Initialzie the LCD
  lcd.begin(16, 4);

  //getting starting time for periodic data
  startTime_ms = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected to Wifi");
}

void loop() {
  //read data from host -> client-> mega if any
  //checkForDataFromHost();
  if (Serial1.available()) {
    String messageRead = Serial1.readString();
    messageRead.trim(); //exlude whitespaces
    //    Serial.println(messageRead);
    if (messageRead.length() == 1) {
      recieveCode = messageRead[0] - 48;
      if (recieveCode == 0) {
        //        Serial.println("Sever recieved message");

        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);
      }
      //get on demand data
      else if (recieveCode == 1) {
        //        Serial.println("on demand was requested"); //to the lcd
        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);

        message = "0;";
        sensorInformation();
      }
      //go back to periodic data
      else if (recieveCode == 2) {
        //        Serial.println("Enable periodic data");
        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);

        onPeriodic = true;
      }
      else if (recieveCode == 3) {
        //        Serial.println("Disable periodic data");
        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);

        onPeriodic = false;
      }
      else if (recieveCode == 4) {
        //        Serial.println("Security mode on");
        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);

        securityMode = true;

      }
      else if (recieveCode == 5) {
        //print to lcd
        //        Serial.println("Security mode off");
        lcd.setCursor(0, 1); //line 2
        lcd.print("Message recieved ");
        lcd.print(++recieveCount);

        securityMode = false;
        arlarmStatus = false;
      }
    }
    else { //configuring the sampling rate for sensors
      int lengthOfMessage = messageRead.length();
      if (messageRead[lengthOfMessage - 1] == 'R') { //check last index for the correct code
        messageRead.remove(lengthOfMessage - 1); //get ride of the last character
        if (messageRead.toFloat()) { //check if you can convert to float value
          float sampVal = messageRead.toFloat();
          if (sampVal >= 0.1 && sampVal <= 10) {
            //            Serial.println("Got valid sample rate");
            //            Serial.println(sampVal);
            samplingRate_hz = sampVal; //update the new sampling rate
            lcd.setCursor(0, 1); //line 2
            lcd.print("Message recieved ");
            lcd.print(++recieveCount);
          }
        }
      }
    }
  }
  //get reading from photocell
  photocellReading = analogRead(photocellPin);  
  
  //on arlarm if someone walked in during security mode on
  checkArlarm();

  //Trigger Sensors to count people
  if (sensorEnter == 1 || sensorLeaving == 1 || analogRead(photocellPin) > 300) {
    startCountingPeople(); //count people when detect motion with PIR
  }

  //periodic mode if enable
  checkPeriodicMode();

  //Sampling rate in ms for sensors
  delay((1.0 / samplingRate_hz) * 1000); //sampling rate in ms
}



