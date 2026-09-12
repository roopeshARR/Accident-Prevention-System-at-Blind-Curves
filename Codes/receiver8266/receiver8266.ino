/********************************************************************
      BLIND CURVE WARNING SYSTEM
      ESP8266 MAIN CONTROLLER

Author : Roopesh Reddy
Version : V2.0

Functions
---------
✓ LoRa Receiver
✓ Vehicle Queue
✓ ETA Calculation
✓ Dual LCD
✓ Priority Logic
✓ Nano Communication
✓ Buzzer
✓ LEDs
✓ Smooth Scrolling
********************************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>



//===========================
// LCD
//===========================

LiquidCrystal_I2C lcdUp(0x27,16,2);
LiquidCrystal_I2C lcdDown(0x23,16,2);



//===========================
// LoRa
//===========================

#define LORA_SS      15
#define LORA_DIO0    16


#define UP_PASS_PIN D3
#define DOWN_PASS_PIN D4


//===========================
// Nano I2C
//===========================

#define NANO_ADDR    8



//===========================
// System Constants
//===========================

#define MAX_QUEUE           5

#define DEMO_DISTANCE       1.0

#define SCROLL_SPEED        250

#define ETA_UPDATE_TIME     1000

#define LCD_REFRESH         300

#define VEHICLE_TIMEOUT     30000


//===========================
// Vehicle Structure
//===========================

struct Vehicle
{

  bool active;

  char direction[6];

  char type[10];

  float speed;

  float eta;

  unsigned long receivedTime;

};



//===========================
// Queue
//===========================

Vehicle upQueue[MAX_QUEUE];

Vehicle downQueue[MAX_QUEUE];

int upFront=0;
int upRear=0;
int upCount=0;

int downFront=0;
int downRear=0;
int downCount=0;



//===========================
// Display Messages
//===========================

String msgAbove="Vehicle approaching from above";

String msgBelow="Vehicle approaching from below";

String msgSafe="No vehicle approaching";



//===========================
// Scroll Variables
//===========================

int scrollUp=0;

int scrollDown=0;

unsigned long previousScroll=0;



//===========================
// Timers
//===========================

unsigned long previousETA=0;

unsigned long previousLCD=0;



//===========================
// Duplicate Packet Detection
//===========================

String previousPacket="";

unsigned long previousPacketTime=0;



//===========================
// Function Prototypes
//===========================

void receiveLoRa();

void parsePacket(String packet);

void enqueueVehicle(Vehicle v);

void dequeueUP();

void dequeueDOWN();

void updateETA();

void updateDisplays();

void scrollDisplays();

void sendNanoCommand(byte cmd);

void updatePriority();

void removeExpiredVehicles();

void checkCurveSensors();;

void showSafe();

void displayUpper();

void displayLower();

float calculateETA(float speed);

/********************************************************************
                        SETUP
********************************************************************/

void setup()
{
  Serial.begin(115200);

  Wire.begin(D2, D1);
  pinMode(UP_PASS_PIN, INPUT_PULLUP);
  pinMode(DOWN_PASS_PIN, INPUT_PULLUP);
  lcdUp.init();
  lcdUp.backlight();

  lcdDown.init();
  lcdDown.backlight();

  lcdUp.clear();
  lcdDown.clear();

  lcdUp.setCursor(0,0);
  lcdUp.print("Blind Curve");

  lcdUp.setCursor(0,1);
  lcdUp.print("Starting...");

  lcdDown.setCursor(0,0);
  lcdDown.print("Blind Curve");

  lcdDown.setCursor(0,1);
  lcdDown.print("Starting...");

  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println(" Blind Curve Warning System");
  Serial.println("=================================");

  Serial.println("Checking LoRa...");

  LoRa.setPins(LORA_SS,-1,LORA_DIO0);

  if(!LoRa.begin(433E6))
  {
    Serial.println("FAILED");

    lcdUp.clear();
    lcdDown.clear();

    lcdUp.print("LoRa Failed");
    lcdDown.print("LoRa Failed");

    while(1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println("OK");

  Serial.println("Checking LCDs...");
  Serial.println("OK");

  Serial.println("Checking Nano...");

  Wire.beginTransmission(NANO_ADDR);

  if(Wire.endTransmission()==0)
      Serial.println("Nano Connected");
  else
      Serial.println("Nano Not Found");

  Serial.println();

  lcdUp.clear();
  lcdDown.clear();

  lcdUp.print("SYSTEM READY");
  lcdDown.print("SYSTEM READY");

  delay(1000);

  showSafe();

  Serial.println("==============================");
  Serial.println("Receiver Ready");
  Serial.println("==============================");
}



/********************************************************************
                        MAIN LOOP
********************************************************************/

void loop()
{
  receiveLoRa();

  checkCurveSensors();

  if(millis()-previousETA>=ETA_UPDATE_TIME)
  {
    previousETA=millis();

    updateETA();

    removeExpiredVehicles();

    updatePriority();
  }

  if(millis()-previousLCD>=LCD_REFRESH)
  {
    previousLCD=millis();

    updateDisplays();
  }

  if(millis()-previousScroll>=SCROLL_SPEED)
  {
    previousScroll=millis();

    scrollDisplays();
  }

}
/********************************************************************
                    RECEIVE LORA PACKET
********************************************************************/
void receiveLoRa()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    Serial.print("Packet detected! Size = ");
    Serial.println(packetSize);
  }
  if (!packetSize)
    return;

  Serial.print("Packet Size = ");
  Serial.println(packetSize);

  String packet = "";

  while (LoRa.available())
  {
    packet += (char)LoRa.read();
  }

  Serial.print("RAW Packet = [");
  Serial.print(packet);
  Serial.println("]");

  Serial.print("Length = ");
  Serial.println(packet.length());

  Serial.print("RSSI = ");
  Serial.println(LoRa.packetRssi());
  Serial.print("SNR = ");
  Serial.println(LoRa.packetSnr());
  Serial.print("Freq Error = ");
  Serial.println(LoRa.packetFrequencyError());
  Serial.print("SNR = ");
  Serial.println(LoRa.packetSnr());
  // Reject duplicate packet within 2 seconds
  if (packet == previousPacket &&
      millis() - previousPacketTime < 1000)
  {
    Serial.println("Duplicate Packet Ignored");
    return;
  }

  previousPacket = packet;
  previousPacketTime = millis();

  parsePacket(packet);

  updatePriority();
  updateDisplays();
  printQueues();
}


/********************************************************************
                    PARSE PACKET
********************************************************************/
void parsePacket(String packet)
{
  Vehicle v;

  int p1 = packet.indexOf(',');
  int p2 = packet.indexOf(',', p1 + 1);
  int p3 = packet.indexOf(',', p2 + 1);

  if (p1 < 0 || p2 < 0 || p3 < 0)
  {
    Serial.println("Invalid Packet");
    return;
  }

  String dir = packet.substring(0, p1);
  String type = packet.substring(p1 + 1, p2);
  String speedStr = packet.substring(p2 + 1, p3);
  float speed = speedStr.toFloat();

  if (speedStr.length() == 0 || (speed == 0 && speedStr.charAt(0) != '0'))
  {
    Serial.println("Invalid Speed Value - Packet Discarded");
    return;
  }
  dir.toCharArray(v.direction, sizeof(v.direction));
  type.toCharArray(v.type, sizeof(v.type));

  v.speed = speed;
  v.eta = calculateETA(speed);
  v.receivedTime = millis();
  v.active = true;

  enqueueVehicle(v);
  printQueues();

  Serial.println("Vehicle Added To Queue");

  Serial.print("Direction : ");
  Serial.println(v.direction);

  Serial.print("Type : ");
  Serial.println(v.type);

  Serial.print("Speed : ");
  Serial.println(v.speed);

  Serial.print("ETA : ");
  Serial.println(v.eta);
}


/********************************************************************
                    ENQUEUE
********************************************************************/
void enqueueVehicle(Vehicle v)
{
  if (strcmp(v.direction, "UP") == 0)
  {
    if (upCount >= MAX_QUEUE)
    {
    Serial.println("UP Queue Full");
    Serial.println("Removing Oldest Vehicle");

    dequeueUP();
    }

    upQueue[upRear] = v;

    upRear++;

    if (upRear >= MAX_QUEUE)
      upRear = 0;

    upCount++;

    return;
  }

  if (strcmp(v.direction, "DOWN") == 0)
  {
    if (downCount >= MAX_QUEUE)
    {
    Serial.println("DOWN Queue Full");
    Serial.println("Removing Oldest Vehicle");

    dequeueDOWN();
    }

    downQueue[downRear] = v;

    downRear++;

    if (downRear >= MAX_QUEUE)
      downRear = 0;

    downCount++;

    return;
  }
}


/********************************************************************
                    ETA
********************************************************************/
float calculateETA(float speed)
{
  if (speed <= 0.1)
    return 999;

  return DEMO_DISTANCE / (speed / 3.6);
}

/********************************************************************
                    UPDATE ETA
********************************************************************/
void updateETA()
{
  for(int i=0;i<upCount;i++)
  {
    int idx=(upFront+i)%MAX_QUEUE;

    if(upQueue[idx].active)
    {
      upQueue[idx].eta -= 1.0;

      if(upQueue[idx].eta<0)
        upQueue[idx].eta = 0;
    }
  }

  for(int i=0;i<downCount;i++)
  {
    int idx=(downFront+i)%MAX_QUEUE;

    if(downQueue[idx].active)
    {
      downQueue[idx].eta -= 1.0;

      if(downQueue[idx].eta<0)
        downQueue[idx].eta = 0;
    }
  }
}



/********************************************************************
                    REMOVE EXPIRED
********************************************************************/
void removeExpiredVehicles()
{
  while(upCount>0)
  {
    if(upQueue[upFront].eta<=0)
    {
      Serial.println("UP Vehicle Removed");
      dequeueUP();
      printQueues();
      updatePriority();
      updateDisplays();
    }
    else
      break;
  }

  while(downCount>0)
  {
    if(downQueue[downFront].eta<=0)
    {
      Serial.println("DOWN Vehicle Removed");
      dequeueDOWN();
      printQueues();
      updatePriority();
      updateDisplays();
    }
    else
      break;
  }
}



/********************************************************************
                    SAFE DISPLAY
********************************************************************/
void showSafe()
{
  lcdUp.clear();

  lcdUp.setCursor(0,0);
  lcdUp.print("No Vehicle");

  lcdUp.setCursor(0,1);
  lcdUp.print("SAFE TO GO");



  lcdDown.clear();

  lcdDown.setCursor(0,0);
  lcdDown.print("No Vehicle");

  lcdDown.setCursor(0,1);
  lcdDown.print("SAFE TO GO");
}



/********************************************************************
                    LCD UPDATE
********************************************************************/
void updateDisplays()
{

  displayUpper();

  displayLower();

}







/********************************************************************
                    SCROLL
********************************************************************/
void scrollDisplays()
{

    scrollUp++;

    scrollDown++;

    if(scrollUp>=msgBelow.length())
        scrollUp=0;

    if(scrollDown>=msgAbove.length())
        scrollDown=0;

}

/********************************************************************
                    SEND COMMAND TO NANO
********************************************************************/
void sendNanoCommand(byte cmd)
{
  Wire.beginTransmission(NANO_ADDR);
  Wire.write(cmd);
  Wire.endTransmission();
}



/********************************************************************
                    UPDATE PRIORITY
********************************************************************/
void updatePriority()
{
  // 0 = SAFE
  // 1 = ABOVE VEHICLE ONLY
  // 2 = BELOW VEHICLE ONLY
  // 3 = BOTH (Below Priority)

  if(upCount==0 && downCount==0)
  {
    sendNanoCommand(0);
    Serial.println("STATE : SAFE");
    return;
  }

  if(upCount>0 && downCount==0)
  {
    sendNanoCommand(1);
    Serial.println("STATE : ABOVE ONLY");
    return;
  }

  if(upCount==0 && downCount>0)
  {
    sendNanoCommand(2);
    Serial.println("STATE : BELOW ONLY");
    return;
  }

  sendNanoCommand(3);
  Serial.println("STATE : BOTH SIDES");
}



/********************************************************************
                    DISPLAY UPPER
********************************************************************/
void displayUpper()
{
  lcdUp.clear();

  if(downCount==0)
  {
    lcdUp.setCursor(0,0);
    lcdUp.print("No Vehicle");

    lcdUp.setCursor(0,1);
    lcdUp.print("SAFE TO GO");

    return;
  }

  lcdUp.setCursor(0,0);
  lcdUp.print("Vehicle Below");

  lcdUp.setCursor(0,1);

  lcdUp.print("ETA:");

  if((int)downQueue[downFront].eta < 10)
    lcdUp.print("0");

  lcdUp.print((int)downQueue[downFront].eta);

  lcdUp.print(" ");

  // BOTH SIDES
  if(upCount>0)
      lcdUp.print("WAIT");
  else
      lcdUp.print("GO SLOW");
}



/********************************************************************
                    DISPLAY LOWER
********************************************************************/
void displayLower()
{
  lcdDown.clear();

  if(upCount==0)
  {
    lcdDown.setCursor(0,0);
    lcdDown.print("No Vehicle");

    lcdDown.setCursor(0,1);
    lcdDown.print("SAFE TO GO");

    return;
  }

  lcdDown.setCursor(0,0);
  lcdDown.print("Vehicle Above");

  lcdDown.setCursor(0,1);

  lcdDown.print("ETA:");

  if((int)upQueue[upFront].eta < 10)
    lcdDown.print("0");

  lcdDown.print((int)upQueue[upFront].eta);

  lcdDown.print(" ");

  // BOTH SIDES
  if(downCount>0)
      lcdDown.print("PROCEED");
  else
      lcdDown.print("GO SLOW");
}



/********************************************************************
                    SERIAL DEBUG
********************************************************************/
void printQueues()
{
  Serial.println();
  Serial.println("========== QUEUE ==========");

  Serial.print("UP Queue   : ");
  Serial.println(upCount);

  for(int i=0;i<upCount;i++)
  {
      int idx=(upFront+i)%MAX_QUEUE;

      Serial.print(i+1);
      Serial.print(" -> ");

      Serial.print(upQueue[idx].type);

      Serial.print("  ");

      Serial.print(upQueue[idx].speed);

      Serial.print(" km/h  ETA ");

      Serial.println(upQueue[idx].eta);
  }

  Serial.println();

  Serial.print("DOWN Queue : ");
  Serial.println(downCount);

  for(int i=0;i<downCount;i++)
  {
      int idx=(downFront+i)%MAX_QUEUE;

      Serial.print(i+1);
      Serial.print(" -> ");

      Serial.print(downQueue[idx].type);

      Serial.print("  ");

      Serial.print(downQueue[idx].speed);

      Serial.print(" km/h  ETA ");

      Serial.println(downQueue[idx].eta);
  }

  Serial.println("===========================");
}

void checkCurveSensors()
{
    static bool upPrevState = HIGH;
    static bool downPrevState = HIGH;

    bool upState = digitalRead(UP_PASS_PIN);
    bool downState = digitalRead(DOWN_PASS_PIN);

    if (upState == LOW && upPrevState == HIGH)
    {
        Serial.println("UP Vehicle Passed");

        dequeueUP();
        printQueues();
        updatePriority();
        updateDisplays();
    }
    upPrevState = upState;

    if (downState == LOW && downPrevState == HIGH)
    {
        Serial.println("DOWN Vehicle Passed");

        dequeueDOWN();
        printQueues();
        updatePriority();
        updateDisplays();
    }
    downPrevState = downState;
}

/********************************************************************
                DEQUEUE UP
********************************************************************/
void dequeueUP()
{
  if (upCount == 0)
    return;

  upQueue[upFront].active = false;

  upFront++;

  if (upFront >= MAX_QUEUE)
    upFront = 0;

  upCount--;

  Serial.println("UP Queue Updated");

  Serial.print("Remaining : ");
  Serial.println(upCount);
}



/********************************************************************
                DEQUEUE DOWN
********************************************************************/
void dequeueDOWN()
{
  if (downCount == 0)
    return;

  downQueue[downFront].active = false;

  downFront++;

  if (downFront >= MAX_QUEUE)
    downFront = 0;

  downCount--;

  Serial.println("DOWN Queue Updated");

  Serial.print("Remaining : ");
  Serial.println(downCount);
}