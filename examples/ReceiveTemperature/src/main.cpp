#include "Arduino.h"
#include <tfa433.h>

#define PIN_RFINPUT GPIO_NUM_23

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
    Serial.printf("%9lu:: id: %d, channel: %d, temperature: %d.%d C\n", millis(), result.id, result.channel, result.temperature / 10, result.temperature % 10);
  }
}