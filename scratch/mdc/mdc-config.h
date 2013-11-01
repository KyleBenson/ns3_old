/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MDC_CONFIG_H
#define MDC_CONFIG_H

#include "ns3/ptr.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/address.h"
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree_fwd.hpp>



namespace ns3 {

/* A Configuration class for the Mobile Data Collector (MDC) sensors, collectors, and sinks.
 */
typedef struct {
	std::string propValueStr;
	std::string propDataTypeStr;
	std::string propDescription;
} PropValue;


class MdcConfig
{
	private:

		std::map<std::string, PropValue > m_configMap;
		std::map<std::string, PropValue >::iterator m_configMapIt;
		static bool instanceFlag;
		static MdcConfig *instance;
		// Private constructor
		MdcConfig() {};

		void processConfigFile (std::string fileName);
		LogLevel translateLogLevel (std::string l_str);
		void setVerboseTraceOptions(boost::property_tree::ptree pt, int verbose);

		// These two are to prevent the compiler generating the copy constructor
		MdcConfig(MdcConfig const& copy);
		MdcConfig& operator=(MdcConfig const& copy);

	public:
		static MdcConfig* getInstance();
		int getIntProperty (std::string propName);
		float getFloatProperty (std::string propName);
		bool getBoolProperty (std::string propName);
		std::string getStringProperty (std::string propName);
		// Overloaded get...Property methods to apply default values provided
		int getIntProperty (std::string propName, int defaultInt);
		float getFloatProperty (std::string propName, float defaultFloat);
		bool getBoolProperty (std::string propName, bool defaultBool);
		std::string getStringProperty (std::string propName, std::string defaultString);
	    ~MdcConfig() { instanceFlag = false;}

};

} //namespace ns3

#endif
