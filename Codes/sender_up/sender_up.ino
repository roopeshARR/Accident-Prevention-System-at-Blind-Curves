#include <SPI.h>
#include <LoRa.h>

#define SS      5
#define DIO0    4

// U1 - Lower Detection Sensor
#define TRIG1   14
#define ECHO1   27

// U2 - Upper Height Sensor
#define TRIG2   26
#define ECHO2   25

// U3 - Speed Sensor
#define TRIG3   33
#define ECHO3   32

#define SENSOR_DISTANCE 0.20      // Distance between U1 and U3 (20 cm)
#define CURVE_DISTANCE 1.0      // Distance from sensor to curve (meters)
#define VEHICLE_TIMEOUT 5000    // Max ms to wait for speed sensor before auto-resetting

bool vehiclePresent = false;

unsigned long t1 = 0;
unsigned long t2 = 0;

String vehicleType = "";

float getDistance(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0)
    return 999;

  return duration * 0.0343 / 2.0;
}

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);

  LoRa.setPins(SS, -1, DIO0);

  if (!LoRa.begin(433E6))
  {
    Serial.println("LoRa Initialization Failed!");
    while (1);
  }

  // Must match ESP8266 Receiver
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println();
  Serial.println("=================================");
  Serial.println("UPPER ESP32 NODE READY");
  Serial.println("=================================");
}

void loop()
{
  float lowerDist = getDistance(TRIG1, ECHO1);
  float upperDist = getDistance(TRIG2, ECHO2);
  float speedDist = getDistance(TRIG3, ECHO3);

  Serial.print("Lower : ");
  Serial.print(lowerDist);
  Serial.print(" cm   ");

  Serial.print("Upper : ");
  Serial.print(upperDist);
  Serial.print(" cm   ");

  Serial.print("Speed Sensor : ");
  Serial.print(speedDist);
  Serial.println(" cm");
  if (vehiclePresent && (millis() - t1 > VEHICLE_TIMEOUT))
  {
  Serial.println("Vehicle timeout - resetting detection");
  vehiclePresent = false;
  }
  if (lowerDist < 15 && !vehiclePresent)
  {
    vehiclePresent = true;
    t1 = millis();

    if (upperDist < 15)
      vehicleType = "HEAVY";
    else
      vehicleType = "LIGHT";

    Serial.println();
    Serial.println("----------------------------");
    Serial.println("Vehicle Detected");
    Serial.print("Vehicle Type : ");
    Serial.println(vehicleType);
    Serial.println("----------------------------");
  }

  if (vehiclePresent && speedDist < 15)
  {
    t2 = millis();

    float timeSec = (t2 - t1) / 1000.0;

    if (timeSec > 0)
    {
      float speed_mps = SENSOR_DISTANCE / timeSec;
      float speed_kmph = speed_mps * 3.6;

      String packet =
        "UP," +
        vehicleType +
        "," +
        String(speed_kmph, 1) +
        "," +
        String((int)CURVE_DISTANCE);

      Serial.println();
      Serial.println("******** VEHICLE DATA ********");

      Serial.print("Type          : ");
      Serial.println(vehicleType);

      Serial.print("Time          : ");
      Serial.print(timeSec);
      Serial.println(" sec");

      Serial.print("Speed         : ");
      Serial.print(speed_kmph);
      Serial.println(" km/h");

      Serial.print("Distance      : ");
      Serial.print(CURVE_DISTANCE);
      Serial.println(" m");

      Serial.print("LoRa Packet   : ");
      Serial.println(packet);
      Serial.print("Sending: ");
      Serial.println(packet);
      LoRa.beginPacket();
      LoRa.print(packet);
      int status = LoRa.endPacket();
      Serial.print("endPacket() returned: ");
      Serial.println(status);

      Serial.print("LoRa TX       : ");

      if (status == 1)
        Serial.println("SUCCESS");
      else
        Serial.println("FAILED");

      Serial.println("******************************");
      Serial.println();

      vehiclePresent = false;

      delay(1000);
    }
  }

  delay(100);
}