#include "ultrasonic.h"

class TriggerUltrasonic : public readingsHandler
{
public:
	TriggerUltrasonic(int trigPin, int echoPin, int triggerUS=10, float echoDiv=5.8, int triggerDelayms=120) :m_trigger(trigPin), m_echo(echoPin), m_triggerUS(triggerUS), m_echoDiv(echoDiv), m_triggerDelay(triggerDelayms)
	{
	}

	void begin();

	bool readSensor();



protected:

	int m_trigger, m_echo;
	// how long to send trigger high
	int m_triggerUS, m_triggerDelay;
	float m_echoDiv;
};

class HRS04 : public TriggerUltrasonic
{
public:
	// 10us for trigger, /58 to get it into cms
	HRS04(int trigPin, int echoPin) :TriggerUltrasonic(trigPin, echoPin)
	{

	}
};

class JSNSR04T : public TriggerUltrasonic
{
public:
	JSNSR04T(int trigPin, int echoPin) :TriggerUltrasonic(trigPin, echoPin)
	{

	}

private:

};