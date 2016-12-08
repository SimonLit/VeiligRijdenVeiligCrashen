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

// ================================================================
// ===           GENERAL SERIAL COMMUNICATION PROTOCOL          ===
// ================================================================
#define START_CHARACTER '#'
#define END_CHARACTER '@'
#define VALUE_CHARACTER ':'
#define MULTI_VALUE_SEPARATOR ','
// ================================================================
// ===            SERIAL COMMUNICATION RP6 PROTOCOL             ===
// ================================================================
#define CONNECT_TO_DEVICE "CONNECT:"
#define RP6_STARTED_PROGRAM "START_RP6"
#define RP6_STOPPED_PROGRAM "STOP_RP6"
#define SPEED_PROTOCOL_SEND_RECEIVE "SPEED:"
#define SIDE_HIT_PROTOCOL_SEND_RECEIVE "SIDE_HIT:"
#define CONTROLLER_DISCONNECTED "NO_CONTROLLER"
#define IMPACT_PROTOCOL_SEND_RECEIVE "IMPACT:"
#define DIST_DRIVEN_PROTOCOL_SEND_RECEIVE "DIST_DRIVEN:"
#define ORIENTATION_PROTOCOL_SEND "ORIENTATION_YPR:"
#define ORIENTATION_PROTOCOL_RECEIVE "ORIENTATION"
// ================================================================
// ===         SERIAL COMMUNICATION CONTROLLER PROTOCOL         ===
// ================================================================
#define CONTROLLER_SPEED_PROTOCOL_SEND "SPEED:"
#define CONTROLLER_STEER_PROTOCOL_SEND "X:"
//=================================================================
// ===         WIFI COMMUNICATION CONTROLLER PROTOCOL           ===
// ================================================================
#define CONTROLLER_VALUE_PROTOCOL_RECEIVE "ControllerValues:"
#define CONTROLLER_VALUE_PROTOCOL_REQUEST_SEND "GetControllerValues"
//=================================================================
// ===        WIFI COMMUNICATION BOARDCOMPUTER PROTOCOL         ===
// ================================================================
#define BOARDCOMPUTER_CONNECT_RESPONSE ""
//=================================================================

#define GENERAL_ACK "ACK"
#define GENERAL_NACK "NACK"
#define RP6_NAME "RP6"
#define WEMOS_NAME "WEMOS"
#define WEMOS_NUMBER 1
//=================================================================
String protocolToSendArray[5]; // 0 = speed; 1 = sideHit; 2 = impact; 3 = distDriven; 4 = orientation;

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
const char* ssid = "Project";
const char* password = "123456780";

String tempMessage = "";
String speedProtocol = "";
String steerProtocol = "";

bool protocolEndCharReceived = false;

long currentMillis = 0;
long lastControllerReceiveTimer = 0;
int maxControllerTimeoutTimer =  120;
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
  while (!MPUIsStable)
  {
    if (getMPUIsStabilized())
    {
      Serial.println("MPU is stable");
      MPUIsStable = true;
      resetYPRValues();
    }
    else return;
  }

  /*
     Update the values in the currentYPR array to the latest YPR values from the MPU.
     The YPR values are then corrected with the offset values.
  */
  if ((millis() -  lastYPRUpdate) > updateTime)
  {
    updateCurrentOrientation(currentYPR);
    lastYPRUpdate = millis();
  }

  // Check if a Serial message is received that end with '@'.
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
     Receive the controller values and send these to the RP6.
  */
  if ((millis() - currentMillis) > requestInterval)
  {
    getControllerValues(&speedProtocol, &steerProtocol);
    currentMillis = millis();
  }

  /*
     When the no controller messages are received for maxControllerTimeoutTimer amount of time
     send a messsage to the RP6 that sais that the RP6 must stop driving.
  */
  if ((millis() - lastControllerReceiveTimer) > maxControllerTimeoutTimer)
  {
    String noControllerMessage = START_CHARACTER + CONTROLLER_DISCONNECTED + END_CHARACTER;
    Serial.println(noControllerMessage);
  }

  /*
     When a '@' is received from the Serial line a potentional protocol message is received.
  */
  if (receivedEndOfSerialString)
  {
    if (checkForValidRP6Message(stringFromSerial) == 1)
    {
      if (stringFromSerial == RP6_STARTED_PROGRAM || stringFromSerial == RP6_STOPPED_PROGRAM)
      {
        sendRP6StatusToBoardcomputer(stringFromSerial);
      }

      
      //  When ORIENTATION is received it means all the crash data is received
      //  from the RP6 and can be send to the boardcomputer.
      if (stringFromSerial == "ORIENTATION")
      {
        sendCrashData(protocolToSendArray, 5);
      }
    }

    // When the received message isn't in the protocol from RP6 to Wemos a NACK is send to the RP6.
    if (checkForValidRP6Message(stringFromSerial) == 0)
    {
      String nackMessage = START_CHARACTER + GENERAL_NACK + END_CHARACTER;
      Serial.println(nackMessage);
    }

    // If the received message was NACK, send the speed speed values again.
    else if (checkForValidRP6Message(stringFromSerial) == -1)
    {
      Serial.println(speedProtocol);
      Serial.println(steerProtocol);
    }

    // This method checks if the received message belongs to one of the recieved crach data messages
    // and puts the message with their values in de crash data arrray.
    formatMessageToProtocol(stringFromSerial, protocolToSendArray);

    // Set the boolean to false to ensure these actions only happen again when a potantional valid
    // serial message is received.
    protocolEndCharReceived = false;

    stringFromSerial = "";
  }
}
