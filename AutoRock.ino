/* Pin Definition */
#define ENC_A_PIN 2
#define ENC_B_PIN 3

#define PUL_PIN 8
#define DIR_PIN 7

#define ESTOP_PIN 13

#define WATER_RELAY_PIN 26
#define DRILL_RELAY_PIN 27
//Unused Relay_Pin 28
//Unused Relay_Pin 29

#define UPPER_LIMIT_PIN 30
#define LOWER_LIMIT_PIN 31
#define PLATE_LIMIT_PIN 32

/* Pre-Processor Definition */
#define   UP false
#define DOWN true

#define  ON false
#define OFF true

enum States 
{
  INIT_STATE,
  IDLE_STATE,
  JOG_UP_STATE,
  JOG_DN_STATE,
  HOME_STATE,
  SETUP_STATE,
  START_STATE,
  RUN_STATE,
  HOLD_STATE,
  RETURN_STATE,
  STOP_STATE,
  ERROR_STATE
};

/* Variable Declaration */
States currState = INIT_STATE;
States nextState = INIT_STATE;

#define ENC_DIS 6000
volatile long encPos = 0;
long encDis = 0;
long encMax = 50000; 

int currDwell = 0;

#define CYCLE_COUNT 5
int currCycle = 0;

/* EEPROM Definition */
#include <avr/eeprom.h>
#define EEREAD(struc) eeprom_read_block((void*)&struc, (void*)0, sizeof(struc))
#define EEWRITE(struc) eeprom_write_block((const void*)&struc, (void*)0, sizeof(struc))

struct DrillConfig {
  uint32_t dSpeed = 300;
  uint32_t jSpeed = 300;
  uint32_t dwellTime = 100;
} settings;

void move_drill(bool, uint32_t);
void toggle_water(bool);
void toggle_drill(bool);

#include "Nextion.h"

#define nexClear() \
  Serial2.write(0xff); \
  Serial2.write(0xff); \
  Serial2.write(0xff);

NexPage p0 = NexPage(0, 0, "init");
NexPage p1 = NexPage(1, 0, "main");

NexSlider  p1_h0 = NexSlider(1, 1, "h0");
NexSlider  p1_h1 = NexSlider(1, 2, "h1");
NexSlider  p1_h2 = NexSlider(1, 10, "h2");
NexHotspot p1_m0 = NexHotspot(1, 3, "m0");
NexHotspot p1_m1 = NexHotspot(1, 4, "m1");
NexHotspot p1_m2 = NexHotspot(1, 5, "m2");
NexHotspot p1_m3 = NexHotspot(1, 6, "m3");
NexHotspot p1_m4 = NexHotspot(1, 7, "m4");
NexHotspot p1_m5 = NexHotspot(1, 8, "m5");
NexHotspot p1_m6 = NexHotspot(1, 9, "m6");

NexTouch *nex_listen_list[] = {
  &p1_h0,
  &p1_h1,
  &p1_m0,
  &p1_m1,
  &p1_m2,
  &p1_m3,
  &p1_m4,
  &p1_m5,
  &p1_m6,
  NULL
};

void p1_h0PopCallback(void *ptr) {
  uint32_t nSpeed;
  p1_h0.getValue(&nSpeed);
  settings.jSpeed = nSpeed;
  EEWRITE(settings);
}

void p1_h1PopCallback(void *ptr) {
  uint32_t nSpeed;
  p1_h1.getValue(&nSpeed);
  settings.dSpeed = nSpeed;
  EEWRITE(settings);
}

void p1_h2PopCallback(void *ptr) {
  uint32_t dTime;
  p1_h1.getValue(&dTime);
  settings.dSpeed = dTime;
  EEWRITE(settings);
}

void p1_m0PushCallback(void *ptr) { currState = STOP_STATE; }
void p1_m1PushCallback(void *ptr) { currState = SETUP_STATE; }
void p1_m2PushCallback(void *ptr) { currState = ERROR_STATE; }
void p1_m3PushCallback(void *ptr) { if(currState ==IDLE_STATE) toggleWater(!digitalRead(WATER_RELAY_PIN)); }
void p1_m4PushCallback(void *ptr) { if(currState == IDLE_STATE) toggleDrill(!digitalRead(DRILL_RELAY_PIN)); }
void p1_m5PushCallback(void *ptr) { if(currState == IDLE_STATE) currState = JOG_UP_STATE; }
void p1_m5PopCallback(void *ptr) { if(currState == JOG_UP_STATE) currState = IDLE_STATE; }
void p1_m6PushCallback(void *ptr) { if(currState == IDLE_STATE) currState = JOG_DN_STATE; }
void p1_m6PopCallback(void *ptr) { if(currState == JOG_DN_STATE) currState = IDLE_STATE; }

void screen_init() {
  nexClear();

  p1_h0.attachPop(p1_h0PopCallback);
  p1_h1.attachPop(p1_h1PopCallback);
  p1_h2.attachPop(p1_h2PopCallback);
  p1_m0.attachPush(p1_m0PushCallback);
  p1_m1.attachPush(p1_m1PushCallback);
  p1_m2.attachPush(p1_m2PushCallback);
  p1_m3.attachPush(p1_m3PushCallback);
  p1_m4.attachPush(p1_m4PushCallback);
  p1_m5.attachPush(p1_m5PushCallback);
  p1_m5.attachPop(p1_m5PopCallback);
  p1_m6.attachPush(p1_m6PushCallback);
  p1_m6.attachPop(p1_m6PopCallback); 
}

void setup() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(PUL_PIN, OUTPUT);

  pinMode(ESTOP_PIN, INPUT_PULLUP);
  
  pinMode(WATER_RELAY_PIN, OUTPUT);
  pinMode(DRILL_RELAY_PIN, OUTPUT);
  toggle_drill(OFF);
  toggle_water(OFF);

  pinMode(UPPER_LIMIT_PIN, INPUT);
  pinMode(LOWER_LIMIT_PIN, INPUT);
  pinMode(PLATE_LIMIT_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), isrA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), isrB, CHANGE);

  EEREAD(settings);

  Serial.begin(9600);

  Serial2.begin(9600);
  screen_init();
}

#define readA digitalRead(ENC_A_PIN)
#define readB digitalRead(ENC_B_PIN)
void isrA() { encPos += (readA != readB) ? 1 : -1; }
void isrB() { encPos += (readA == readB) ? 1 : -1; }

void move_drill(bool dir, uint32_t spd) {
  digitalWrite(DIR_PIN, dir);

  digitalWrite(PUL_PIN, HIGH);
  delayMicroseconds(spd);

  digitalWrite(PUL_PIN, LOW);   
  delayMicroseconds(spd); 
}

#define getLow digitalRead(LOWER_LIMIT_PIN);
#define getUpp digitalRead(UPPER_LIMIT_PIN);
#define getPlate digitalRead(PLATE_LIMIT_PIN);
void toggle_drill(bool toggle) { digitalWrite(DRILL_RELAY_PIN, toggle); }
void toggle_water(bool toggle) { digitalWrite(WATER_RELAY_PIN, toggle); }

void loop() {
  if(digitalRead(ESTOP_PIN)) {
    currState = ERROR_STATE;
  } else if(currState == ERROR_STATE) {
    currState = IDLE_STATE;
  }

  switch(currState)
    case INIT_STATE:
      currState = IDLE_STATE;
      delay(2000);

      p1.show();
      p1_h0.setValue(settings.jSpeed);
      p1_h1.setValue(settings.dSpeed);
      break;
  
    case IDLE_STATE:
      break;
    
    case JOG_UP_STATE:
      if(getUpp) move_drill(UP, settings.jSpeed);
      break;

    case JOG_DN_STATE:
      if(getLow) move_drill(DOWN, settings.jSpeed);
      break;

    case HOME_STATE:
      if(!getUpp) {
        encPos = 0;
        currState = nextState;
      } else {
        move_drill(UP, settings.dSpeed);
      }
      break;

    case SETUP_STATE:
      toggle_drill(OFF);
      toggle_water(OFF);
      nextState = START_STATE;
      currState = HOME_STATE;
      break;

    case START_STATE:
      if(!getLow || (encPos >= encMax)) {
        currState = STOP_STATE;
        break;
      }

      if(!getPlate) {
        encDis = encPos - ENC_DIS;
        currState = RETURN_STATE;
      } else {
        move_drill(DOWN, settings.dSpeed);
      }
      break;

    case RUN_STATE:
      if(!getLow || (encPos >= encMax)) {
        toggle_drill(OFF);
        toggle_water(OFF);
        
        nextState = IDLE_STATE;
        currState = HOME_STATE;
        break;
      }

      toggle_drill(ON);
      toggle_water(ON);

      if(!getPlate) {
        move_drill(DOWN, settings.dSpeed);
        delay(100);
        currState = HOLD_STATE;
      } else {
        move_drill(DOWN, settings.dSpeed);
      }

      break;

    case HOLD_STATE:
      if(currDwell++ < settings.dwellTime && !getPlate) {
        delay(100);
        break;
      }
    
      currDwell = 0;
      currState = currCycle++ < CYCLE_COUNT ? RUN_STATE : RETURN_STATE;
      break;

    case RETURN_STATE:
      if(!getUpp) {
        currState = STOP_STATE;
        break;
      }

      if(encPos <= encDis) {
        currState = RUN_STATE;
      } else {
        move_drill(UP, settings.dSpeed);
      }
      break;

    case STOP_STATE:
      currState = IDLE_STATE;
      toggle_drill(OFF);
      toggle_water(OFF);
      break;
    
    case ERROR_STATE:
      toggle_drill(OFF);
      toggle_water(OFF);
      break;

    case default:
      currState = IDLE_STATE;
  
  nexLoop(nex_listen_list);
}