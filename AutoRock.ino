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
  long uses = 0;
} settings;

void moveDrill(bool);
void toggleWater(bool);
void toggleDrill(bool);

#include "Nextion.h"

// Main Screen
NexButton p0_b0 = NexButton(0, 3, "b0");

// Debug Screen
NexButton p1_b0 = NexButton(1, 3, "b0");
NexButton p1_b1 = NexButton(1, 4, "b1");
NexHotspot p1_m2 = NexHotspot(1, 5, "m2");
NexHotspot p1_m3 = NexHotspot(1, 6, "m3");

// Settings Screen
NexSlider p2_h0 = NexSlider(2, 3, "h0");

// Run Screen
NexHotspot p3_m0 = NexHotspot(3, 1, "m0");
NexHotspot p3_m1 = NexHotspot(3, 2, "m1");
NexHotspot p3_m2 = NexHotspot(3, 3, "m2");

NexTouch *nex_listen_list[] = {
  &p0_b0,
  &p1_b0,
  &p1_b1,
  &p1_m2,
  &p1_m3,
  &p2_h0,
  &p3_m0,
  &p3_m1,
  &p3_m2,
  NULL
};

void p0_b0PushCallback(void *ptr) {
  state = SETUP_STATE;
}

void p1_b0PushCallback(void *ptr) {
  toggleDrill(!digitalRead(DRILL_RELAY_PIN));
}

void p1_b1PushCallback(void *ptr) {
  toggleWater(!digitalRead(WATER_RELAY_PIN));
}

void p1_m2PushCallback(void *ptr) {
  state = JOG_UP_STATE;
}

void p1_m2PopCallback(void *ptr) {
  state = IDLE_STATE;
}

void p1_m3PushCallback(void *ptr) {
  state = JOG_DN_STATE;  
}

void p1_m3PopCallback(void *ptr) {
  state = IDLE_STATE;
}

void p2_h0PopCallback(void *ptr) {
  uint32_t nSpeed;
  p2_h0.getValue(&nSpeed);
  settings.dSpeed = nSpeed;
  Serial.println(settings.dSpeed);
  EEWRITE(settings);
}

void p3_m0PushCallback(void *ptr) {
}

void p3_m1PushCallback(void *ptr) {
}

void p3_m2PushCallback(void *ptr) {
  state = STOP_STATE; 
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
  Serial2.write(0xFF);
  Serial2.write(0xFF);
  Serial2.write(0xFF);
  
  p0_b0.attachPush(p0_b0PushCallback);
  p1_b0.attachPush(p1_b0PushCallback);
  p1_b1.attachPush(p1_b1PushCallback);
  p1_m2.attachPush(p1_m2PushCallback);
  p1_m2.attachPop(p1_m2PopCallback);
  p1_m3.attachPush(p1_m3PushCallback);
  p1_m3.attachPop(p1_m3PopCallback);
  p2_h0.attachPop(p2_h0PopCallback);
  p3_m0.attachPush(p3_m0PushCallback);
  p3_m1.attachPush(p3_m1PushCallback);
  p3_m2.attachPush(p3_m2PushCallback);

  delay(3000); 
}

#define readA digitalRead(ENC_A_PIN)
#define readB digitalRead(ENC_B_PIN)
void isrA() { encPos += (readA != readB) ? 1 : -1; }
void isrB() { encPos += (readA == readB) ? 1 : -1; }

void moveDrill(bool dir) {
  digitalWrite(DIR_PIN, dir);

  digitalWrite(PUL_PIN, HIGH);
  delayMicroseconds(settings.dSpeed);

  digitalWrite(PUL_PIN, LOW);   
  delayMicroseconds(settings.dSpeed); 
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
    break;
      
    case IDLE_STATE:               
    break;

    case JOG_UP_STATE:
      if(digitalRead(UPPER_LIMIT_PIN)) {
        moveDrill(UP);
      }
    break;

    case JOG_DN_STATE:
      if(digitalRead(PLATE_LIMIT_PIN) && digitalRead(LOWER_LIMIT_PIN)) {
        moveDrill(DOWN);
      }
    break;
    
    case HOME_STATE:
      if(!digitalRead(UPPER_LIMIT_PIN)) {
        encPos = 0;
        state = nextState;
      } else {
        moveDrill(UP);
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
        moveDrill(DOWN);
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
				moveDrill(DOWN);
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
				moveDrill(UP);
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
