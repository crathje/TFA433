#include "Arduino.h"
#include <tfa433.h>

#ifdef ESP32
#define PIN_RFINPUT GPIO_NUM_2
#else
// beware thate the pin must be capable of hardware interrupts!
#define PIN_RFINPUT 2
#endif

TFA433 tfa = TFA433();

void setup()
{
  Serial.begin(115200);
#ifndef __TFA_ENABLE_DRY_TEST
  tfa.start(PIN_RFINPUT); // Input pin where 433 receiver is connected.
  Serial.printf("Started receiver on pin: %d\r\n", PIN_RFINPUT);
#endif
}

void loop()
{
#ifdef __TFA_ENABLE_DRY_TEST
  tfa._play_dry();

  while (1)
    ;
#endif

  if (tfa.isDataAvailable())
  {
    tfaResult result = tfa.getData();
    Serial.printf("%9lu:: received package at %9lu id: %d, channel: %d, temperature: %d.%d C\n", millis(), result.packageMS, result.id, result.channel, result.temperature / 10, result.temperature % 10);
  }
}