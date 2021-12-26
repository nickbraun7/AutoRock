#include "Nextion.h"

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
  currState = STOP_STATE;
}

void p1_m1PushCallback(void *ptr) {
  currState = SETUP_STATE;
}

void p1_m2PushCallback(void *ptr) {
  currState = ERROR_STATE;
}

void p1_m3PushCallback(void *ptr) {
  if(currState ==IDLE_STATE) {
    toggleWater(!digitalRead(WATER_RELAY_PIN));
  }
}

void p1_m4PushCallback(void *ptr) {
  if(currState == IDLE_STATE) {
    toggleDrill(!digitalRead(DRILL_RELAY_PIN));
  }
}

void p1_m5PushCallback(void *ptr) {
  if(currState == IDLE_STATE) {
    currState = JOG_UP_STATE;
  }
}

void p1_m5PopCallback(void *ptr) {
  if(currState == JOG_UP_STATE) {
    currState = IDLE_STATE;
  }
}

void p1_m6PushCallback(void *ptr) {
  if(currState == IDLE_STATE) {
    currState = JOG_DN_STATE;  
  }
}

void p1_m6PopCallback(void *ptr) {
  if(currState == JOG_DN_STATE) {
    currState = IDLE_STATE;
  }
}

void screen_init() {
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
