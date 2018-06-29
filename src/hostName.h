extern "C" {

#include <user_interface.h>		// for system_get_chip_id();

}

// work out a unique hostname for this thing
class hostName : public String
{
public:
	hostName()
	{
		ManufactureName("esp8266_");
	}

	hostName(const char *stemName)
	{
		ManufactureName(stemName);
	}

private:

	void ManufactureName(const char *stem)
	{
		(String&)*this = stem;
		char idstr[20];
		sprintf(idstr, "%0x", system_get_chip_id());
		(String&)*this += idstr;

	}
};
