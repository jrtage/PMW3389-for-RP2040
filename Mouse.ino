/* 

# PIN CONNECTION
  * MI = MISO
  * MO = MOSI
  * SS = Slave Select / Chip Select
  * SC = SPI Clock
  * MT = Motion (active low interrupt line)
  * RS = Reset
  * GD = Ground
  * VI = Voltage in up to +5.5V 

Module   Arduino
  RS --- (NONE)
  GD --- GND
  MT --- (NONE)
  SS --- Pin_10   (use this pin to initialize a PMW3389 instance)
  SC --- SCK 
  MO --- MOSI
  MI --- MISO
  VI --- 5V

# PMW3389_DATA struct format and description
  - PMW3389_DATA.isMotion      : bool, True if a motion is detected. 
  - PMW3389_DATA.isOnSurface   : bool, True when a chip is on a surface 
  - PMW3389_DATA.dx, data.dy   : integer, displacement on x/y directions.
  - PMW3389_DATA.SQUAL         : byte, Surface Quality register, max 0x80
                               * Number of features on the surface = SQUAL * 8
  - PMW3389_DATA.rawDataSum    : byte, It reports the upper byte of an 18‐bit counter 
                               which sums all 1296 raw data in the current frame;
                               * Avg value = Raw_Data_Sum * 1024 / 1296
  - PMW3389_DATA.maxRawData    : byte, Max/Min raw data value in current frame, max=127
    PMW3389_DATA.minRawData
  - PMW3389_DATA.shutter       : unsigned int, shutter is adjusted to keep the average
                               raw data values within normal operating ranges.
 */
#include "PMW3389.h"
#include <Mouse.h>

#define SS  5   // Slave Select pin. Connect this to SS on the module.
#define led 8   // LED Indicator to tell me its on
#define NUMBTN 2        // number of buttons attached
#define BTN1 15          // left button pin
#define BTN2 14          // right button pin
#define DEBOUNCE  1    // debounce itme in ms. Minimun time required for a button to be stabilized.

int btn_pins[NUMBTN] = { BTN1, BTN2 };
char btn_keys[NUMBTN] = { MOUSE_LEFT, MOUSE_RIGHT };

PMW3389 sensor;

// button pins & debounce buffers
bool btn_state[NUMBTN] = { false, false };
uint8_t btn_buffers[NUMBTN] = {0xFF, 0xFF};

unsigned long lastButtonCheck = 0;

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);

  Serial.begin(115200);  
  while(!Serial);
  
  if(sensor.begin(SS))  // 10 is the pin connected to SS of the module.
    Serial.println("Sensor initialization successed");
  else
    Serial.println("Sensor initialization failed");
  Mouse.begin();
  buttons_init();
}

void loop() {
  check_buttons_state();

  PMW3389_DATA data = sensor.readBurst();
  
  if(data.isOnSurface && data.isMotion)
  {
    int mdx = constrain(data.dx, -255, 255);
    int mdy = constrain(data.dy, -255, 255);
    Serial.print(data.dx);
    Serial.print("\t");
    Serial.print(data.dy);
    Serial.print("\t");
    Serial.print(mdx);
    Serial.print("\t");
    Serial.print(mdy);
    Serial.println();

    //Mouse.move(mdx, mdy, 0);
  }
  
}


void buttons_init()
{
  for(int i=0;i < NUMBTN; i++)
  {
    pinMode(btn_pins[i], INPUT_PULLUP);
  }
}

void check_buttons_state() 
{
  unsigned long elapsed = micros() - lastButtonCheck;
  
  // Update at a period of 1/8 of the DEBOUNCE time
  if(elapsed < (DEBOUNCE * 1000UL / 8))
    return;
  
  lastButtonCheck = micros();
    
  // Fast Debounce (works with 0 latency most of the time)
  for(int i=0;i < NUMBTN ; i++)
  {
    int state = digitalRead(btn_pins[i]);
    btn_buffers[i] = btn_buffers[i] << 1 | state; 

    if(!btn_state[i] && btn_buffers[i] == 0xFE)  // button pressed for the first time
    {
      Mouse.press(btn_keys[i]);
      btn_state[i] = true;
    }
    else if( (btn_state[i] && btn_buffers[i] == 0x01) // button released after stabilized press
            // force release when consequent off state (for the DEBOUNCE time) is detected 
            || (btn_state[i] && btn_buffers[i] == 0xFF) ) 
    {
      Mouse.release(btn_keys[i]);
      btn_state[i] = false;
    }
  }
}
