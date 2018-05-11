#include "atLEDS.h"



ATleds::ATleds(int addr) :m_addr(addr), successCount(0), m_chipMode(atUnknown)
{}

bool ATleds::begin()
{
	Wire.begin();


	byte ack;
	if (!(ack = Wire.requestFrom(m_addr, 1)))
	{
		m_chipMode = atFailed;
		Serial.println("failed");
		return false;
	}
	else
	{
		ack = Wire.read();

		if (ack&_FLAG_PALETTE_MODE)
		{
			m_chipMode = atPalette;
			Serial.println("palette");
		}
		else
		{
			m_chipMode = atRGB;
			Serial.println("rgb");
		}
	}

	return true;

};

#ifdef _XSISTOR_FOR_ON
bool ATleds::On()
{
	byte data[] = { CMD_ON };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::Off()
{
	byte data[] = { CMD_OFF };
	return SendData(&data[0], sizeof(data));
}
#endif

bool ATleds::SetUserPalette(byte offset, byte r, byte g, byte b)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_USER_PALETTE_SET, offset, r,g,b };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetSize(unsigned size)
{
	if (m_chipMode == atFailed)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SIZE, (byte)size };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetAll(byte r, byte g, byte b)
{
	if (m_chipMode != atRGB)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SETALL, r,g,b };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetAllPalette(byte colour)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SETALL_PALETTE, colour };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetOne(byte offset, byte r, byte g, byte b)
{
	if (m_chipMode != atRGB)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SETONE,offset, r,g,b };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetOnePalette(byte offset, byte colour)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SETONE_PALETTE,offset, colour };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::SetPaletteDiv(byte div)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_DIV_PALETTE, div };
	return SendData(&data[0], sizeof(data));
}


bool ATleds::WipeRight(byte r, byte g, byte b, byte step)
{
	if (m_chipMode != atRGB)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_SHIFT,step, r,g,b };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::WipeRightPalette(byte colour, byte step)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}
	byte data[] = { CMD_SHIFT_PALETTE,step, colour };
	return SendData(&data[0], sizeof(data));
}


bool ATleds::WipeLeft(byte r, byte g, byte b, byte step)
{
	if (m_chipMode != atRGB)
	{
		Serial.println("wrong mode");
		return false;
	}

	return WipeRight(r, g, b, -step);
}

bool ATleds::WipeLeftPalette(byte colour, byte step)
{
	if (m_chipMode != atPalette)
	{
		Serial.println("wrong mode");
		return false;
	}
	return WipeRightPalette(colour, -step);
}

bool ATleds::Clear()
{
	if (m_chipMode == atFailed)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_RESET };
	return SendData(&data[0], sizeof(data));
}

void ATleds::DisplayAndWait()
{
	if (m_chipMode == atFailed)
	{
		Serial.println("wrong mode");
		return;
	}

	// wait until the queue is flushed, so we KNOW we're the only outstanding command
	byte data[] = { CMD_DISPLAY };
	bool ret = SendData(&data[0], sizeof(data), true);
	if (!ret)
	{
		Serial.printf("DisplayAndWait failed\n\r");
	}
}

bool ATleds::Invert(byte mask)
{
	if (m_chipMode != atRGB)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_INVERT,mask };
	return SendData(&data[0], sizeof(data));
}


bool ATleds::SendData(byte *data, unsigned size, bool waitIfDisplayed)
{
	Wire.beginTransmission(m_addr);
	for (unsigned each = 0; each<size; each++)
	{
		int sent = Wire.write(data[each]);
		if (sent != 1)
			Serial.printf("err on write %d\n\r", sent);
	}
	byte error = Wire.endTransmission();
	if (error != I2C_OK)
	{
		Serial.printf("err on endTransmission %d (successCount %d) status %d\n\r", error, successCount, Wire.status());
		successCount = 0;
		return false;
	}
	successCount++;

	// we suffer because the at turns off interrupts when it shunts to LED
	// so - take a breath 
	if (waitIfDisplayed)
	{
		delay(_ATLEDS_DISPLAY_DELAY);
	}

	waitForSpace(waitIfDisplayed);

	return true;
}

// flushed means wait until Display has run really
void ATleds::waitForSpace(bool waitTilEmpty)
{
	yield();
#ifndef REQUESTFROM_DELAY
	delay(5);
	return;
#else
	// we've just been sending, give the slave some breathing room
	do {
		delay(REQUESTFROM_DELAY);
		byte ack;
		while (!(ack = Wire.requestFrom(m_addr, 1)))
		{
			// we got no reply from the slave 
			Serial.printf("%03d ", Wire.status());

			delay(ERROR_DELAY);
		}
		ack = Wire.read();
		if (waitTilEmpty)
		{
			if (ack & 0x40)
				break;
		}
		else if (ack & 0xC0)
		{
			break;
		}
		else
		{
			Serial.printf("ack %02x\n\r", ack);
		}
	} while (true);
#endif
}
