#include "tfa433.h"

volatile bool _avail = false;
volatile byte _buff[_BUFF_SIZE];
volatile byte _buffEnd = 0;

unsigned long _lastPackageArrived;
byte _pin = 0;

TFA433::TFA433()
{
	_lastPinValue = 0xFF;
	_lastPackageArrived = 0;
}

void TFA433::start(int pin)
{
	_pin = pin;
	pinMode(_pin, INPUT);
	_init();
	attachInterrupt(digitalPinToInterrupt(_pin), std::bind(&TFA433::_handler, this), CHANGE);
	dbg("tfa started");
}

void TFA433::stop()
{
	detachInterrupt(digitalPinToInterrupt(_pin));
}

bool TFA433::isDataAvailable()
{
	return _avail;
}

void TFA433::_init()
{
	memset((void *)_buff, 0, _BUFF_SIZE);
	_avail = false;
	_lastHighUsec = 0;
	_buffEnd = 0;
	_inSync = false;
	_inPacket = false;
}
void TFA433::_handler()
{
	if (_handler_internal(micros(), digitalRead(_pin)))
	{
		_avail = true;
	}
}

inline bool TFA433::_handler_internal(unsigned long uSec, uint8_t pinValue)
{
	// sanity for change when sampling from files
	if (pinValue == _lastPinValue)
	{
		return false;
	}
	_lastPinValue = pinValue;

	unsigned long thisPulseLen = uSec - _lastUsec;
	// #ifdef __TFA_ENABLE_DRY_TEST
	// 	Serial.printf("_handler_internal: uSec: %8ld pin: %d len: %8ld lUsec: %8ld inS: %d inP: %d\r\n", uSec, pinValue, thisPulseLen, _lastUsec, _inSync, _inPacket);
	// #endif
	_lastUsec = uSec;

	if (thisPulseLen > MAXPULSELEN || thisPulseLen < SYNCPULSELENMIN)
	{
		// time out
		_buffEnd = 0;
		_inSync = false;
		_inPacket = false;
		return false;
	}

	if (thisPulseLen < SYNCPULSELENMAX)
	{
		_inSync = true;
	}
	if (_inSync)
	{
		// wait for long pulse
		if (thisPulseLen > 1250)
		{
			_inPacket = true;
		}
	}
	if (!_inPacket)
	{
		return false;
	}
	if (pinValue == 1)
	{
		if (uSec - _lastHighUsec > 2000)
		{
			_buff[_buffEnd++] = 0;
		}
		else
		{
			_buff[_buffEnd++] = 1;
		}
		_lastHighUsec = uSec;
	}
	if (_buffEnd == _PAK_SIZE)
	{
		_inSync = false;
		_inPacket = false;
		_buffEnd = 0;

		// try to match a checksum withing the three occurances every 6 bytes
		for (int i = 0; i < 3; i++)
		{
			int checksumFromMessage = _binToDec(_buff, i * 6 * 8 + 34, 37);
			int cchecksumComputed = _checksum(_buff, i * 6 * 8);
#ifdef __TFA_ENABLE_DRY_TEST
			Serial.printf("_handler_internal: checksums for repeat: %d -->  %02x vs %02x --> %d\r\n", i, checksumFromMessage, cchecksumComputed, checksumFromMessage == cchecksumComputed ? 1 : 0);
#endif
			if (checksumFromMessage == cchecksumComputed)
			{
				this->_values.channel = _binToDecRev(_buff, i * 6 * 8 + 2, i * 6 * 8 + 3) + 1;
				this->_values.id = _binToDecRev(_buff, i * 6 * 8 + 4, i * 6 * 8 + 11);
				int16_t sign = _binToDecRev(_buff, i * 6 * 8 + 12, i * 6 * 8 + 12) ? -1 : 1;
				this->_values.temperature = sign * _binToDecRev(_buff, i * 6 * 8 + 13, i * 6 * 8 + 23);
				_avail = true;

#ifdef __TFA_ENABLE_DRY_TEST
				Serial.printf("_handler_internal: received data %d\r\n", this->_values.temperature);
#endif
				return true;
			}
		}

		// 		// since the CRC did not work out, we just compare the received messages
		// 		for (int i = 1; i < 40; i++)
		// 		{
		// 			if ((_buff[i] != _buff[i + 48]) || (_buff[i] != _buff[i + 48 + 48]))
		// 			{
		// #ifdef __TFA_ENABLE_DRY_TEST
		// 				Serial.printf("_handler_internal: data packets did not match\r\n", uSec);
		// #endif
		// 				return false;
		// 			}
		// 		}

		// 		int16_t sign = _binToDecRev(_buff, 12, 12) ? -1 : 1;
		// 		this->_values.temperature = sign * _binToDecRev(_buff, 13, 23);
		// 		_avail = true;

		// #ifdef __TFA_ENABLE_DRY_TEST
		// 		Serial.printf("_handler_internal: received data %d\r\n", this->_values.temperature);
		// #endif
		// 		return true;
	}
	if (_buffEnd >= _PAK_SIZE)
	{
		// try again later...
		_buffEnd = 0;
		return false;
	}

	return false;
}

void TFA433::getData(byte &id, byte &channel, int16_t &temperature)
{
	temperature = this->_values.temperature;

	// id = _binToDecRev(_buff, 2, 9);
	// channel = _binToDecRev(_buff, 12, 13) + 1;
	// temp1 = _binToDecRev(_buff, 14, 17);
	// temp2 = _binToDecRev(_buff, 18, 21);
	// temp3 = _binToDecRev(_buff, 22, 25);

	// // Convert from F to C,  a zero value is equivalent to -90.00 F with an exp of 10, we enlarge that to 2 digit
	// temperature = (((((temp1 + temp2 * 16 + temp3 * 256) * 10) - 9000 - 3200) * 5) / 9);

	// humi1 = _binToDecRev(_buff, 26, 29);
	// humi2 = _binToDecRev(_buff, 30, 33);
	_avail = false;
}

tfaResult TFA433::getData()
{
	tfaResult result;
	getData(result.id, result.channel, result.temperature);
	return result;
}

int TFA433::_binToDecRev(byte *binary, int s, int e)
{
	int result = 0;
	unsigned int mask = 1;
	for (; e > 0 && s <= e; mask <<= 1)
		if (binary[e--] != 0)
			result |= mask;
	return result;
}

int TFA433::_binToDec(byte *binary, int s, int e)
{
	unsigned int mask = 1;
	int result = 0;
	for (; s <= e; mask <<= 1)
		if (binary[s++] != 0)
			result |= mask;
	return result;
}

/*
	Based on crc8 from https://github.com/lucsmall/BetterWH2/blob/master/BetterWH2.ino#L276
	Adapted to the bitasbyte thinking
*/
uint8_t TFA433::_checksum(byte *binary, int offset)
{
	uint8_t crc = 0;

	for (uint8_t ci = 0; ci < 4; ci++)
	{
		byte inbyte = _binToDecRev(_buff, offset + ci * 8, offset + ci * 8 + 7);
		for (uint8_t i = 8; i; i--)
		{
			uint8_t mix = (crc ^ inbyte) & 0x80; // changed from & 0x01
			crc <<= 1;							 // changed from right shift
			if (mix)
				crc ^= 0x31; // changed from 0x8C;
			inbyte <<= 1;	 // changed from right shift
		}
	}

	return crc & 0xFE;
}

#ifdef __TFA_ENABLE_DRY_TEST
void TFA433::_play_dry()
{
	_init();
	Serial.println("Dry play starting...");

	// timestamp and pinvalue from sampled test data from logic anlyzer
	// for  9.0°C/48.2°F at channel 1 (decodes to  )
	uint32_t testData[] = {0, 0, 31429973, 1, 31430487, 0, 31431487, 1, 31432001, 0, 31432991, 1, 31433505, 0, 31434505, 1, 31435019, 0, 31436019, 1, 31436533, 0, 31437524, 1, 31438038, 0, 31439038, 1, 31439552, 0, 31440552, 1, 31441056, 0, 31442056, 1, 31443576, 0, 31444576, 1, 31445091, 0, 31446090, 1, 31447611, 0, 31448601, 1, 31450121, 0, 31451121, 1, 31452641, 0, 31453641, 1, 31455161, 0, 31456161, 1, 31456675, 0, 31457666, 1, 31459186, 0, 31460186, 1, 31461706, 0, 31462706, 1, 31464226, 0, 31465226, 1, 31466746, 0, 31467737, 1, 31468251, 0, 31469251, 1, 31470771, 0, 31471771, 1, 31473291, 0, 31474291, 1, 31475811, 0, 31476801, 1, 31478321, 0, 31479321, 1, 31480841, 0, 31481841, 1, 31482355, 0, 31483355, 1, 31484875, 0, 31485865, 1, 31486379, 0, 31487379, 1, 31487893, 0, 31488893, 1, 31490413, 0, 31491403, 1, 31491918, 0, 31492917, 1, 31494438, 0, 31495437, 1, 31495952, 0, 31496951, 1, 31497456, 0, 31498456, 1, 31498970, 0, 31499970, 1, 31500484, 0, 31501474, 1, 31501988, 0, 31502988, 1, 31503502, 0, 31504502, 1, 31505016, 0, 31506007, 1, 31506521, 0, 31507521, 1, 31508035, 0, 31509035, 1, 31509549, 0, 31510539, 1, 31511053, 0, 31512053, 1, 31513573, 0, 31514573, 1, 31516093, 0, 31517093, 1, 31517598, 0, 31518598, 1, 31519112, 0, 31520111, 1, 31521632, 0, 31523136, 1, 31523641, 0, 31524641, 1, 31525155, 0, 31526155, 1, 31526669, 0, 31527659, 1, 31528173, 0, 31529173, 1, 31529687, 0, 31530687, 1, 31531201, 0, 31532191, 1, 31532705, 0, 31533705, 1, 31534219, 0, 31535219, 1, 31536739, 0, 31537739, 1, 31538243, 0, 31539243, 1, 31540763, 0, 31541763, 1, 31543283, 0, 31544283, 1, 31545803, 0, 31546803, 1, 31548323, 0, 31549313, 1, 31549827, 0, 31550827, 1, 31552347, 0, 31553347, 1, 31554867, 0, 31555858, 1, 31557387, 0, 31558378, 1, 31559898, 0, 31560898, 1, 31561412, 0, 31562411, 1, 31563932, 0, 31564922, 1, 31566452, 0, 31567442, 1, 31568962, 0, 31569962, 1, 31571482, 0, 31572482, 1, 31574002, 0, 31575002, 1, 31575516, 0, 31576506, 1, 31578026, 0, 31579026, 1, 31579540, 0, 31580539, 1, 31581054, 0, 31582044, 1, 31583564, 0, 31584564, 1, 31585078, 0, 31586077, 1, 31587598, 0, 31588597, 1, 31589102, 0, 31590101, 1, 31590616, 0, 31591615, 1, 31592129, 0, 31593129, 1, 31593643, 0, 31594633, 1, 31595148, 0, 31596147, 1, 31596661, 0, 31597661, 1, 31598175, 0, 31599165, 1, 31599679, 0, 31600679, 1, 31601193, 0, 31602193, 1, 31602707, 0, 31603697, 1, 31604212, 0, 31605211, 1, 31606731, 0, 31607731, 1, 31609251, 0, 31610251, 1, 31610756, 0, 31611755, 1, 31612270, 0, 31613269, 1, 31614789, 0, 31616294, 1, 31616798, 0, 31617798, 1, 31618312, 0, 31619312, 1, 31619826, 0, 31620826, 1, 31621331, 0, 31622330, 1, 31622844, 0, 31623844, 1, 31624358, 0, 31625348, 1, 31625862, 0, 31626862, 1, 31627376, 0, 31628376, 1, 31629896, 0, 31630896, 1, 31631400, 0, 31632400, 1, 31633920, 0, 31634920, 1, 31636440, 0, 31637440, 1, 31638960, 0, 31639960, 1, 31641480, 0, 31642470, 1, 31642984, 0, 31643984, 1, 31645504, 0, 31646504, 1, 31648024, 0, 31649024, 1, 31650544, 0, 31651534, 1, 31653054, 0, 31654054, 1, 31654568, 0, 31655568, 1, 31657088, 0, 31658088, 1, 31659608, 0, 31660598, 1, 31662128, 0, 31663118, 1, 31664638, 0, 31665637, 1, 31667158, 0, 31668157, 1, 31668671, 0, 31669661, 1, 31671191, 0, 31672181, 1, 31672695, 0, 31673695, 1, 31674209, 0, 31675209, 1, 31676729, 0, 31677719, 1, 31678233, 0, 31679233, 1, 31680753, 0, 31681753, 1, 31682267, 0, 31683267, 1, 31683772, 0, 31684772, 1, 31685286, 0, 31686285, 1, 31686800, 0, 31687790, 1, 31688304, 0, 31689304, 1, 31689818, 0, 31690818, 1, 31691332, 0, 31692322, 1, 31692836, 0, 31693836, 1, 31694350, 0, 31695350, 1, 31695864, 0, 31696854, 1, 31697369, 0, 31698368, 1, 31699889, 0, 31700888, 1, 31702408, 0, 31703408, 1, 31703913, 0, 31704912, 1, 31705426, 0, 31706426, 1, 31707946, 0};

	for (int i = 0; i < sizeof(testData) / sizeof(uint32_t) / 2; i++)
	{
		// Serial.println(i);
		if (_handler_internal(testData[i * 2], testData[i * 2 + 1]))
		{
		}
		// for (int buffp = 0; buffp < _BUFF_SIZE; buffp++)
		// {
		// 	Serial.printf("%d", _buff[buffp]);
		// }
		// Serial.println();
	}
	Serial.println("Done Playback");
	for (int buffp = 0; buffp < _BUFF_SIZE; buffp++)
	{
		Serial.printf("%d", _buff[buffp]);
	}
	Serial.println();
}
#endif