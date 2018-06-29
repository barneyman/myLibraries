#include <circQ.h>

class distanceReadings : public circQueueT<8, 3, int>
{
public:
	distanceReadings()
	{
	}

	volatile qStorageType average, min, max, median;

	virtual void reset()
	{
		average = min = max = median = 0;
		circQueueT<8, 3, int>::reset();
	}

	void lock()
	{

		// walk thru them, sorting min to max
		for (bool sorted = false; !sorted;)
		{
			sorted = true;
			// 
			average = peek(available() - 1);
			// 
			for (unsigned each = 0; each < available() - 1; each++)
			{

				if (peek(each) > peek(each + 1))
				{
					// swap them
					swap(each, each + 1);
					sorted = false;
				}
				else
				{
					average += peek(each);
					average /= 2;
				}
			}

			min = peek(0);
			max = peek(available() - 1);

			if (available() > 2)
			{
				if (available() % 2)
				{
					median = peek((available() >> 1) + 1);
				}
				else
					median = peek(available() >> 1);
			}
			else
				median = peek(0);
		}
	}

protected:

	void swap(int a, int b)
	{
		qCounterType offsetA = readCursor + a, offsetB = readCursor + b;
		qStorageType scratch = m_data[offsetA];
		m_data[offsetA] = m_data[offsetB];
		m_data[offsetB] = scratch;

	}

};

class readingsHandler
{
public:

	readingsHandler() :m_aReadingActive(true)
	{}

	distanceReadings* Readings()
	{
		return m_aReadingActive ? &m_sensorReadingsA : &m_sensorReadingsB;
	}

	distanceReadings* InactiveReadings()
	{
		return !m_aReadingActive ? &m_sensorReadingsA : &m_sensorReadingsB;
	}


protected:

	void SwapReadings()
	{
		m_aReadingActive = !m_aReadingActive;
	}

private:

	distanceReadings m_sensorReadingsA, m_sensorReadingsB;
	bool m_aReadingActive;

};
