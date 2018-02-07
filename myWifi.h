#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

//#define _NO_MDNS

#ifndef  _NO_MDNS
#define NO_GLOBAL_MDNS	// we create an instance in this class!
#include <ESP8266mDNS.h>
#endif // ! _NO_MDNS

// mine
#include "hostName.h"
#include "debug_defines.h"


class myWifiClass : public ESP8266WiFiClass
{
public:

	struct wifiDetails
	{
		String ssid;
		String password;
		bool configured;

		bool dhcp;
		// if not dhcp
		IPAddress ip, netmask, gateway;

	};

	volatile bool busyDoingSomethingIgnoreSwitch;

	enum wifiMode { modeOff, modeAP, modeSTA, modeSTAspeculative, modeCold, modeUnknown };
	wifiMode currentMode;

	myWifiClass():server(80)
	{
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;

		SetHandlers();
	}

	myWifiClass(const char*stem):m_hostName(stem),server(80)
	{
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;

		SetHandlers();
	}


	wifiMode ConnectWifi(wifiMode intent, wifiDetails &details);

	int ScanNetworks(std::vector<std::pair<String, int>> &allWifis);
	void WriteDetailsToJSON(JsonObject &root, wifiDetails &wifiDetails);
	void ReadDetailsFromJSON(JsonObject &root, wifiDetails &wifiDetails);

	hostName m_hostName;


protected:

	void BeginWebServer();
	void BeginMDNSServer();

	// we run MDNS so we can be found by "esp8266_<last 3 bytes of MAC address>.local" by the RPI
#ifndef  _NO_MDNS
	MDNSResponder mdns;
#endif

	void  SetHandlers();

public:
	ESP8266WebServer server;


};