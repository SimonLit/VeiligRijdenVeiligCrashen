// ================================================================
// ===                        INCLUDES                          ===
// ================================================================
#include <ESP8266WiFi.h>
#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"
//#include "MPU6050.h" // not necessary if using MotionApps include file

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

String stringFromSerial = "";
String speedProtocol = "";
String steerProtocol = "";

// ================================================================
// ===                   MPU VARIABLE SETUP                     ===
// ================================================================

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high


#define INTERRUPT_PIN D2  // use pin 2 on Arduino Uno & most boards
#define LED_PIN LED_BUILTIN // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

bool MPUIsStable = false;

int currentYPR[3] = {0, 0, 0};
long lastYPRUpdate = 0;
long updateTime = 3000;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady()
{
  mpuInterrupt = true;
}

// ================================================================
// ===                      WIFI VARIABLES                      ===
// ================================================================
//const char* ssid = "HotSpotBoardComputer";
//const char* password = "1234567890";

const char* ssid = "Project";
const char* password = "123456780";

//const char* ssid = "eversveraa";
//const char* password = "qwerty69";

String tempMessage = "";

String protocolToSendArray[5]; // 0 = speed; 1 = sideHit; 2 = impact; 3 = distDriven; 4 = orientation;

bool protocolEndCharReceived = false;

long currentMillis = 0;
int requestInterval = 100;

// ================================================================
// ===                      MAIN SETUP                          ===
// ================================================================
void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  //Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  stwire::setup(400, true);
#endif

  // initialize serial communication
  // (115200 chosen because it is required for Teapot Demo output, but it's
  // really up to you depending on your project)
  Serial.begin(38400);

  // =========================SETUP FOR MPU=========================

  // NOTE: 8MHz or slower host processors, like the Teensy @ 3.3v or Ardunio
  // Pro Mini running at 3.3v, cannot handle this baud rate reliably due to
  // the baud timing being too misaligned with processor ticks. You must use
  // 38400 or slower in these cases, or use some kind of external separate
  // crystal solution for the UART timer.

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  // verify connection
  Serial.println(mpu.getDeviceID(), HEX);
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU9250 connection successful") : F("MPU9250 connection failed"));

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0)
  {
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else
  {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  // configure LED for output
  pinMode(LED_PIN, OUTPUT);


  // ========================SETUP FOR ESP========================
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // If the router isn't available for use comment this while loop.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{
  /*
     Make Sure the DMP values are stable.
  */
  /*while (!MPUIsStable)
    {
    if (getMPUIsStabilized())
    {
      Serial.println("MPU is stable");
      MPUIsStable = true;
      resetYPRValues();
    }
    else return;
    }*/

  if ((millis() -  lastYPRUpdate) > updateTime)
  {
    updateCurrentOrientation(currentYPR);
    lastYPRUpdate = millis();
  }

  bool receivedEndOfSerialString = getIncommingString(&stringFromSerial);

  /*
     Reset the orentation values close to 0.
  */
  if (stringFromSerial == "RESET")
  {
    Serial.println(stringFromSerial);
    stringFromSerial = "";
    resetYPRValues();
  }

  /*
     Receive the controller values.
  */
  if ((millis() - currentMillis) > requestInterval)
  {
    //getControllerValues(&speedProtocol, &steerProtocol);
    Serial.println(speedProtocol);
    Serial.println(steerProtocol);

    currentMillis = millis();
  }

  if (receivedEndOfSerialString)
  {
    if (checkForValidMessage(stringFromSerial))
    {
      //Serial.println("NACK");
    }
    formatMessageToProtocol(stringFromSerial, protocolToSendArray);
    /*if (stringFromSerial == "NACK")
      {
      Serial.println(speedProtocol);
      Serial.println(steerProtocol);
      }*/
    protocolEndCharReceived = false;
  }

  /*
      When ORIENTATION is received it means all the crash data is received
      from the RP6 and can be send to the boardcomputer.
  */
  if (stringFromSerial == "ORIENTATION")
  {
    // SEND CHRASH DATA TO BOARDCOMPUTER.
    Serial.println("fdsa");
    sendCrashData(protocolToSendArray, 5);
    stringFromSerial = "";
  }
}
