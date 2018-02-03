#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//#define _NO_MDNS

#ifndef  _NO_MDNS
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

	enum wifiMode { modeOff, modeAP, modeSTA, modeSTAspeculative, modeUnknown };
	wifiMode currentMode;

	myWifiClass():server(80)
	{
		currentMode = modeUnknown;
		busyDoingSomethingIgnoreSwitch = false;

		SetHandlers();
	}

	myWifiClass(const char*stem):m_hostName(stem),server(80)
	{
		currentMode = modeUnknown;
		busyDoingSomethingIgnoreSwitch = false;

		SetHandlers();
	}


	wifiMode ConnectWifi(wifiMode intent, wifiDetails &details);


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