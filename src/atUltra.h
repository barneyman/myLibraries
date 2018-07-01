

class ATultrasonic
{
public:

	bool GetReading(int &mms, int &numReads, int &status)
	{
		mms = numReads = status = 0;

		Wire.beginTransmission(0x12);
		Wire.write(0);
		Wire.endTransmission();
		delay(100);


		// send a wakeup
		Wire.beginTransmission(0x12);
		Wire.write(0);
		if ((status = Wire.endTransmission()) != 0)
		{
			return false;
		}
		delay(1000);

		// gets the i2c state
		int i2cstatus;

		uint8_t result = Wire.requestFrom(0x12, 4, i2cstatus);
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
				while (Wire.available())
					Wire.read();

				return false;
			}

		}
		else
		{
			return false;
		}

		return true;

	}

};
