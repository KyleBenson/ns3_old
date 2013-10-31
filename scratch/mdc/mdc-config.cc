/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "mdc-config.h"
#include <stdio.h>

/**
 * This implements a singleton class called MdcConfig. This class does two things...
 * 1. It parses an XML file that contains the configuration parameters and loads it into a map.
 * The map allows other programs to access the parameters without modification to the config class
 * 2. It parses the XML file and sets the verbose tracing option across all the modules as specified
 */


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MdcConfig");
bool MdcConfig::instanceFlag = false;
MdcConfig* MdcConfig::instance = NULL;

/**
 * This method is the only method you will need to use the MdcConfig class.
 * Calling this method will automatically instantiate the singleton and
 * subsequent calls will only return references to this object.
 */
MdcConfig* MdcConfig::getInstance()
{
	if (!instanceFlag)
	{
		instance = new MdcConfig();
		instanceFlag = true;
		instance->processConfigFile("mdcconfig.xml");
		return instance;
	}
	else
		return instance;
}

/**
 * This is the place where the XML file is parsed and all the parameters are registered as a map.
 */
void MdcConfig::processConfigFile (std::string fileName)
{
//  NS_LOG_FUNCTION_NOARGS ();

  // Loads config_settings from the specified XML file
  using boost::property_tree::ptree;
  // Create an empty property tree object
  boost::property_tree::ptree pt;

  // Load the XML file into the property tree. If reading fails
  // (cannot open file, parse error), an exception is thrown.
  read_xml(fileName, pt);

  boost::property_tree::ptree ptLogLevels = pt.get_child("mdc.startup-options");
  boost::property_tree::ptree ptOptionParam;
  std::string opName;
  std::string opDescription;
  std::string opDataType;
  std::string opValue;

  m_configMap.clear(); // empty the map before filling it with the values from the XML file

  for (ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
  {
	opName = "";
	opDescription = "";
	opDataType = "";
	opValue = "";

    ptOptionParam = pos->second; // This is the option parameter entry with 4 elements

    opName = ptOptionParam.get<std::string>("name");
    opDescription = ptOptionParam.get<std::string>("description");
    opDataType = ptOptionParam.get<std::string>("data-type");
    opValue = ptOptionParam.get<std::string>("value");

	std::pair<std::string,PropValue> opEntry;
	opEntry.first = opName;
	opEntry.second.propDescription = opDescription;
	opEntry.second.propDataTypeStr = opDataType;
	opEntry.second.propValueStr = opValue;

	m_configMapIt = m_configMap.end();
	m_configMap.insert(m_configMapIt, opEntry);

    ++pos;

  }

  // Now capture the verbose level and accordingly, set the trace options.
  int verbose = getIntProperty("mdc.Verbose");
  setVerboseTraceOptions(pt, verbose);


}

/**
 * This method returns the value of the MdcConfig property as an int.
 */
int MdcConfig::getIntProperty (std::string propName)
{
	int ipropValue = 0;
	std::map<std::string, PropValue >::iterator mapIt;

	mapIt = m_configMap.find(propName);
	if (mapIt == m_configMap.end())
	{
		// Key was not found
		ipropValue = 0;
	}
	else
	{
		sscanf(mapIt->second.propValueStr.c_str(),"%d",&ipropValue);
	}

	return ipropValue;
}

/**
 * This method returns the value of the MdcConfig property as a float
 */
float MdcConfig::getFloatProperty (std::string propName)
{
	float fpropValue = 0;
	std::map<std::string, PropValue >::iterator mapIt;

	mapIt = m_configMap.find(propName);
	if (mapIt == m_configMap.end())
	{
		// Key was not found
		fpropValue = 0.0;
	}
	else
	{
		sscanf(mapIt->second.propValueStr.c_str(),"%f",&fpropValue);
	}

	return fpropValue;
}

/**
 * This method returns the value of the MdcConfig property as an bool.
 */
bool MdcConfig::getBoolProperty (std::string propName)
{
	bool bpropValue = false;
	std::map<std::string, PropValue >::iterator mapIt;

	mapIt = m_configMap.find(propName);
	if (mapIt == m_configMap.end())
	{
		// Key was not found
		bpropValue = false;
	}
	else
	{
		if (mapIt->second.propValueStr.compare("true") == 0)
			bpropValue = true;
		else
			bpropValue = false;
	}

	return bpropValue;
}

/**
 * This method returns the value of the MdcConfig property as an string.
 */
std::string MdcConfig::getStringProperty (std::string propName)
{
	std::string spropValue = "";
	std::map<std::string, PropValue >::iterator mapIt;

	mapIt = m_configMap.find(propName);
	if (mapIt == m_configMap.end())
	{
		// Key was not found
		spropValue = "";
	}
	else
	{
		spropValue = mapIt->second.propValueStr;
	}

	return spropValue;
}

/**
 * This method simply translates the string literal corresponding to the Log Level
 * into equivalent numeric values defined in the ns3 code.
 */
LogLevel MdcConfig::translateLogLevel (std::string l_str)
{
	LogLevel l = LOG_NONE;
	if (l_str.compare("LOG_LEVEL_ERROR") == 0) l = LOG_LEVEL_ERROR;
	else if (l_str.compare("LOG_LEVEL_WARN") == 0) l = LOG_LEVEL_WARN;
	else if (l_str.compare("LOG_LEVEL_DEBUG") == 0) l = LOG_LEVEL_DEBUG;
	else if (l_str.compare("LOG_LEVEL_INFO") == 0) l = LOG_LEVEL_INFO;
	else if (l_str.compare("LOG_LEVEL_FUNCTION") == 0) l = LOG_LEVEL_FUNCTION;
	else if (l_str.compare("LOG_LEVEL_LOGIC") == 0) l = LOG_LEVEL_LOGIC;
	else if (l_str.compare("LOG_LEVEL_ALL") == 0) l = LOG_LEVEL_ALL;
	return l;
}

/**
 * This is the method that reads all the verbose trace options defined
 * and applies them to the relevant modules.
 */
void MdcConfig::setVerboseTraceOptions(boost::property_tree::ptree pt, int verbose)
{
	boost::property_tree::ptree ptLogLevels;
	boost::property_tree::ptree ptModule;
	std::string modName;
	std::string modLevel;

	for (int i=0; i<=verbose; i++) // Start from 0 all the way to the verbose level set
	{
		// Position on the tree containing all log levels
		std::string traceRootStr = "mdc.verbose-logging." + boost::lexical_cast<std::string>(i);
		ptLogLevels = pt.get_child(traceRootStr);
		for (boost::property_tree::ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
		{
			modName = "";
			modLevel = "";

			ptModule = pos->second; // This is the module entry with 2 elements
			modName = ptModule.get<std::string>("name");
			modLevel = ptModule.get<std::string>("level");
			LogComponentEnable (modName.c_str(), translateLogLevel(modLevel));
			++pos;

		}
	}

}


} //namespace ns3


