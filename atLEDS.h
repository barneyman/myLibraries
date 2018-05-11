#include <wire.h>
#include <arduino.h>


#define _COLOR_PALLETE_BLACK		0
#define _COLOR_PALLETE_WHITE		1
#define _COLOR_PALLETE_RED			2
#define _COLOR_PALLETE_LIME			3
#define _COLOR_PALLETE_BLUE			4
#define _COLOR_PALLETE_YELLOW		5
#define _COLOR_PALLETE_CYAN			6
#define _COLOR_PALLETE_MAGENTA		7
#define _COLOR_PALLETE_SILVER		8
#define _COLOR_PALLETE_GREY			9
#define _COLOR_PALLETE_MAROON		10
#define _COLOR_PALLETE_OLIVE		11
#define _COLOR_PALLETE_GREEN		12
#define _COLOR_PALLETE_PURPLE		13
#define _COLOR_PALLETE_TEAL			14
#define _COLOR_PALLETE_NAVY			15

#define _COLOR_PALLETE_USER1		16
#define _COLOR_PALLETE_USER2		17
#define _COLOR_PALLETE_USER3		18
#define _COLOR_PALLETE_USER4		19
#define _COLOR_PALLETE_USER5		20
#define _COLOR_PALLETE_USER6		21
#define _COLOR_PALLETE_USER7		22
#define _COLOR_PALLETE_USER8		23


// requestData returned flags
#define _FLAG_ROOM_IN_QUEUE	128
#define _FLAG_QUEUE_FLUSHED	64
#define _FLAG_PALETTE_MODE	32

// sendData commands
#define CMD_RESET	0	// turn it all off
#define CMD_SIZE	1	// actual number of LEDS
#define CMD_SETALL	2	// set all leds to RGB
#define CMD_SETONE	3	// set a single led - offset(0) RGB
#define CMD_SHIFT	4	// shift current set - signed byte (for L and R) RGB replace
//#define CMD_ROLL	5	// roll - signed byte
#define CMD_DISPLAY	6	// shunt out to the LEDS - beware, interrupts get cleared, so I2C will fail
#define CMD_INVERT	7	// invert all rgbs
// only works when _XSISTOR_FOR_ON define
#define CMD_ON		8	// + on/off byte
#define CMD_OFF		9	// + on/off byte
// palette commands
#define CMD_SETALL_PALETTE		10	// set all leds to RGB
#define CMD_SETONE_PALETTE		11	// set a single led - offset(0) RGB
#define CMD_SHIFT_PALETTE		12	// shift current set - signed byte (for L and R) RGB replace
#define CMD_DIV_PALETTE			13	// apply div to palette colours - one byte = rgb >> div
#define CMD_USER_PALETTE_SET	14	// set one of the user colours - 4 bytes - offset r g b 



#define _ATLEDS_COMMAND_DELAY		500
#define _ATLEDS_WIPE_DELAY			0
#define _ATLEDS_ERROR_DELAY			1000
#define _ATLEDS_DISPLAY_DELAY		20



class ATleds
{
protected:

	int m_addr;

public:
	ATleds(int addr);

#ifdef _XSISTOR_FOR_ON
	bool On();

	bool Off();
#endif

	bool SetUserPalette(byte offset, byte r, byte g, byte b);

	bool SetSize(unsigned size);

	bool SetAll(byte r, byte g, byte b);

	bool SetAllPalette(byte colour);

	bool SetOne(byte offset, byte r, byte g, byte b);

	bool SetOnePalette(byte offset, byte colour);

	bool SetPaletteDiv(byte div);


	bool WipeRight(byte r, byte g, byte b, byte step = 1);

	bool WipeRightPalette(byte colour, byte step = 1);

	bool WipeLeft(byte r, byte g, byte b, byte step = 1);

	bool WipeLeftPalette(byte colour, byte step = 1);

	bool Clear();

	void DisplayAndWait();

	bool Invert(byte mask);

protected:

	unsigned successCount;

	bool SendData(byte *data, unsigned size, bool waitIfDisplayed = false);

	// flushed means wait until Display has run really
	void waitForSpace(bool waitTilEmpty = true);

};