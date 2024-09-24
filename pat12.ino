/*
Pressure Project - Creation & Computation - OCADU

Paul Van Rijn - 2024-09-19

Encased in a cardboard hat.

Read a presure sensor from the C&C sensor workshop
look for "nod" and "lean" patterns from the hat wearer.
display light patterns for  on patterns detected from the sensor.

Input sensor data smoothing ideas from the Arduino formums.

*/
// Global variables - needed to hold state for patterns and light displays.
int sensor_pin = A7;  // Analog input pin for the pressure sensor
int longSmoothSensor;
float longSmooth = 30.0;      //number of samples to smooth/average for the long average
float longRunningAverage = 0.0;
float longRunningTotal = 0.0;
float shortSmooth = 5.0;      //number of samples to smooth/average for the short average
float shortRunningAverage = 0.0;
float shortRunningTotal = 0.0;
int shortSmoothSensor;

unsigned long myTime;
unsigned long startTime;

// bump pattern detection states
#define B_SEEK 0 
#define B_RUN 1

// blinkenlites
#define RD_PIN 2 
#define BL_PIN 3
#define WT_PIN 5

// lean detection states
#define L_START 1 
#define L_RUN 2

int leanOn = 0;
int leanColour;
int leanState = L_START;
unsigned long int leanStartTime = 0;

// bump detection threshold
int bumpThresh = 12;  // set to just exceed the usual noise
unsigned long int bumpMinDuration = 300; // set these to determine the duration of a nod
unsigned long int bumpMaxDuration = 1000; 
unsigned long int bumpStartTime = 0;
int bumpState = B_SEEK;

int intense = 128; // intensity of the response
int turnOn = 1;    // light flash flip flop
unsigned long int nextTime = 0;

int gap; // difference between the long and short smoothings 

int gotabump = 0;  
unsigned long int bumpDuration;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  pinMode(sensor_pin, INPUT);
  pinMode(RD_PIN, OUTPUT);
  pinMode(BL_PIN, OUTPUT);
  pinMode(WT_PIN, OUTPUT);
}

void loop() {

  // Read the analog value from the pressure sensor
  int sensorValue = analogRead(sensor_pin);
  // smooth the input
  longRunningTotal = (longRunningTotal - longRunningAverage) + sensorValue; // include this reading with the agerage
  longRunningAverage = (longRunningTotal/longSmooth);
  longSmoothSensor = int(longRunningAverage);
  // short of it
  shortRunningTotal = (shortRunningTotal - shortRunningAverage) + sensorValue; // include this reading with the agerage
  shortRunningAverage = (shortRunningTotal/shortSmooth);
  shortSmoothSensor = int(shortRunningAverage);
  // get the current time
  myTime =  millis();
// the gap uses the long running average as a continuous calibraion
// the short smoothing is a "cleaned up" input signal
// the smoothing lengths were determined by testing
  gap = shortSmoothSensor - longSmoothSensor; 
// duration is used to time the duration of the bump motion
  bumpDuration = myTime - bumpStartTime;

// assess the smoothed inputs to determine if there has been a bump/nod
  switch(bumpState){
    case B_SEEK: // starting a bump
      if (gap > bumpThresh) {
        bumpState = B_RUN;
        bumpStartTime = myTime;
        gotabump = 0;
      }
      break;
    case B_RUN: // checking for the end of a bump
      if (abs(gap) < bumpThresh && bumpDuration < bumpMinDuration) {
        // dropped below threshold
        // start over
        bumpState = B_SEEK;
        // exit no bump - too short
        gotabump = 0;
        break;
      }
        // exit, no bump - too long
      if (bumpDuration > bumpMaxDuration) {
          gotabump = 0;
          bumpState = B_SEEK;
          break;
      }
      if (abs(gap) < bumpThresh && bumpDuration > bumpMinDuration) {
        // exit with bump
         gotabump = 1;
         bumpState = B_SEEK;
         break;
      }
      gotabump = 0; // shouldn't get here...
      break;
  }
  // increase the intensity for multiple nods
  if (gotabump > 0) {
    intense = intense + 75;
    if (intense > 255) {
      intense = 255;
    }
    gotabump = 0;
  }

// detect a lean
  switch(leanState){ 
    case(L_START): // start a lean detection
      if (abs(gap) > bumpThresh) {
        leanState = L_RUN;
        leanStartTime = myTime;
        leanOn = 0;
      }
      break;
    case(L_RUN): // look for a gap above the threshold and longer than a nod
      if (abs(gap) > bumpThresh && myTime > (bumpMaxDuration + leanStartTime)) {
         leanOn = 1;
         if (gap < 0) { //different colours for back or fore leans - this shouldn't be in the detector!!!
           leanColour = RD_PIN;
         } else {
           leanColour = BL_PIN;
         }
      } else {
        if (abs(gap) < bumpThresh) { // dropped below threshold so start over
          leanOn = 0;
          leanState = L_START;
        }
      }
    }

  // slowly decrease the intensity if there has been no nodding
  intense = intense - 1;
  if (intense < 25) { // minimum intensity, which is pretty dull
    intense = 25;
  }

// display based on the current state of bumps and leans
  if (nextTime < myTime) {
    if (turnOn == 1) {
      if (leanOn) {
        analogWrite(leanColour, 200);
      } else {
        analogWrite(RD_PIN, intense);
        analogWrite(BL_PIN, 0);
        analogWrite(WT_PIN, intense);
      }
      turnOn = 0;
    } else {
      if (leanOn) {
        analogWrite(leanColour, 0);
      } else {
        analogWrite(RD_PIN, 0);
        analogWrite(BL_PIN, intense);
        analogWrite(WT_PIN, 0);
      }
      turnOn = 1;
    }
    // flash rate depends on recent nods or leans
    if (leanOn) {
      nextTime = myTime + 128;
    } else {
      nextTime = myTime + (800 - (intense*3)); // flash at the inverse of the intensity making higher faster
    }
  }
// print sensor data - looks great on the "Serial Plotter" tool
  Serial.print(sensorValue);
  Serial.print("  ");
  Serial.print(longSmoothSensor);
  Serial.print("  ");
  Serial.println(shortSmoothSensor);
  delay(30);
}// end loop()

