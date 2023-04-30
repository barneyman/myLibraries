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
		uint8_t baseMac[6];
		// Get MAC address for WiFi station
#ifdef ESP8266
		wifi_get_macaddr(STATION_IF,&baseMac[0]);
#else
		esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
#endif
		char idstr[18] = { 0 };
		// first 3 are the vendor, so probably the same
		sprintf(idstr, "%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
		(String&)*this += idstr;

	}
};
