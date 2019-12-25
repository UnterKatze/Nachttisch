/***************************** I N C L U D E S **********************************/
// IR
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// rangesensors
#include "Adafruit_VL53L0X.h"
/***************************** I N C L U D E S **********************************/

/****************************** D E F I N E S ***********************************/
// rangesensors
#define L0X1_ADDRESS 0x31      // i2c address of first rangesensor
#define L0X2_ADDRESS 0x30      // i2c address of second rangesensor
#define SHT_L0X1 D5            // shutdown-pin of first rangesensor
#define SHT_L0X2 D6            // shutdown-pin of second rangesensor
#define RangeSens_StartDist 45 // Threshold value (range in mm) of both sensors

// ledstrips
#define LED_R1 D0
#define LED_G1 D7
#define LED_B1 D8

#define LED_R2 D3
#define LED_G2 D9
#define LED_B2 D10
/****************************** D E F I N E S ***********************************/

/******************************** I N I T S *************************************/
// IR
const uint16_t IR_Pin = D4;
IRrecv irrecv(IR_Pin);
decode_results IR_result;

// rangesensors
Adafruit_VL53L0X rangeSens1 = Adafruit_VL53L0X();
Adafruit_VL53L0X rangeSens2 = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measuredRange1;
VL53L0X_RangingMeasurementData_t measuredRange2;

int actual_mode = 1; // 0 = FADE, 1 = ON, 2 = OFF

// ledstrips
const double max_brightness = 1023; // set max brightness
int brightness_step = 5;            // initial brightness level: 1 = 11%, 2 = 22%, 3 = 33%, 4 = 44%, 5 = 55%, 6 = 66%, 7 = 77%, 8 = 88%, 9 = 100%
int old_brightness_step = brightness_step;

double c_R1 = max_brightness * (brightness_step / 9.0); // set R1 led to 55% (inital)
double c_G1 = max_brightness * (brightness_step / 9.0); // set G1 led to 55% (inital)
double c_B1 = max_brightness * (brightness_step / 9.0); // set B1 led to 55% (inital)

double c_R2 = max_brightness * (brightness_step / 9.0); // set R2 led to 55% (inital)
double c_G2 = max_brightness * (brightness_step / 9.0); // set G2 led to 55% (inital)
double c_B2 = max_brightness * (brightness_step / 9.0); // set B2 led to 55% (inital)

double R_global = 0; // global color values from the IR
double G_global = 0;
double B_global = 0;

double R = 0;
double G = 0;
double B = 0;

int R_int = 0; // only for conversion from double to int
int G_int = 0;
int B_int = 0;
/******************************** I N I T S *************************************/

/***************************** F U N C T I O N S ********************************/
void setSensorIDs()
{
  pinMode(SHT_L0X1, OUTPUT);
  pinMode(SHT_L0X2, OUTPUT);

  digitalWrite(SHT_L0X1, LOW); // reset sensor 1
  digitalWrite(SHT_L0X2, LOW); // reset sensor 2
  delay(10);
  digitalWrite(SHT_L0X1, HIGH); // unreset sensor 1
  digitalWrite(SHT_L0X2, HIGH); // unreset sensor 2
  delay(10);

  digitalWrite(SHT_L0X1, HIGH); // activate sensor 1
  digitalWrite(SHT_L0X2, LOW);  // deactivate sensor 2

  if (!rangeSens1.begin(L0X1_ADDRESS))
  {
    Serial.println(F("Failed to boot first VL53L0X"));
    while (1)
      ;
  }
  delay(10);

  digitalWrite(SHT_L0X2, HIGH); // activate sensor 2
  delay(10);

  if (!rangeSens2.begin(L0X2_ADDRESS))
  {
    Serial.println(F("Failed to boot second VL53L0X"));
    while (1)
      ;
  }
  Serial.println("Sensor IDs set");
}

void initLEDstrips()
{
  pinMode(LED_R1, OUTPUT); // pwm for strip 1 is set to 0
  analogWrite(LED_R1, 0);
  delay(10);
  pinMode(LED_G1, OUTPUT);
  analogWrite(LED_G1, 0);
  delay(10);
  pinMode(LED_B1, OUTPUT);
  analogWrite(LED_B1, 0);
  delay(10);

  pinMode(LED_R2, OUTPUT); // pwm for strip 2 is set to 0
  analogWrite(LED_R2, 0);
  delay(10);
  pinMode(LED_G2, OUTPUT);
  analogWrite(LED_G2, 0);
  delay(10);
  pinMode(LED_B2, OUTPUT);
  analogWrite(LED_B2, 0);

  delay(50);

  Serial.println("Both LED strips initialized");
}

void checkRCVinput()
{
  if (irrecv.decode(&IR_result)) // is there a IR input?
  {
    Serial.println("Button pressed!");

    switch (IR_result.value)
    {
    case 0xF7C837: // mode == FADE ?
      actual_mode = 0;
      break;
    case 0xF7C03F: // mode == ON ?
      actual_mode = 1;
      break;
    case 0xF740BF: // mode == OFF ?
      actual_mode = 2;
      break;
    case 0xF720DF: // Color == 1.1 ?
      R_global = 1023;
      G_global = B_global = 0;
      break;
    case 0xF710EF: // Color == 1.2 ?
      R_global = 822;
      G_global = 409;
      B_global = 0;
      break;
    case 0xF730CF: // Color == 1.3 ?
      R_global = 1023;
      G_global = 662;
      B_global = 0;
      break;
    case 0xF708F7: // Color == 1.4 ?
      R_global = 1023;
      G_global = 863;
      B_global = 0;
      break;
    case 0xF728D7: // Color == 1.5 ?
      R_global = G_global = 1023;
      B_global = 0;
      break;
    case 0xF7A05F: // Color == 2.1 ?
      R_global = B_global = 0;
      G_global = 1023;
      break;
    case 0xF7906F: // Color == 2.2 ?
      R_global = B_global = 0;
      G_global = 822;
      break;
    case 0xF7B04F: // Color == 2.3 ?
      R_global = 257;
      G_global = 899;
      B_global = 834;
      break;
    case 0xF78877: // Color == 2.4 ?
      R_global = 0;
      G_global = 766;
      B_global = 1023;
      break;
    case 0xF7A857: // Color == 2.5 ?
      R_global = 120;
      G_global = 578;
      B_global = 1023;
      break;
    case 0xF7609F: // Color == 3.1 ?
      R_global = G_global = 0;
      B_global = 1023;
      break;
    case 0xF750AF: // Color == 3.2 ?
      R_global = 425;
      G_global = 361;
      B_global = 822;
      break;
    case 0xF7708F: // Color == 3.3 ?
      R_global = 557;
      G_global = 233;
      B_global = 393;
      break;
    case 0xF748B7: // Color == 3.4 ?
      R_global = 1023;
      G_global = 421;
      B_global = 722;
      break;
    case 0xF76897: // Color == 3.5 ?
      R_global = 1023;
      G_global = 80;
      B_global = 590;
      break;
    case 0xF7E01F: // Color == White ?
      R_global = G_global = B_global = 1023;
      break;
    default:
      break;
    }

    if (IR_result.value == 0xF700FF) // Brightness == Up ?
    {
      switch (brightness_step)
      {
      case 1:
        brightness_step = 2;
        break;
      case 2:
        brightness_step = 3;
        break;
      case 3:
        brightness_step = 4;
        break;
      case 4:
        brightness_step = 5;
        break;
      case 5:
        brightness_step = 6;
        break;
      case 6:
        brightness_step = 7;
        break;
      case 7:
        brightness_step = 8;
        break;
      case 8:
        brightness_step = 9;
        break;
      default:
        break;
      }
    }
    else
    {
      if (IR_result.value == 0xF7807F) // Brightness == Down ?
      {
        switch (brightness_step)
        {
        case 2:
          brightness_step = 1;
          break;
        case 3:
          brightness_step = 2;
          break;
        case 4:
          brightness_step = 3;
          break;
        case 5:
          brightness_step = 4;
          break;
        case 6:
          brightness_step = 5;
          break;
        case 7:
          brightness_step = 6;
          break;
        case 8:
          brightness_step = 7;
          break;
        case 9:
          brightness_step = 8;
          break;
        default:
          break;
        }
      }
    }

    R = R_global * (brightness_step / 9.0);
    G = G_global * (brightness_step / 9.0);
    B = B_global * (brightness_step / 9.0);

    serialPrintUint64(IR_result.value, HEX);
    irrecv.resume(); // Receive the next value
  }
}

void checkDrawerRanges()
{
  rangeSens1.rangingTest(&measuredRange1, false);
  rangeSens2.rangingTest(&measuredRange2, false);

  if (measuredRange1.RangeStatus != 4) // if not out of range
  {
    Serial.println("Sensor 1:\t");
    Serial.print(measuredRange1.RangeMilliMeter);
    Serial.println("");
  }
  else
  {
    measuredRange1.RangeMilliMeter = 0;
    Serial.println("Sensor 1 Out of range");
  }
  if (measuredRange2.RangeStatus != 4) // if not out of range
  {
    Serial.println("Sensor 2:\t");
    Serial.print(measuredRange2.RangeMilliMeter);
    Serial.println("");
  }
  else
  {
    measuredRange2.RangeMilliMeter = 0;
    Serial.println("Sensor 2 Out of range");
  }
}

void calculateBrightness()
{
  if (actual_mode == 1) // if Mode == ON
  {
    c_R1 = c_R2 = R;
    c_G1 = c_G2 = G;
    c_B1 = c_B2 = B;
  }

  if (actual_mode == 2) // if Mode == OFF
  {
    c_R1 = c_R2 = c_G1 = c_G2 = c_B1 = c_B2 = 0;
    Serial.println("Mode = OFF");
  }

  if (actual_mode == 0)
  {
    if (measuredRange2.RangeStatus != 4) // if drawer 1 is not out of range
    {
      c_R1 = R * ((measuredRange2.RangeMilliMeter - 45.0) / 260.0);
      c_G1 = G * ((measuredRange2.RangeMilliMeter - 45.0) / 260.0);
      c_B1 = B * ((measuredRange2.RangeMilliMeter - 45.0) / 260.0);

      if (c_R1 > 1023)
      {
        c_R1 = 1023;
      }
      if (c_R1 < 0)
      {
        c_R1 = 0;
      }

      if (c_G1 > 1023)
      {
        c_G1 = 1023;
      }
      if (c_G1 < 0)
      {
        c_G1 = 0;
      }

      if (c_B1 > 1023)
      {
        c_B1 = 1023;
      }
      if (c_B1 < 0)
      {
        c_B1 = 0;
      }
    }
    else
    {
      c_R1 = c_G1 = c_B1 = 0;
    }

    if (measuredRange1.RangeStatus != 4) // if drawer 2 is not out of range
    {
      c_R2 = R * ((measuredRange1.RangeMilliMeter - 45.0) / 260.0);
      c_G2 = G * ((measuredRange1.RangeMilliMeter - 45.0) / 260.0);
      c_B2 = B * ((measuredRange1.RangeMilliMeter - 45.0) / 260.0);

      if (c_R2 > 1023)
      {
        c_R2 = 1023;
      }
      if (c_R2 < 0)
      {
        c_R2 = 0;
      }

      if (c_G2 > 1023)
      {
        c_G2 = 1023;
      }
      if (c_G2 < 0)
      {
        c_G2 = 0;
      }

      if (c_B2 > 1023)
      {
        c_B2 = 1023;
      }
      if (c_B2 < 0)
      {
        c_B2 = 0;
      }
    }
    else
    {
      c_R2 = c_G2 = c_B2 = 0;
    }
  }

  Serial.println("R = ");
  Serial.println((int)R);
  Serial.println("G = ");
  Serial.println((int)G);
  Serial.println("B = ");
  Serial.println((int)B);
  Serial.println();
}

void setLEDstrip()
{
  R_int = c_R1;
  G_int = c_G1;
  B_int = c_B1;

  analogWrite(LED_R1, R_int);
  analogWrite(LED_G1, G_int);
  analogWrite(LED_B1, B_int);

  R_int = c_R2;
  G_int = c_G2;
  B_int = c_B2;

  analogWrite(LED_R2, R_int);
  analogWrite(LED_G2, G_int);
  analogWrite(LED_B2, B_int);
}
/***************************** F U N C T I O N S ********************************/

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("\nSerial initialized");
  delay(1000);

  irrecv.enableIRIn(); // Start the IR
  Serial.println("IR initialized");
  delay(1000);

  setSensorIDs(); // set IDs for both sensors
  delay(1000);

  initLEDstrips(); // init both led strips
  delay(1000);
}

void loop()
{
  checkRCVinput(); // check mode, color, brightness
  //delay(1000);

  checkDrawerRanges(); // check ranges & choose drawer
  //delay(1000);

  calculateBrightness(); // calculate relative brightness
  //delay(1000);

  setLEDstrip(); // set the strip

  delay(5); // ~ 1/delay Hz refresh rate
}
