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
int state = IDLE_STATE;
int nextState = NULL;

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
NexSlider  main_h0 = NexSlider(1, 1, "h0");
NexSlider  main_h1 = NexSlider(1, 2, "h1");
NexHotspot main_m0 = NexHotspot(0, 3, "m0");
NexHotspot main_m1 = NexHotspot(0, 4, "m1");
NexHotspot main_m2 = NexHotspot(0, 5, "m2");
NexHotspot main_m3 = NexHotspot(0, 6, "m3");
NexHotspot main_m4 = NexHotspot(0, 7, "m4");
NexHotspot main_m5 = NexHotspot(0, 8, "m5");
NexHotspot main_m6 = NexHotspot(0, 9, "m6");

NexTouch *nex_listen_list[] = {
  &main_h0,
  &main_h1,
  &main_m0,
  &main_m1,
  &main_m2,
  &main_m3,
  &main_m4,
  &main_m5,
  &main_m6,
  NULL
};

void main_h0PopCallback(void *ptr) {
  uint32_t nSpeed;
  main_h0.getValue(&nSpeed);
  settings.dSpeed = nSpeed;
  EEWRITE(settings);
}

void main_h1PopCallback(void *ptr) {
  uint32_t nSpeed;
  main_h1.getValue(&nSpeed);
  settings.jSpeed = nSpeed;
  EEWRITE(settings);
}

void main_m0PushCallback(void *ptr) {
  state = STOP_STATE;
}

void main_m1PushCallback(void *ptr) {
  state = SETUP_STATE;
}

void main_m2PushCallback(void *ptr) {
  state = ERROR_STATE;
}

void main_m3PushCallback(void *ptr) {
  if(state ==IDLE_STATE) {
    toggleWater(!digitalRead(WATER_RELAY_PIN));
  }
}

void main_m4PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    toggleDrill(!digitalRead(DRILL_RELAY_PIN));
  }
}

void main_m5PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    state = JOG_UP_STATE;
  }
}

void main_m5PopCallback(void *ptr) {
  if(state == JOG_UP_STATE) {
    state = IDLE_STATE;
  }
}

void main_m6PushCallback(void *ptr) {
  if(state == IDLE_STATE) {
    state = JOG_DN_STATE;  
  }
}

void main_m6PopCallback(void *ptr) {
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

  main_h0.setValue(settings.dSpeed);
  main_h1.setValue(settings.jSpeed);
  
  main_h0.attachPop(main_h0PopCallback);
  main_h1.attachPop(main_h1PopCallback);
  main_m0.attachPush(main_m0PushCallback);
  main_m1.attachPush(main_m1PushCallback);
  main_m2.attachPush(main_m2PushCallback);
  main_m3.attachPush(main_m3PushCallback);
  main_m4.attachPush(main_m4PushCallback);
  main_m5.attachPush(main_m5PushCallback);
  main_m5.attachPop(main_m5PopCallback);
  main_m6.attachPush(main_m6PushCallback);
  main_m6.attachPop(main_m6PopCallback); 
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
      
      delay(3000);

      Serial2.print("main");
      nexClear();
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
        delay(1);
				state = HOLD_STATE;
			} else {
				moveDrill(DOWN, settings.dSpeed);
			}
    break;

		case HOLD_STATE:
			if(!digitalRead(PLATE_LIMIT_PIN)) {
				break;
			} else {
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
