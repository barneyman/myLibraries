#include "atLEDS.h"



ATleds::ATleds(int addr) :m_addr(addr), successCount(0), m_chipMode(atUnknown), m_macros(false)
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

		if (ack&_FLAG_MACROS)
		{
			m_macros = true;
		}

		// then get display duration
		m_durationForDisplay = GetDisplayLag();
		Serial.printf("displayDuration %u\n\r", m_durationForDisplay);
	}

	return true;

};

bool ATleds::ChangeResponse(uint8_t to)
{
	byte data[] = { CMD_CHANGE_RESPONSE, to };
	return SendData(&data[0], sizeof(data));
}

#ifdef _XSISTOR_FOR_ON
bool ATleds::On()
{
	byte data[] = { CMD_ON_OFF, 1 };
	return SendData(&data[0], sizeof(data));
}

bool ATleds::Off()
{
	byte data[] = { CMD_ON_OFF, 0 };
	return SendData(&data[0], sizeof(data));
}
#endif

bool ATleds::SetMacro(byte * macro, byte len)
{
	if (!m_macros)
	{
		Serial.println("wrong mode");
		return false;
	}

	if (len > 30)
	{
		// too much!
		Serial.println("too much");
		return false;
	}

	byte data[] = { CMD_SET_MACRO, len };
	if (!SendData(&data[0], sizeof(data)))
		return false;


	// memory is at a premium on the tiny, so the recv buffer is only 8 bytes
	// so chunk this send up
#define _CHUNK_SIZE	4
	for (int chunk = 0; chunk < len; chunk += _CHUNK_SIZE)
	{
		int xmitSize = ((len-chunk)>_CHUNK_SIZE? _CHUNK_SIZE:len-chunk);
		Serial.printf("%d %d:",chunk,xmitSize);
		if (!SendData(&macro[chunk], xmitSize))
		{
			// GOD knows what state this leaves the other side in!!!
			return false;
		}
	}

	return true;

}

bool ATleds::RunMacro()
{
	if (!m_macros)
	{
		Serial.println("wrong mode");
		return false;
	}

	byte data[] = { CMD_RUN_MACRO };
	return SendData(&data[0], sizeof(data));

}

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

void ATleds::DisplayAndWait(bool fetchDisplayLag)
{
	if (m_chipMode == atFailed)
	{
		Serial.println("wrong mode");
		return;
	}

	//return;

	// wait until the queue is flushed, so we KNOW we're the only outstanding command
	byte data[] = { CMD_DISPLAY };
	bool ret = SendData(&data[0], sizeof(data), true);
	if (!ret)
	{
		Serial.printf("DisplayAndWait failed\n\r");
	}
	else
	{
		if (fetchDisplayLag)
		{
			m_durationForDisplay = GetDisplayLag();
			Serial.printf("displayDuration %u\n\r", m_durationForDisplay);
		}
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
		delay(m_durationForDisplay+_ATLEDS_DISPLAY_DELAY);
	}

	waitForSpace(waitIfDisplayed);

	return true;
}

unsigned ATleds::GetStackUse()
{
	return GetResponseType(1);
}

unsigned ATleds::GetDisplayLag()
{
	return GetResponseType(2);
}

unsigned ATleds::GetResponseType(uint8_t theType)
{
	unsigned resp;
	if (ChangeResponse(theType)) // stack
	{
		if (Wire.requestFrom(m_addr, 1))
		{
			resp = Wire.read();
			ChangeResponse(0); // flags (default) 
		}
		else
			Serial.println("requestFrom failed");
	}
	else
		Serial.println("change resp failed");
	return resp;
}


// flushed means wait until Display has run really
void ATleds::waitForSpace(bool waitTilEmpty)
{
	yield();
#ifndef REQUESTFROM_DELAY
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
