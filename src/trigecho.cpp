#include "trigecho.h"
#include <Arduino.h>


bool TriggerUltrasonic::readSensor()
{
	// swap to empty backpck
	distanceReadings *inactive = InactiveReadings();
	// reset it
	inactive->reset();

	// and start the clock
	for (int readings = 0; readings<inactive->size(); readings++)
	{
		// write to the trigger, for the proscribed time
		digitalWrite(m_trigger, HIGH);
		delayMicroseconds(m_triggerUS);
		digitalWrite(m_trigger, LOW);


		while (digitalRead(m_echo) == LOW);
		unsigned long start = micros();
		while (digitalRead(m_echo) == HIGH);
		unsigned long duration = micros() - start;
		float mms = (duration / m_echoDiv);

		inactive->write(mms);

		// and wait for echoes to vanish
		delay(m_triggerDelay);
	}

	inactive->lock();

	SwapReadings();


	return true;
}

void TriggerUltrasonic::begin()
{
	// set trig as output, echo as in
	pinMode(m_trigger, OUTPUT);
	digitalWrite(m_trigger, LOW);

	pinMode(m_echo, INPUT);
}