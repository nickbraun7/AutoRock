#ifndef __SCREEN_H__
#define __SCREEN_H__

#define nexClear() \
  Serial2.write(0xff); \
  Serial2.write(0xff); \
  Serial2.write(0xff);

void screen_init();

#endif
