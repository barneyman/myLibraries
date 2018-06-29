#include <Stream.h>
#include "ultrasonic.h"

#define NO_XSISTOR	(unsigned)(-1)

class dypUltraSonic : public readingsHandler
{
protected:

	enum lastReadResult { lrUnknown, lrSuccess, lrDryRead, lrNoSync } m_lastReadingResult;

public:
	dypUltraSonic(Stream*serialPort, unsigned xistor = NO_XSISTOR);

	void begin()
	{
	}

	bool readSensor();

	enum lastReadResult LastReadState() { return m_lastReadingResult;  }

protected:


	void turnSensor(bool on);


	Stream* m_serial;
	circQueueT<32, 5> m_sensorData;
	unsigned m_transistorOnPort;
};




