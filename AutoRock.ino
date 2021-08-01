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

#define   INIT_STATE 0
#define   IDLE_STATE 1
#define JOG_UP_STATE 2
#define JOG_DN_STATE 3
#define   HOME_STATE 4
#define  SETUP_STATE 5
#define  START_STATE 6
#define    RUN_STATE 7
#define   HOLD_STATE 8
#define RETURN_STATE 9
#define   STOP_STATE 10
#define  ERROR_STATE 11

#define EEREAD(struc) eeprom_read_block((void*)&struc, (void*)0, sizeof(struc))
#define EEWRITE(struc) eeprom_write_block((const void*)&struc, (void*)0, sizeof(struc))

/* Variable Declaration */
int state = INIT_STATE;
int nextState = NULL;

int cycleCount = 0;
#define HOLD_TIMEOUT 30

#define ENC_DIS 6000
volatile long encPos = 0;
long encDis = 0;
long encMax = 50000; 

#include <avr/eeprom.h>
struct DrillConfig {
  uint32_t dSpeed = 300;
  uint32_t jSpeed = 300;
  long uses = 0;
} settings;

void moveDrill(bool, uint32_t);
void toggleWater(bool);
void toggleDrill(bool);

#include "Nextion.h"
#define nexClear() \
  Serial2.write(0xff); \
  Serial2.write(0xff); \
  Serial2.write(0xff);

// Main Screen
NexPage p0 = NexPage(0, 0, "init");
NexPage p1 = NexPage(1, 0, "main");

NexSlider  p1_h0 = NexSlider(1, 1, "h0");
NexSlider  p1_h1 = NexSlider(1, 2, "h1");
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

void p1_m0PushCallback(void *ptr) {
  state = STOP_STATE;
}

void p1_m1PushCallback(void *ptr) {
  state = SETUP_STATE;
}

void p1_m2PushCallback(void *ptr) {
  state = ERROR_STATE;
}

void p1_m3PushCallback(void *ptr) {
  if(state ==IDLE_STATE) {
    toggleWater(!digitalRead(WATER_RELAY_PIN));
  }
}

void p1_m4PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    toggleDrill(!digitalRead(DRILL_RELAY_PIN));
  }
}

void p1_m5PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    state = JOG_UP_STATE;
  }
}

void p1_m5PopCallback(void *ptr) {
  if(state == JOG_UP_STATE) {
    state = IDLE_STATE;
  }
}

void p1_m6PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    state = JOG_DN_STATE;  
  }
}

void p1_m6PopCallback(void *ptr) {
  if(state == JOG_DN_STATE) {
    state = IDLE_STATE;
  }
}

void setup() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(PUL_PIN, OUTPUT);

  pinMode(ESTOP_PIN, INPUT_PULLUP);
  
  pinMode(WATER_RELAY_PIN, OUTPUT);
  pinMode(DRILL_RELAY_PIN, OUTPUT);
  toggleDrill(OFF);
  toggleWater(OFF);

  pinMode(UPPER_LIMIT_PIN, INPUT);
  pinMode(LOWER_LIMIT_PIN, INPUT);
  pinMode(PLATE_LIMIT_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), isrA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), isrB, CHANGE);

  EEREAD(settings);

  Serial.begin(9600);

  Serial2.begin(9600);
  nexClear();


  
  p1_h0.attachPop(p1_h0PopCallback);
  p1_h1.attachPop(p1_h1PopCallback);
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

#define readA digitalRead(ENC_A_PIN)
#define readB digitalRead(ENC_B_PIN)
void isrA() { encPos += (readA != readB) ? 1 : -1; }
void isrB() { encPos += (readA == readB) ? 1 : -1; }

void moveDrill(bool dir, uint32_t spd) {
  digitalWrite(DIR_PIN, dir);

  digitalWrite(PUL_PIN, HIGH);
  delayMicroseconds(spd);

  digitalWrite(PUL_PIN, LOW);   
  delayMicroseconds(spd); 
}

void toggleDrill(bool toggle) {
  digitalWrite(DRILL_RELAY_PIN, toggle); 
}

void toggleWater(bool toggle) {
  digitalWrite(WATER_RELAY_PIN, toggle); 
}

void stateMachine() {
  switch(state) {
    case INIT_STATE:   
      state = IDLE_STATE;
      
      delay(2000);

      p1.show();
      p1_h0.setValue(settings.jSpeed);
      p1_h1.setValue(settings.dSpeed);
    break;
      
    case IDLE_STATE:               
    break;

    case JOG_UP_STATE:
      if(digitalRead(UPPER_LIMIT_PIN)) {
        moveDrill(UP, settings.jSpeed);
      }
    break;

    case JOG_DN_STATE:
      if(digitalRead(PLATE_LIMIT_PIN) && digitalRead(LOWER_LIMIT_PIN)) {
        moveDrill(DOWN, settings.jSpeed);
      }
    break;
    
    case HOME_STATE:
      if(!digitalRead(UPPER_LIMIT_PIN)) {
        encPos = 0;
        state = nextState;
      } else {
        moveDrill(UP, settings.dSpeed);
      }
    break;
      
    case SETUP_STATE:
      toggleDrill(OFF);
      toggleWater(OFF);
			nextState = START_STATE;
      state = HOME_STATE;
    break;
      
    case START_STATE: 
      if(!digitalRead(LOWER_LIMIT_PIN) || (encPos >= encMax)) {
				state = STOP_STATE;
				break;
			}

      if(!digitalRead(PLATE_LIMIT_PIN)) {
        encDis = encPos - ENC_DIS;
        Serial.println(encDis);
        state = RETURN_STATE;
      } else {
        moveDrill(DOWN, settings.dSpeed);
      }
    break;
      
    case RUN_STATE:
      if(!digitalRead(LOWER_LIMIT_PIN) || (encPos >= encMax)) {
        toggleDrill(OFF);
        toggleWater(OFF);

        settings.uses += 1;
        EEWRITE(settings);
        
        nextState = IDLE_STATE;
        state = HOME_STATE;
		    break;
			}

			toggleDrill(ON);
      toggleWater(ON);

			if(!digitalRead(PLATE_LIMIT_PIN)) {
        delay(10);
				state = HOLD_STATE;
			} else {
				moveDrill(DOWN, settings.dSpeed);
			}
    break;

		case HOLD_STATE:
			if(cycleCount++ < HOLD_TIMEOUT && !digitalRead(PLATE_LIMIT_PIN)) {
				break;
			} else {
        cycleCount = 0;
        state = RETURN_STATE;
			}
	  break;

		case RETURN_STATE:
      if(!digitalRead(UPPER_LIMIT_PIN)) {
        state = STOP_STATE;
				break;
      }

      if(encPos <= encDis) {
        state = RUN_STATE;
      } else {
				moveDrill(UP, settings.dSpeed);
			}
    break;
      
    case STOP_STATE:
      toggleDrill(OFF);
      toggleWater(OFF);
      
      state = IDLE_STATE;
    break;
      
    case ERROR_STATE:
      toggleDrill(OFF);
      toggleWater(OFF);
    break;
  }
}

void loop() {
  if(digitalRead(ESTOP_PIN)) {
    state = ERROR_STATE;
  } else if(state == ERROR_STATE) {
    state = IDLE_STATE;
  }

  stateMachine();
  
  nexLoop(nex_listen_list);
}
