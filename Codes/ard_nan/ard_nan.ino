/********************************************************************
        BLIND CURVE WARNING SYSTEM
        ARDUINO NANO CONTROLLER

Functions
---------
✓ Curve Ultrasonics
✓ LED Controller
✓ Buzzer
✓ I2C Slave
✓ Vehicle Passed Detection
********************************************************************/

#include <Wire.h>

#define I2C_ADDRESS 8

//===========================
// Upper Curve Ultrasonic
//===========================

#define TRIG_UP 2
#define ECHO_UP 3

//===========================
// Lower Curve Ultrasonic
//===========================

#define TRIG_DOWN 4
#define ECHO_DOWN 5

//===========================
// LEDs
//===========================

#define UP_GREEN 6
#define UP_RED 7

#define DOWN_GREEN 8
#define DOWN_RED 9

//===========================
// Buzzer
//===========================

#define BUZZER 10

//===========================
// ESP8266 Signal Pins
//===========================

#define UP_PASS 11
#define DOWN_PASS 12

//===========================
// Variables
//===========================

byte currentState = 0;

bool upperDetected = false;
bool lowerDetected = false;

unsigned long buzzerTimer = 0;
bool buzzerState = false;

//===========================
// Function Prototypes
//===========================

float getDistance(int trigPin,int echoPin);

void updateOutputs();

void receiveEvent(int bytes);

void checkCurveSensors();

void pulseUP();

void pulseDOWN();

/********************************************************************
                        SETUP
********************************************************************/

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_UP,OUTPUT);
  pinMode(ECHO_UP,INPUT);

  pinMode(TRIG_DOWN,OUTPUT);
  pinMode(ECHO_DOWN,INPUT);

  pinMode(UP_GREEN,OUTPUT);
  pinMode(UP_RED,OUTPUT);

  pinMode(DOWN_GREEN,OUTPUT);
  pinMode(DOWN_RED,OUTPUT);

  pinMode(BUZZER,OUTPUT);

  pinMode(UP_PASS,OUTPUT);
  pinMode(DOWN_PASS,OUTPUT);

  digitalWrite(UP_PASS,HIGH);
  digitalWrite(DOWN_PASS,HIGH);

  Wire.begin(I2C_ADDRESS);

  Wire.onReceive(receiveEvent);

  Serial.println();
  Serial.println("===========================");
  Serial.println("Nano Controller Ready");
  Serial.println("===========================");

  updateOutputs();
}



/********************************************************************
                        LOOP
********************************************************************/

void loop()
{
  checkCurveSensors();

  updateOutputs();
}

/********************************************************************
                    RECEIVE I2C COMMAND
********************************************************************/
void receiveEvent(int howMany)
{
  if(Wire.available())
  {
    currentState = Wire.read();

    Serial.print("Received State : ");
    Serial.println(currentState);
  }
}



/********************************************************************
                    UPDATE OUTPUTS
********************************************************************/
void updateOutputs()
{
  switch(currentState)
  {
    //================================================
    // SAFE
    //================================================
    case 0:

      digitalWrite(UP_GREEN,HIGH);
      digitalWrite(UP_RED,LOW);

      digitalWrite(DOWN_GREEN,HIGH);
      digitalWrite(DOWN_RED,LOW);

      digitalWrite(BUZZER,LOW);

      break;



    //================================================
    // ABOVE VEHICLE ONLY
    //================================================
    case 1:

      digitalWrite(UP_GREEN,HIGH);
      digitalWrite(UP_RED,LOW);

      digitalWrite(DOWN_GREEN,LOW);
      digitalWrite(DOWN_RED,HIGH);

      digitalWrite(BUZZER,LOW);

      break;



    //================================================
    // BELOW VEHICLE ONLY
    //================================================
    case 2:

      digitalWrite(UP_GREEN,LOW);
      digitalWrite(UP_RED,HIGH);

      digitalWrite(DOWN_GREEN,HIGH);
      digitalWrite(DOWN_RED,LOW);

      digitalWrite(BUZZER,LOW);

      break;



    //================================================
    // BOTH SIDES
    //================================================
    case 3:

      digitalWrite(UP_GREEN,LOW);
      digitalWrite(UP_RED,HIGH);

      digitalWrite(DOWN_GREEN,HIGH);
      digitalWrite(DOWN_RED,LOW);

      if(millis()-buzzerTimer>=500)
      {
        buzzerTimer=millis();

        buzzerState=!buzzerState;

        digitalWrite(BUZZER,buzzerState);
      }

      break;
  }

}
/********************************************************************
                    DISTANCE
********************************************************************/
float getDistance(int trigPin,int echoPin)
{
  digitalWrite(trigPin,LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin,HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin,LOW);

  long duration=pulseIn(echoPin,HIGH,30000);

  if(duration==0)
      return 999;

  return duration*0.0343/2.0;
}



/********************************************************************
                CHECK CURVE SENSORS
********************************************************************/
void checkCurveSensors()
{
  float upDistance=getDistance(TRIG_UP,ECHO_UP);

  float downDistance=getDistance(TRIG_DOWN,ECHO_DOWN);

  if(upDistance<15 && !upperDetected)
  {
      upperDetected=true;

      Serial.println("Upper Vehicle Passed");

      pulseUP();
  }

  if(upDistance>20)
      upperDetected=false;



  if(downDistance<15 && !lowerDetected)
  {
      lowerDetected=true;

      Serial.println("Lower Vehicle Passed");

      pulseDOWN();
  }

  if(downDistance>20)
      lowerDetected=false;

}
/********************************************************************
                PULSE TO ESP8266
********************************************************************/
void pulseUP()
{
    digitalWrite(UP_PASS,LOW);

    delay(100);

    digitalWrite(UP_PASS,HIGH);
}



/********************************************************************
                PULSE TO ESP8266
********************************************************************/
void pulseDOWN()
{
    digitalWrite(DOWN_PASS,LOW);

    delay(100);

    digitalWrite(DOWN_PASS,HIGH);
}
