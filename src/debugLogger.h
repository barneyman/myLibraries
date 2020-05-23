#pragma once
#include <stdarg.h>

#ifndef __AVR_ATtiny85__
#include <vector>
#endif

class debug
{
public:
	enum dbLevel { dbVerbose=0, dbInfo, dbImportant, dbWarning, dbError, dbAlways } m_currentLevel;

	debug(enum dbLevel currentLevel = dbImportant) :m_currentLevel(currentLevel)
	{

	}

	enum dbImpl { dbNone, dbSerial, dbSysLog, dbUnknown };

};



class debugBaseClass : public debug
{
private:

	char m_temp[255];

public:


	debugBaseClass(enum dbLevel currentLevel = dbImportant) :debug(currentLevel)
	{

	}



	virtual void printf(enum dbLevel level, const char * format, ...)
	{
		va_list args;
		va_start(args, format);

		if (level >= m_currentLevel)
		{
			/*unsigned len = */vsnprintf(m_temp, 254, format, args);
			internalPrint(level, m_temp);
		}

		va_end(args);
	}

#ifndef __AVR_ATtiny85__

	// as printf, but gets put in a queue to be delivered by pump()
	// mostly for interrupt service routines
	virtual void isr_printf(enum dbLevel level, const char * format, ...)
	{
		va_list args;
		va_start(args, format);

		if (level >= m_currentLevel)
		{
			/*unsigned len = */vsnprintf(m_temp, 254, format, args);

			m_isrLogs.push_back(
				std::make_pair(level, std::make_pair(false, String(m_temp)))
			);
		}

		va_end(args);
	}

#endif

	void println(enum dbLevel level, const char * output)
	{
		if (level >= m_currentLevel)
			internalPrintLn(level, output);
	}

#ifndef __AVR_ATtiny85__

	// TODO this should be a circQ, 
	void ICACHE_RAM_ATTR isr_println(enum dbLevel level, const char * output)
	{
		return;
		if (level >= m_currentLevel)
			m_isrLogs.push_back(
				std::make_pair(level, std::make_pair(true, String(output)))
			);
	}

#endif

	// string variant
	void println(enum dbLevel level, String output)
	{
		println(level, output.c_str());
	}


	void isr_pump()
	{
#ifndef __AVR_ATtiny85__
		for (; m_isrLogs.size(); m_isrLogs.erase(m_isrLogs.begin()))
		{
			if (m_isrLogs[0].second.first)
				internalPrintLn(m_isrLogs[0].first, m_isrLogs[0].second.second.c_str());
			else
				internalPrint(m_isrLogs[0].first, m_isrLogs[0].second.second.c_str());
		}
#endif
	}


protected:

	virtual void internalPrint(enum dbLevel level, const char*out) = 0;
	virtual void internalPrintLn(enum dbLevel level, const char*out)
	{
		String addcrlf(out);
		addcrlf += "\n\r";
		internalPrint(level, addcrlf.c_str());
	}

#ifndef __AVR_ATtiny85__
	std::vector<std::pair<enum dbLevel, std::pair<bool, String>>> m_isrLogs;
#endif


};





template<typename printProviderClass>
class debugPrintProvider : public debugBaseClass
{
protected:

	printProviderClass *m_provider;

public:


	debugPrintProvider(printProviderClass &provider, enum dbLevel currentLevel = dbImportant) :debugBaseClass(currentLevel), m_provider(&provider)
	{

	}
	
protected:

	virtual void internalPrint(enum dbLevel, const char*output)
	{
		m_provider->print(output);
	}

	virtual void internalPrintLn(enum dbLevel, const char*output)
	{
		m_provider->println(output);
	}



};

class NullDebug : public debugBaseClass
{
public:

	virtual void internalPrint(enum dbLevel level, const char*out)
	{}

	static String getConfigOptionsJSON()
	{
		return String("[]");
	}

};

#ifndef __AVR_ATtiny85__

#include <HardwareSerial.h>

class SerialDebug : public debugPrintProvider<HardwareSerial>
{
public:

	SerialDebug(enum dbLevel level=dbVerbose):debugPrintProvider(Serial, level)
	{}

	void begin(int baud)
	{
		m_provider->begin(baud);
	}

	static String getConfigOptionsJSON()
	{
		return String("[]");
	}


protected:

};



#include <WiFiUdp.h>
#include <Syslog.h> // https://github.com/arcao/Syslog.git


class syslogDebug : public debugBaseClass
{

private:

	WiFiUDP m_udpClient;
	Syslog m_syslog;

public:

	syslogDebug(enum dbLevel currentLevel) :
		debugBaseClass(currentLevel), m_syslog(m_udpClient)
	{
	}


	syslogDebug(enum dbLevel currentLevel, const char *server, int port, const char *myHostname, const char * appname) :
		debugBaseClass(currentLevel), m_syslog(m_udpClient, server, port, myHostname, appname)
	{
	}

	void SetHostname(const char *name)
	{
		m_syslog.deviceHostname(name);
	}

	void SetServer(String host, uint16_t port=514)
	{
		m_syslog.server(host.c_str(),port);
	}

	void SetAppName(String app)
	{
		m_syslog.appName(app.c_str());
	}

	static String getConfigOptionsJSON()
	{
		return String("[{'name':'Host', 'type':'text'},{'name':'Port', 'type':'number', default: 514}]");
	}


protected:

	virtual void internalPrint(enum dbLevel level, const char*out)
	{
		int logLevel = LOG_KERN;
		// the level comparison has already been made, we just need to change to syslog error levels
		switch (level)
		{
		case dbVerbose:
			logLevel = LOG_DEBUG;
			break;
		case dbInfo:
			logLevel = LOG_INFO;
			break;
		case dbImportant:
		case dbAlways:
			logLevel = LOG_NOTICE;
			break;
		case dbWarning:
			logLevel = LOG_WARNING;
			break;
		case dbError:
			logLevel = LOG_ERR;
			break;
		}

		if (!m_syslog.log(LOG_MAKEPRI(LOG_USER, logLevel), (out)))
		{
			m_isrLogs.push_back(
				std::make_pair(level, std::make_pair(false, String(out)))
			);
		}
		//Serial.print(out);
	}



};


#endif