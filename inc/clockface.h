#ifndef CLOCKFACE_H_
#define CLOCKFACE_H_

#include "pcf8523.h"

void clockface_draw(rtc_time_t* t);
void clockface_fill(uint8_t r, uint8_t g, uint8_t b);

#endif // CLOCKFACE_H_