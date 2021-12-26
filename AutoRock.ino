#include "screen.h"

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
int currState = INIT_STATE;
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

void move_drill(bool, uint32_t);
void toggle_water(bool);
void toggle_drill(bool);

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

void toggle_drill(bool toggle) {
  digitalWrite(DRILL_RELAY_PIN, toggle); 
}

void toggle_water(bool toggle) {
  digitalWrite(WATER_RELAY_PIN, toggle); 
}

void loop() {
  if(digitalRead(ESTOP_PIN)) {
    currState = ERROR_STATE;
  } else if(currState == ERROR_STATE) {
    currState = IDLE_STATE;
  }

  bool low_limit = digitalRead(LOWER_LIMIT_PIN);
  bool upp_limit = digitalRead(UPPER_LIMIT_PIN);
  bool plate = digitalRead(PLATE_LIMIT_PIN);

  switch(currState) {
    case  INIT_STATE: 
      currState = IDLE_STATE;
      delay(2000);

      p1.show();
      p1_h0.setValue(settings.jSpeed);
      p1_h1.setValue(settings.dSpeed);
    case  IDLE_STATE: 
      break;

    case JOG_UP_STATE: 
      if(upp_limit) { 
        move_drill(UP, settings.jSpeed);
      }
      break;

    case JOG_DN_STATE: 
      if(plate && low_limit) {
        move_drill(DOWN, settings.jSpeed);
      }
      break;
    
    case HOME_STATE:
      if(!upp_limit) {
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
      if(!(low_limit) || (encPos >= encMax)) {
        currState = STOP_STATE;
        break;
      }

      if(!(plate)) {
        encDis = encPos - ENC_DIS;
        Serial.println(encDis);
        currState = RETURN_STATE;
      } else {
        move_drill(DOWN, settings.dSpeed);
      }
      break;

    case RUN_STATE:
      if(!(low_limit) || (encPos >= encMax)) {
        toggle_drill(OFF);
        toggle_water(OFF);

        settings.uses += 1;
        EEWRITE(settings);
        
        nextState = IDLE_STATE;
        currState = HOME_STATE;
		    break;
			}

			toggle_drill(ON);
      toggle_water(ON);

			if(!(plate)) {
        delay(10);
				currState = HOLD_STATE;
			} else {
				move_drill(DOWN, settings.dSpeed);
			}
      break;

		case HOLD_STATE:
			if(cycleCount++ < HOLD_TIMEOUT && !(plate)) {
        break;
      }
      cycleCount = 0;
      currState = RETURN_STATE;
	    break;

		case RETURN_STATE:
      if(!(upp_limit)) {
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
    case ERROR_STATE: 
      toggle_drill(OFF);
      toggle_water(OFF);
  }
  
  nexLoop(nex_listen_list);
}
