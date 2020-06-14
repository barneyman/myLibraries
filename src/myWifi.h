#if defined( ESP8266 ) || defined (ESP32)

#ifdef ESP8266

#include <ESP8266WiFi.h>

#else

#include <WiFi.h>

// the sync web server seems to have 'issues' sending TCP RSTs  on the second connection
#define _ESP_USE_ASYNC_WEB

#endif

#ifdef _ESP_USE_ASYNC_WEB
	#ifdef ESP32
		#include <AsyncTCP.h>
	#else
		#include <ESPAsyncTCP.h>
	#endif
	#include <ESPAsyncWebServer.h>
#else
	#ifdef ESP32
		#include <WebServer.h>
	#else
		#include <ESP8266WebServer.h>
	#endif
#endif



#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

//#define _NO_MDNS
#ifndef  _NO_MDNS

#ifdef ESP8266

#define NO_GLOBAL_MDNS	// we create an instance in this class!
#include <ESP8266mDNS.h>

#elif defined(ESP32)

#include <ESPmDNS.h>

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

	enum wifiMode { modeOff, modeAP, modeSTA, modeSTA_unjoined, modeSTAspeculative, modeSTAandAP, modeCold, modeUnknown, modeSTA_unjoinedAndAP };
	wifiMode currentMode;

	myWifiClass(debugBaseClass *dblog, const char *mdnsServiceName):server(80), m_mdnsName(mdnsServiceName)
	{
		SetDebug(dblog);
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;
		// set my host name
#ifdef ESP32
		WiFi.setHostname(m_hostName.c_str());
#else		
		WiFi.hostname(m_hostName);
#endif
		SetHandlers();
	}

	myWifiClass(const char*stem, debugBaseClass *dblog, const char *mdnsServiceName):m_hostName(stem),server(80), m_mdnsName(mdnsServiceName)
	{
		SetDebug(dblog);
		currentMode = modeCold;
		busyDoingSomethingIgnoreSwitch = false;
		// set my host name
#ifdef ESP32
		WiFi.setHostname(m_hostName.c_str());
#else		
		WiFi.hostname(m_hostName);
#endif
		SetHandlers();
	}

	bool isLocalIPset();

	wifiMode QuickStartAP();
	wifiMode ConnectWifi(wifiMode intent, wifiDetails &details, bool startServers=true);

	int ScanNetworks(std::vector<std::pair<String, int>> &allWifis);
	void WriteDetailsToJSON(JsonObject &root, wifiDetails &wifiDetails);
	bool ReadDetailsFromJSON(JsonObject &root, wifiDetails &wifiDetails);

	hostName m_hostName;

	bool QueryServices(const char *service, std::vector<mdnsService> &services, const char *protocol="tcp");

	void SetDebug(debugBaseClass*debug)
	{
		m_dblog=debug;
	}

	void serviceComponents()
	{
#ifndef _ESP_USE_ASYNC_WEB		
		server.handleClient();
#endif		
#ifndef ESP32		
		mdns.update();
#endif		
	}

protected:

	void BeginWebServer();
	void BeginMDNSServer();


	void  SetHandlers();

public:

	// we run MDNS so we can be found by "esp8266_<last 3 bytes of MAC address>.local" by the RPI
#ifndef  _NO_MDNS
#ifdef ESP8266
	esp8266::MDNSImplementation::MDNSResponder mdns;
#else
	MDNSResponder mdns;
#endif	
#endif


#ifdef ESP8266
	ESP8266WebServer server;
#else
#ifdef _ESP_USE_ASYNC_WEB
	AsyncWebServer server;
#else
	WebServer server;
#endif	
#endif

	debugBaseClass *m_dblog;
	String m_mdnsName;

};



#endif