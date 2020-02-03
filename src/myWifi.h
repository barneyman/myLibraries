#if defined( ESP8266 ) || defined (ESP32)

#ifdef ESP8266

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <vector>

#else

#include <WiFi.h>
#include <webserver.h>

#endif

#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

//#define _NO_MDNS
#ifndef  _NO_MDNS

#ifdef ESP8266

#define NO_GLOBAL_MDNS	// we create an instance in this class!
#include <ESP8266mDNS.h>

#else

#define _NO_MDNS

#endif

#endif // ! _NO_MDNS

// mine
#include "hostName.h"

#include <debugLogger.h>





class myWifiClass : 
#ifdef ESP8266
	public ESP8266WiFiClass
#else
	public WiFiClass
#endif
{
public:

	WiFiClient m_wificlient;

	class mdnsService
	{
	public:

		mdnsService(String host, IPAddress ip) :hostName(host), IP(ip)
		{
		}

		String hostName;
		IPAddress IP;

	};


	static void TurnOff()
	{
		WiFi.mode(WIFI_OFF);
		while (WiFi.getMode() != WIFI_OFF)
			delay(1);
	}

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

	enum wifiMode { modeOff, modeAP, modeSTA, modeSTA_unjoined, modeSTAspeculative, modeSTAandAP, modeCold, modeUnknown };
	wifiMode currentMode;

	myWifiClass(debugBaseClass *dblog, const char *mdnsServiceName):server(80),m_dblog(dblog), m_mdnsName(mdnsServiceName)
	{
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;
		// set my host name
		WiFi.hostname(m_hostName);
		SetHandlers();
	}

	myWifiClass(const char*stem, debugBaseClass *dblog, const char *mdnsServiceName):m_hostName(stem),server(80), m_dblog(dblog), m_mdnsName(mdnsServiceName)
	{
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;
		// set my host name
		WiFi.hostname(m_hostName);
		SetHandlers();
	}

	wifiMode QuickStartAP();
	wifiMode ConnectWifi(wifiMode intent, wifiDetails &details, bool startServers=true);

	int ScanNetworks(std::vector<std::pair<String, int>> &allWifis);
	void WriteDetailsToJSON(JsonObject &root, wifiDetails &wifiDetails);
	bool ReadDetailsFromJSON(JsonObject &root, wifiDetails &wifiDetails);

	hostName m_hostName;

	bool QueryServices(const char *service, std::vector<mdnsService> &services, const char *protocol="tcp");

protected:

	void BeginWebServer();
	void BeginMDNSServer();


	void  SetHandlers();

public:

	// we run MDNS so we can be found by "esp8266_<last 3 bytes of MAC address>.local" by the RPI
#ifndef  _NO_MDNS
	esp8266::MDNSImplementation::MDNSResponder mdns;
#endif


#ifdef ESP8266
	ESP8266WebServer server;
#else
	WebServer server;
#endif

	debugBaseClass *m_dblog;
	String m_mdnsName;

};



#endif