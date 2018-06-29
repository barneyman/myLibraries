
#include "dypUltraSonic.h"
#include <Arduino.h>
#include <HardwareSerial.h>

#define _MAX_DRY_READS	10


dypUltraSonic::dypUltraSonic(Stream*serialPort, unsigned xistor) :m_serial(serialPort), m_transistorOnPort(xistor), m_lastReadingResult(lrUnknown)
{
	if (m_transistorOnPort != NO_XSISTOR)
	{
#ifndef __AVR_ATtiny85__
		pinMode(m_transistorOnPort, OUTPUT);
#else
		DDRB |= (1 << m_transistorOnPort);
#endif
		turnSensor(false);
	}
}

#define _ALLOWED_US_BOOTTIME	50

void dypUltraSonic::turnSensor(bool on)
{
	if (m_transistorOnPort != NO_XSISTOR)
	{
#ifndef __AVR_ATtiny85__
		digitalWrite(m_transistorOnPort, on?HIGH:LOW);
#else
		if(on)
			PORTB |= (1 << m_transistorOnPort);
		else
			PORTB &= ~(1 << m_transistorOnPort);
#endif
		// give it time to boot
		if(on)
			delay(_ALLOWED_US_BOOTTIME);
	}
}

bool dypUltraSonic::readSensor()
{
	// turn sensor on
	turnSensor(true);

	// scribble into the one NOT active
	distanceReadings *inactive = InactiveReadings();
	// flush the circQ
	m_sensorData.reset();
	inactive->reset();
	m_serial->flush();

	// fill a bucket ... we should check this for timeouts
	// test for data available N times, fail if nothing turns up
	int countdown;
	for (countdown = 0; (m_serial->available() < m_sensorData.space()) && countdown < _MAX_DRY_READS; countdown++)
	{
		delay(100);
	}

	if (countdown == _MAX_DRY_READS)
	{
		turnSensor(false);
		// tell the cache we have failed
		m_lastReadingResult = lrDryRead;
		return false;
	}

	// fill the buffer with all we can
	while (m_serial->available() && m_sensorData.space())
	{
		m_sensorData.write(m_serial->read());
	}

	bool syncFound = false;
	// we need at least 4 bytes
	for (int examine = 0; !syncFound && (m_sensorData.available()>3); examine++)
	{
		// we're looking for 0xFF X Y (X+Y+FF)
		if ((m_sensorData[0] == 0xff) && (((m_sensorData[0] + m_sensorData[1] + m_sensorData[2]) & 0xff) == m_sensorData[3]))
		{
			// we have a hit
			syncFound = true;
			// we'll fall out at the next loop condition check
		}
		else
		{
			// read past one byte
			m_sensorData.read();
			// and replace it
			m_sensorData.write(m_serial->read());
			// then try again
		}
	}

	turnSensor(false);


	// if we got a sync
	if (syncFound)
	{

		// push them all into the cirQ
		for (int examine = 0; m_sensorData.available()>3; examine++)
		{
			// 0 is FF, 1 & 2 we want, # is a checksum
			unsigned char c1 = m_sensorData.read();
			unsigned char c2 = m_sensorData.read();
			unsigned char c3 = m_sensorData.read();
			unsigned char c4 = m_sensorData.read();
			int value = (c2 << 8) | (c3);
			inactive->write(value);
#ifndef __AVR_ATtiny85__
			Serial.printf("%02x %02x %02x %02x :", c1, c2, c3, c4);
#endif
		}
#ifndef __AVR_ATtiny85__
		Serial.println("");
#endif

		// then sort them
		inactive->lock();
		// and swap active/inactive
		SwapReadings();
		// success
		m_lastReadingResult = lrSuccess;
		return true;
	}
	m_lastReadingResult = lrNoSync;
	// failed miserably getting a lock
	return false;
}

////////////////////////

