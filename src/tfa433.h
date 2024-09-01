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

// we do not like static interrupt handlers, ESP32 allows us to use function interrupts
#include <FunctionalInterrupt.h>

// #define __TFA_ENABLE_DRY_TEST 1

// actual ~515
#define SYNCPULSELENMIN 400
// actual ~515
#define SYNCPULSELENMAX 600
#define MAXPULSELEN 3000
#define _PAK_SIZE 135
#define _BUFF_SIZE _PAK_SIZE + 5

// #define dbg(s) Serial.println(s)
#define dbg(s)

typedef struct tfaResult
{
	byte id;
	byte channel;
	int16_t temperature;
} tfaResult;

class TFA433
{
public:
	TFA433();
	void start(int pin);
	void stop();
	bool isDataAvailable();
	void getData(byte &id, byte &channel, int16_t &temperature);
	tfaResult getData();
#ifdef __TFA_ENABLE_DRY_TEST
	void _play_dry();
#endif

private:
	volatile bool _avail, _inSync, _inPacket;
	byte _buff[_BUFF_SIZE];
	volatile byte _buffEnd;

	uint8_t _lastPinValue;
	unsigned long _lastPulseLen, _lastHighUsec, _lastUsec;

	unsigned long _lastPackageArrived;
	byte _lastBuff[_BUFF_SIZE];
	byte _pin;

	void _init();
	void ARDUINO_ISR_ATTR _handler();
	bool ARDUINO_ISR_ATTR _handler_internal(unsigned long uSec, uint8_t pinValue);
	int _binToDecRev(byte *binary, int s, int e);
	int _binToDec(byte *binary, int s, int e);
	uint8_t _checksum(byte *binary, int offset);
	tfaResult _values;
};

#endif