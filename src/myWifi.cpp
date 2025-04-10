#include "myWifi.h"

#if defined( ESP8266 ) || defined (ESP32)

// needs to be persisted or the event is unsubscribed
#ifdef ESP8266
WiFiEventHandler onConnect, onDisconnect, onIPgranted, onDHCPtimedout, onWifiChanged;
#else
wifi_event_id_t onConnect, onDisconnect, onIPgranted, onDHCPtimedout;
#include <esp_wifi.h>
#endif

// quick and dirty START ME AS AN AP!!!
myWifiClass::wifiMode myWifiClass::QuickStartAP()
{
	if(m_dblog)
		m_dblog->println(debug::dbVerbose, "Starting QuickAP");
	// this gets used by default if nothing is specificed
	// just starts an AP at 192.168.4.1
	myWifiClass::wifiDetails defaultAPBindings = {

		"","",false,true

	};

	return ConnectWifi(myWifiClass::modeAP, defaultAPBindings, true);
}

// disjoin and rejoin, optionally force a STA attempt
myWifiClass::wifiMode myWifiClass::ConnectWifi(wifiMode intent, wifiDetails &wifiDetails, bool startServers)
{
	busyDoingSomethingIgnoreSwitch = true;

	if(m_dblog)
		m_dblog->printf(debug::dbInfo, "ConnectWifi from %d to %d\n\r", currentMode, intent);

	// turn off wifi
	switch (currentMode)
	{
	case wifiMode::modeOff:
		break;
	case wifiMode::modeAP:
		// modeOff, modeAP, modeSTA, modeSTAspeculative, modeSTAandAP
		// in speculative mode, we run BOTH ap and sta
		switch (intent)
		{
			case wifiMode::modeOff:
			case wifiMode::modeSTA:
			case wifiMode::modeSTA_unjoined:
				WiFi.softAPdisconnect();
				break;
		}
		break;
	case wifiMode::modeSTAspeculative:
	case wifiMode::modeSTAandAP:
		// modeOff, modeAP, modeSTA, modeSTAspeculative, modeSTAandAP
		switch (intent)
		{
		case wifiMode::modeOff:
			WiFi.softAPdisconnect();
		case wifiMode::modeAP:
			WiFi.setAutoReconnect(false);
			WiFi.disconnect();
			break;
		case wifiMode::modeSTA:
			WiFi.softAPdisconnect();
			break;
		}
		break;
	case wifiMode::modeSTA:
		// modeOff, modeAP, modeSTA, modeSTAspeculative, modeSTAandAP
		switch (intent)
		{
		case wifiMode::modeOff:
		case wifiMode::modeAP:
			WiFi.setAutoReconnect(false);
			WiFi.disconnect();
			break;
		}
		break;
	case wifiMode::modeCold:
		// we've been rebooted - probably :)
		// make use of the persistance to speed us up a bit maybe
		if (intent != wifiMode::modeSTA && intent != modeSTAspeculative && intent!= modeSTAandAP)
		{
			WiFi.setAutoReconnect(false);
			WiFi.disconnect();
			WiFi.softAPdisconnect();
		}
		else
		{
			for (int attempts = 0; attempts<3; attempts++)
			{
				if (WiFi.status() != WL_CONNECTED)
				{
					if(m_dblog)
						m_dblog->printf(debug::dbVerbose, "*[%d]", WiFi.status());
					delay(200);
				}
				else
					break;
			}
		}
		break;

	case wifiMode::modeUnknown:
		// turn all lights off
		WiFi.setAutoReconnect(false);
		WiFi.disconnect(); 
		WiFi.softAPdisconnect();
		break;
	}

	if(m_dblog)
		m_dblog->printf(debug::dbInfo, "setting wifi intent %d\n\r", intent);

	if (intent == wifiMode::modeOff)
	{
		// turn wifi off
		WiFi.mode(WIFI_OFF);

	}

	if (intent == wifiMode::modeSTA || intent == wifiMode::modeSTAspeculative || intent==wifiMode::modeSTAandAP)
	{

		// turn bonjour off??
		if(m_dblog)
		{
			m_dblog->printf(debug::dbVerbose, "Attempting connect to ");
			m_dblog->println(debug::dbVerbose, wifiDetails.ssid);
		}


		WiFiMode_t intendedMode = WIFI_OFF;
		switch (intent)
		{
			case wifiMode::modeSTAspeculative:
			case wifiMode::modeSTAandAP:
				intendedMode = WIFI_AP_STA;
				break;
			default:
				intendedMode = WIFI_STA;
				break;
		}

		if(WiFi.getMode()!= intendedMode)
		{
			if(m_dblog)
				m_dblog->printf(debug::dbInfo, "Setting mode to WIFI_%d\n\r",intendedMode);
			WiFi.mode(intendedMode);

			if(intendedMode==WIFI_AP_STA)
			{
				WiFi.softAP(m_hostName.c_str());
				// in lieu of waiting for the event
				delay(100); yield();
				WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
			}

		}
		else
		{
			if(m_dblog)
				m_dblog->println(debug::dbInfo, "optimised out a Wifi mode change");


		}
#ifdef ESP8266
		if(m_dblog)
			m_dblog->printf(debug::dbInfo, "DHCP state %d\n\r", wifi_station_dhcpc_status());
#endif
		if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == wifiDetails.ssid)
		{
			//WiFi.begin();
			if(m_dblog)
				m_dblog->println(debug::dbInfo, "optimised out a Wifi join");
		}
		else
		{
			WiFi.begin(wifiDetails.ssid.c_str(), wifiDetails.password.c_str());
		}

		// Wait for connection - this can take up to 15 seconds (that i've witnessed)
		for (int attempts = 0; attempts<30; attempts++)
		{
			if (WiFi.status() != WL_CONNECTED)
			{
				delay(500); yield();
				if(m_dblog)
					m_dblog->printf(debug::dbVerbose, "%2d.[%d] ",attempts+1, WiFi.status());
			}
			else
				break;
		}

		if (WiFi.status() == WL_CONNECTED)
		{

			if (!wifiDetails.dhcp)
			{
				if (WiFi.config(wifiDetails.ip, wifiDetails.gateway, wifiDetails.netmask))
				{
					if(m_dblog)
						m_dblog->printf(debug::dbImportant, "IP request %s\n\r", WiFi.localIP().toString().c_str());
				}
				else
				{
					if(m_dblog)
						m_dblog->println(debug::dbImportant, "IP request FAILED");
				}
			}

			if(m_dblog)
			{
				m_dblog->printf(debug::dbInfo, "Connected to %s\n\r", wifiDetails.ssid.c_str());
				m_dblog->printf(debug::dbInfo, "IP address: %s\n\r", WiFi.localIP().toString().c_str());
				m_dblog->printf(debug::dbInfo, "Gateway address: %s\n\r", WiFi.gatewayIP().toString().c_str());
			}

			WiFi.setAutoReconnect(true);

//			wifiMode::modeSTA || intent == wifiMode::modeSTAspeculative || intent == wifiMode::modeSTAandAP
			switch (intent)
			{
			case wifiMode::modeSTAspeculative:
				currentMode= wifiMode::modeSTAandAP;
				break;
			default:
				currentMode = intent;
				break;
			}

		}
		else
		{

			if(m_dblog)
				m_dblog->printf(debug::dbError, "FAILED to connect - status %d\n\r", WiFi.status());

			// depending on intent ...
			if (intent == wifiMode::modeSTAspeculative)
			{
				// we're trying this for the first time, we failed, fall back to AP
				return ConnectWifi(wifiMode::modeAP, wifiDetails);
			}
			if(intent==modeSTAandAP)
			{
				currentMode = modeSTA_unjoinedAndAP;
			}
			else
			{
				currentMode = modeSTA_unjoined;
			}
			
			
		}
	}

	if (intent == wifiMode::modeAP)
	{
		// defaults to 192.168.4.1, sometimes - so force it
		// the sequence is setsoftap - wait for softap event set config
		// https://github.com/espressif/arduino-esp32/issues/985
		if(m_dblog)
			m_dblog->println(debug::dbInfo, "Attempting to start AP");
		// we were unable to connect, so start our own access point
		WiFi.mode(WIFI_AP);
		WiFi.softAP(m_hostName.c_str());
		// in lieu of waiting for the event
		delay(100); yield();
		WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "Started AP - Host IP %s\n\r", WiFi.softAPIP().toString().c_str());

		currentMode = wifiMode::modeAP;


	}

	if(startServers)
	{
		yield();
		BeginMDNSServer();
		yield();
		BeginWebServer();
		yield();
	}

	busyDoingSomethingIgnoreSwitch = false;

	if(m_dblog)
		m_dblog->printf(debug::dbInfo, "return with mode %d\n\r",currentMode);

	return currentMode;
}

void myWifiClass::CloseServers()
{
	mdns.end();
#ifndef _ESP_USE_ASYNC_WEB
	server.stop();
#endif	
}

bool myWifiClass::isLocalIPset()
{
#ifdef ESP32
	return !(localIP()==IPAddress(0,0,0,0));
#else
	return localIP().isSet();
#endif	
}

void myWifiClass::BeginWebServer()
{
	if(m_dblog)
		m_dblog->println(debug::dbInfo, "Starting WebServer");
	server.begin();
	if(m_dblog)
		m_dblog->println(debug::dbVerbose, "HTTP server started");
}

void myWifiClass::WriteDetailsToJSON(JsonObject &root, wifiDetails &wifiDetails)
{
	JsonObject &wifi = root.createNestedObject("wifi");

	wifi["configured"] = wifiDetails.configured;
	if (wifiDetails.configured)
	{
		wifi["password"] = wifiDetails.password;
		wifi["ssid"] = wifiDetails.ssid;

		if (!wifiDetails.dhcp)
		{
			JsonObject &staticDetails = wifi.createNestedObject("network");
			staticDetails["ip"] = wifiDetails.ip.toString();
			staticDetails["gateway"] = wifiDetails.gateway.toString();
			staticDetails["mask"] = wifiDetails.netmask.toString();
		}

	}

}

bool myWifiClass::ReadDetailsFromJSON(JsonObject &root, wifiDetails &wifiDetails)
{
	// look for our node
	JsonObject &wifi = root["wifi"];
	if (!wifi.success())
	{
		if(m_dblog)
			m_dblog->println(debug::dbImportant,"No wifi config in JSON");
		return false;
	}

	wifiDetails.configured = wifi["configured"];
	if (wifiDetails.configured)
	{
		wifiDetails.password = (const char*)(wifi["password"]);
		wifiDetails.ssid = (const char*)(wifi["ssid"]);

		JsonObject &staticDetails = wifi["network"];

		if (staticDetails.success())
		{
			wifiDetails.dhcp = false;

			if (wifiDetails.ip.fromString((const char*)staticDetails["ip"]) &&
				wifiDetails.gateway.fromString((const char*)staticDetails["gateway"]) &&
				wifiDetails.netmask.fromString((const char*)staticDetails["mask"]))
			{
				wifiDetails.dhcp = false;
			}
			else
			{
				if(m_dblog)
					m_dblog->println(debug::dbError, "staticDetails parse failed, reverting to DHCP");
				wifiDetails.dhcp = true;
			}
		}
		else
		{
			wifiDetails.dhcp = true;
		}

	}
	else
	{
		wifiDetails.password = String();
		wifiDetails.ssid = String();
		return false;
	}

	return true;
}


int myWifiClass::ScanNetworks(std::vector<std::pair<String, int>> &allWifis)
{

#ifdef ESP32
	bool disableSTA=false;

	// esp32 won't scan when an AP, so rather than destroying the tcp stack, just return 'what?'
	if(!isSTAactivated())
	{
		disableSTA=true;
	}
#endif	


	// let's get all wifis we can see
	m_dblog->println(debug::dbInfo, "scanNetworks ... ");
	int found = WiFi.scanNetworks();
	m_dblog->printf(debug::dbInfo, "returned %d\r", found);



	if(found<0)
	{
		return 0;
	}

	for (int each = 0; each < found; each++)
	{
		allWifis.push_back(std::pair<String, int>(WiFi.SSID(each), WiFi.RSSI(each)));
	}
	yield();
	std::sort(allWifis.begin(), allWifis.end(), [](const std::pair<String, int> &a, const std::pair<String, int> &b) { return a.second > b.second; });


#ifdef ESP32
	if(disableSTA)
	{
		WiFi.enableSTA(false);	
	}
#endif


	return found;

}

bool myWifiClass::QueryServices(const char *service, std::vector<mdnsService> &services, const char *protocol)
{
	// clear it
	services.clear();

	int found = mdns.queryService(service, protocol);

	for (int item = 0; item < found; item++)
	{
		services.push_back(mdnsService(mdns.hostname(item), mdns.IP(item)));
	}

	return found ? true : false;
}

void myWifiClass::BeginMDNSServer()
{
#ifndef  _NO_MDNS

	if(m_dblog)
		m_dblog->println(debug::dbInfo, "Starting MDNS");
	mdns.end();
	if (mdns.begin(m_hostName.c_str()))
	{
		//mdns.addService("http", "tcp", 80);
		mdns.addService(m_mdnsName, "tcp", 80);
		//mdns.addServiceTxt("http", "tcp", "bjfid", "{62262736-2891-4D94-A3F1-BEBB890ACDF5}");
		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "MDNS responder started http://%s.local/\n\r", m_hostName.c_str());
	}
	else
	{
		if(m_dblog)
			m_dblog->printf(debug::dbError, "MDNS responder failed\n\r");
	}

#endif
}

void myWifiClass::SetHandlers()
{

#ifdef ESP8266
	// set callbacks for wifi
	onConnect = WiFi.onStationModeConnected([this](const WiFiEventStationModeConnected&c) {

		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT connected to '%s'\n\r", c.ssid.c_str());

	});

	onIPgranted = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
		IPAddress copy = event.ip;
		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT IP granted %s\n\r", copy.toString().c_str());

		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT GATEWAY %s\n\r", WiFi.gatewayIP().toString().c_str());

		if (currentMode == modeSTA_unjoined)
			currentMode = modeSTA;
		else if (currentMode==modeSTA_unjoinedAndAP)
			currentMode = modeSTAandAP;

		// reconnect will get us back on, but flag this so we can HUP the mdns
		mdnsNeedsHUP=true;

	});

	onDisconnect = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected &c) {

		if(m_dblog)
			m_dblog->println(debug::dbWarning, "EVENT disconnected ");

	});

	onWifiChanged=WiFi.onWiFiModeChange([this](const WiFiEventModeChange &mc) {
		
		if(m_dblog)
			m_dblog->printf(debug::dbWarning, "MODE changed from %d to %d\r\n",mc.oldMode,mc.newMode);

	});

	/*onDHCPtimedout = WiFi.onStationModeDHCPTimeout([]() {
		DEBUG(DEBUG_IMPORTANT, Serial.println("EVENT DHCP timed out "));
	});*/

#else
	onConnect=WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info)->void{		

		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT wifi asscociated, not yet connected\n\r");
		if (currentMode == modeSTA_unjoined)
			currentMode = modeSTA;
		else if (currentMode==modeSTA_unjoinedAndAP)
			currentMode = modeSTAandAP;

    	}, ARDUINO_EVENT_WIFI_STA_CONNECTED);

	onIPgranted=WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info)->void{

		//IPAddress copy = event.ip;
		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT IP granted %s\n\r", "unk");//copy.toString().c_str());

		if(m_dblog)
			m_dblog->printf(debug::dbImportant, "EVENT GATEWAY %s\n\r", "unk");//WiFi.gatewayIP().toString().c_str());

   		}, ARDUINO_EVENT_WIFI_STA_GOT_IP);

	onDisconnect=WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info)->void{


		if(m_dblog)
			m_dblog->printf(debug::dbWarning, "EVENT disconnected %d - autoreconnect %s\r",
				info.wifi_sta_disconnected.reason,
				WiFi.getAutoReconnect()?"true":"false");
/*
		if (currentMode == modeSTA)
			currentMode = modeSTA_unjoined;
		else if (currentMode==modeSTAandAP)
			currentMode = modeSTA_unjoinedAndAP;
*/

		}, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif

}

#endif