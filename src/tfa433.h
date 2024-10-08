/*
433 MHz weather station receiver library for so called TFA 30.3212.02 (for the "Joker" base station)


30.3212.02 temperature only https://www.tfa-dostmann.de/produkt/temperatursender-30-3212/
compatible base station 30.3055.01 https://www.tfa-dostmann.de/produkt/funk-thermometer-joker-30-3055/

*/
#ifndef tfa433_h
#define tfa433_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef ESP32
// we do not like static interrupt handlers, ESP32 allows us to use function interrupts
#include <FunctionalInterrupt.h>
#else
#define ARDUINO_ISR_ATTR
#endif

// in case you do not have a device handy and still need to play around
// uncomment the next line to get some stuff going while offline
// #define __TFA_ENABLE_DRY_TEST 1

// actual ~515
#define SYNCPULSELENMIN 400
// actual ~515
#define SYNCPULSELENMAX 600
#define MAXPULSELEN 3000
#define _PAK_SIZE 40
#define _BUFF_SIZE _PAK_SIZE / 8

typedef struct tfaResult
{
	uint8_t id;
	uint8_t channel;
	int16_t temperature;
	unsigned long packageMS;
} tfaResult;

class TFA433
{
public:
	TFA433();
	void start(int pin);
	void stop();
	bool isDataAvailable();
	void getData(uint8_t &id, uint8_t &channel, int16_t &temperature, unsigned long &packageMS);
	tfaResult getData();
#ifdef __TFA_ENABLE_DRY_TEST
	void _play_dry();
#endif

private:
	volatile bool _avail, _inPacket;
	uint8_t _buff[_BUFF_SIZE];
	volatile uint8_t _buffEnd, _syncCount;

	uint8_t _lastPinValue;
	unsigned long _lastPulseLen, _lastLowUsec, _lastUsec;
	uint8_t _pin;

	void _init();
	void ARDUINO_ISR_ATTR _handler();
	bool ARDUINO_ISR_ATTR _handler_internal(unsigned long uSec, uint8_t pinValue);
	uint8_t _checksum(uint8_t *addr, uint8_t len);
	tfaResult _values;
};

#endif