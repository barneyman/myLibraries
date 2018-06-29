#include "myWifi.h"

#ifdef __ESP8266_ESp8266__

// needs to be persisted or the event is unsubscribed
WiFiEventHandler onConnect, onDisconnect, onIPgranted, onDHCPtimedout;


// disjoin and rejoin, optionally force a STA attempt
myWifiClass::wifiMode myWifiClass::ConnectWifi(wifiMode intent, wifiDetails &wifiDetails, bool startServers)
{
	busyDoingSomethingIgnoreSwitch = true;

	// does not work
	// WiFi.persistent(false);


	//onIPgranted = WiFi.onStationModeGotIP([wifiDetails](const WiFiEventStationModeGotIP& event) {
	//	IPAddress copy = event.ip;
	//	DEBUG(DEBUG_IMPORTANT, Serial.printf("EVENT IP granted %s\n\r", copy.toString().c_str()));

	//	DEBUG(DEBUG_IMPORTANT, Serial.printf("EVENT GATEWAY %s\n\r", WiFi.gatewayIP().toString().c_str()));


	//	if (!wifiDetails.dhcp)
	//	{
	//		if (WiFi.config(wifiDetails.ip, wifiDetails.gateway, wifiDetails.netmask))
	//		{
	//			DEBUG(DEBUG_IMPORTANT, Serial.printf("IP request %s\n\r", WiFi.localIP().toString().c_str()));
	//		}
	//		else
	//		{
	//			DEBUG(DEBUG_IMPORTANT, Serial.println("IP request FAILED"));
	//		}

	//	}


	//});

	
	//DEBUG(DEBUG_INFO, Serial.printf("ConnectWifi from %d to %d\n\r", currentMode, intent));
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
		// we've been booted - probably :)
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
					//DEBUG(DEBUG_VERBOSE, Serial.printf("*[%d]", WiFi.status()));
					m_dblog->printf(debug::dbVerbose, "*[%d]", WiFi.status());
					delay(1000);
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

	//DEBUG(DEBUG_INFO, Serial.printf("setting wifi intent %d\n\r",intent));
	m_dblog->printf(debug::dbInfo, "setting wifi intent %d\n\r", intent);

	if (intent == wifiMode::modeOff)
	{
		// turn wifi off
		WiFi.mode(WIFI_OFF);

	}

	if (intent == wifiMode::modeSTA || intent == wifiMode::modeSTAspeculative || intent==wifiMode::modeSTAandAP)
	{

		// turn bonjour off??
		//DEBUG(DEBUG_VERBOSE, Serial.print("Attempting connect to "));
		m_dblog->printf(debug::dbVerbose, "Attempting connect to ");
		//DEBUG(DEBUG_VERBOSE, Serial.println(wifiDetails.ssid));
		m_dblog->println(debug::dbVerbose, wifiDetails.ssid);


		//if (!wifiDetails.dhcp)
		//{
		//	//if (WiFi.config(wifiDetails.ip, wifiDetails.gateway, wifiDetails.netmask))
		//	//{
		//	//	DEBUG(DEBUG_IMPORTANT, Serial.printf("IP request %s\n\r", WiFi.localIP().toString().c_str()));
		//	//}
		//	//else
		//	//{
		//	//	DEBUG(DEBUG_IMPORTANT, Serial.println("IP request FAILED"));
		//	//}

		//}
		//else
		//{
		//	WiFi.config(IPAddress(), IPAddress(), IPAddress());
		//}

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
			WiFi.mode(intendedMode);
		}
		else
		{
			//DEBUG(DEBUG_INFO, Serial.println("optimised out a Wifi mode change"));
			m_dblog->println(debug::dbInfo, "optimised out a Wifi mode change");
		}

		//DEBUG(DEBUG_INFO, Serial.printf("DHCP state %d\n\r", wifi_station_dhcpc_status()));
		m_dblog->printf(debug::dbInfo, "DHCP state %d\n\r", wifi_station_dhcpc_status());

		if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == wifiDetails.ssid)
		{
			WiFi.begin();
			//DEBUG(DEBUG_INFO, Serial.println("optimised out a Wifi join"));
			m_dblog->println(debug::dbInfo, "optimised out a Wifi join");
		}
		else
		{
			WiFi.begin(wifiDetails.ssid.c_str(), wifiDetails.password.c_str());
		}

		// Wait for connection
		for (int attempts = 0; attempts<15; attempts++)
		{
			if (WiFi.status() != WL_CONNECTED)
			{
				delay(1000);
				//DEBUG(DEBUG_VERBOSE, Serial.printf("[%d]", WiFi.status()));
				m_dblog->printf(debug::dbVerbose, "[%d]", WiFi.status());
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
					//DEBUG(DEBUG_IMPORTANT, Serial.printf("IP request %s\n\r", WiFi.localIP().toString().c_str()));
					m_dblog->printf(debug::dbImportant, "IP request %s\n\r", WiFi.localIP().toString().c_str());
				}
				else
				{
					//DEBUG(DEBUG_IMPORTANT, Serial.println("IP request FAILED"));
					m_dblog->println(debug::dbImportant, "IP request FAILED");
				}
			}


			//DEBUG(DEBUG_INFO, Serial.printf("Connected to %s\n\r", wifiDetails.ssid.c_str()));
			m_dblog->printf(debug::dbInfo, "Connected to %s\n\r", wifiDetails.ssid.c_str());
			//DEBUG(DEBUG_INFO, Serial.printf("IP address: %s\n\r", WiFi.localIP().toString().c_str()));
			m_dblog->printf(debug::dbInfo, "IP address: %s\n\r", WiFi.localIP().toString().c_str());
			//DEBUG(DEBUG_INFO, Serial.printf("Gateway address: %s\n\r", WiFi.gatewayIP().toString().c_str()));
			m_dblog->printf(debug::dbInfo, "Gateway address: %s\n\r", WiFi.gatewayIP().toString().c_str());

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

			//DEBUG(DEBUG_ERROR, Serial.printf("FAILED to connect - status %d\n\r", WiFi.status()));
			m_dblog->printf(debug::dbError, "FAILED to connect - status %d\n\r", WiFi.status());

			// depending on intent ...
			if (intent == wifiMode::modeSTAspeculative)
			{
				// we're trying this for the first time, we failed, fall back to AP
				return ConnectWifi(wifiMode::modeAP, wifiDetails);
			}
			currentMode = modeSTA_unjoined;
		}
	}

	if (intent == wifiMode::modeAP)
	{
		// defaults to 192.168.4.1
		//DEBUG(DEBUG_INFO, Serial.println("Attempting to start AP"));
		m_dblog->println(debug::dbInfo, "Attempting to start AP");

		// we were unable to connect, so start our own access point
		WiFi.mode(WIFI_AP);
		WiFi.softAP(m_hostName.c_str());

		//DEBUG(DEBUG_IMPORTANT, Serial.printf("Started AP - Host IP %s\n\r", WiFi.softAPIP().toString().c_str()));
		m_dblog->printf(debug::dbImportant, "Started AP - Host IP %s\n\r", WiFi.softAPIP().toString().c_str());

		currentMode = wifiMode::modeAP;


	}

	if(startServers)
	{
		BeginMDNSServer();

		BeginWebServer();
	}

	busyDoingSomethingIgnoreSwitch = false;

	return currentMode;
}

void myWifiClass::BeginWebServer()
{
	//DEBUG(DEBUG_INFO, Serial.println("Starting WebServer"));
	m_dblog->println(debug::dbInfo, "Starting WebServer");
	server.begin();
	//DEBUG(DEBUG_INFO, Serial.println("HTTP server started"));
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

void myWifiClass::ReadDetailsFromJSON(JsonObject &root, wifiDetails &wifiDetails)
{
	JsonObject &wifi = root["wifi"];

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
				//DEBUG(DEBUG_ERROR, Serial.println("staticDetails parse failed, reverting to DHCP"));
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
	}

}


int myWifiClass::ScanNetworks(std::vector<std::pair<String, int>> &allWifis)
{

	// let's get all wifis we can see
	int found = WiFi.scanNetworks();

	for (int each = 0; each < found; each++)
	{
		allWifis.push_back(std::pair<String, int>(WiFi.SSID(each), WiFi.RSSI(each)));
		std::sort(allWifis.begin(), allWifis.end(), [](const std::pair<String, int> &a, const std::pair<String, int> &b) { return a.second > b.second; });

	}

	return found;

}

void myWifiClass::BeginMDNSServer()
{
#ifndef  _NO_MDNS

	//DEBUG(DEBUG_INFO, Serial.println("Starting MDNS"));
	m_dblog->println(debug::dbInfo, "Starting MDNS");
	if (mdns.begin(m_hostName.c_str()))
	{
		mdns.addService("http", "tcp", 80);
		//DEBUG(DEBUG_IMPORTANT, Serial.printf("MDNS responder started http://%s.local/\n\r", m_hostName.c_str()));
		m_dblog->printf(debug::dbImportant, "MDNS responder started http://%s.local/\n\r", m_hostName.c_str());
	}
	else
	{
		//DEBUG(DEBUG_ERROR, Serial.println("MDNS responder failed"));
		m_dblog->printf(debug::dbError, "MDNS responder failed\n\r");
	}

#endif
}

void myWifiClass::SetHandlers()
{
	// set callbacks for wifi
	onConnect = WiFi.onStationModeConnected([this](const WiFiEventStationModeConnected&c) {

		//DEBUG(DEBUG_IMPORTANT, Serial.printf("EVENT wifi connected %s\n\r", c.ssid.c_str()));
		m_dblog->printf(debug::dbImportant, "EVENT wifi connected %s\n\r", c.ssid.c_str());
		if (currentMode == modeSTA_unjoined)
			currentMode = modeSTA;

	});

	onIPgranted = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
		IPAddress copy = event.ip;
		//DEBUG(DEBUG_IMPORTANT, Serial.printf("EVENT IP granted %s\n\r", copy.toString().c_str()));
		m_dblog->printf(debug::dbImportant, "EVENT IP granted %s\n\r", copy.toString().c_str());

		//DEBUG(DEBUG_IMPORTANT, Serial.printf("EVENT GATEWAY %s\n\r", WiFi.gatewayIP().toString().c_str()));
		m_dblog->printf(debug::dbImportant, "EVENT GATEWAY %s\n\r", WiFi.gatewayIP().toString().c_str());

	});

	onDisconnect = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected &c) {

		//DEBUG(DEBUG_WARN, Serial.println("EVENT disconnected "));
		m_dblog->println(debug::dbWarning, "EVENT disconnected ");

	});

	/*onDHCPtimedout = WiFi.onStationModeDHCPTimeout([]() {
		DEBUG(DEBUG_IMPORTANT, Serial.println("EVENT DHCP timed out "));
	});*/

}

#endif