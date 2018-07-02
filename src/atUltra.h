

class ATultrasonic
{
public:

	bool GetReading(int &mms, int &numReads, int &status)
	{
		mms = numReads = status = 0;

		// send a wakeup
		Wire.beginTransmission(0x12);
		Wire.write(0);
		if ((status = Wire.endTransmission()) != 0)
		{
			status *= 10;
			return false;
		}
		delay(1000);

		uint8_t result = Wire.requestFrom(0x12, 4);
		if (result)
		{
			status = Wire.read();

			if (status == 1)
			{

				mms = (Wire.read() << 8) | Wire.read();
				numReads = Wire.read();
			}
			else
			{
				Serial.println("failed in read");


				while (Wire.available())
					Wire.read();

				return false;
			}

		}
		else
		{
			status=10;
			return false;
		}

		return true;

	}

};
