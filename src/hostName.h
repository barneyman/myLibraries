extern "C" {

#ifdef ESP8266
#include <user_interface.h>		// for system_get_chip_id();
#else

#endif

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
#ifdef ESP8266
		char idstr[20];
		sprintf(idstr, "%0x", system_get_chip_id());
#else
		uint8_t baseMac[6];
		// Get MAC address for WiFi station
		esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
		char idstr[18] = { 0 };
		// first 3 are the vendor, so probably the same
		sprintf(idstr, "%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
#endif
		(String&)*this += idstr;

	}
};
