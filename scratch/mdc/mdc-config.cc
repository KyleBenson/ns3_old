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
MdcConfig* MdcConfig::GetInstance()
{
	if (!instanceFlag)
	{
		instance = new MdcConfig();
		instanceFlag = true;
		instance->ProcessConfigFile("mdcconfig.xml");
		return instance;
	}
	else
		return instance;
}

/**
 * This is the place where the XML file is parsed and all the parameters are registered as a map.
 */
void MdcConfig::ProcessConfigFile (std::string fileName)
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
  std::string opValue;

  m_configMap.clear(); // empty the map before filling it with the values from the XML file

  for (ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
  {
    opName = "";
    opValue = "";

    ptOptionParam = pos->second; // This is the option parameter entry

    opName = ptOptionParam.get<std::string>("name");
    opValue = ptOptionParam.get<std::string>("value");

    // Turn strings into AttributeValue representations
    std::pair<std::string,PropValue> opEntry;
    opEntry.first = opName;
    opEntry.second = Create<StringValue> (opValue);

    // Store them
    m_configMapIt = m_configMap.end();
    m_configMap.insert(m_configMapIt, opEntry);

    ++pos;
  }

  // Now capture the verbose level and accordingly, set the trace options.
  int verbose = GetIntProperty("mdc.Verbose");
  SetVerboseTraceOptions(pt, verbose);
}

/**
 * This method simply translates the string literal corresponding to the Log Level
 * into equivalent numeric values defined in the ns3 code.
 */
LogLevel MdcConfig::TranslateLogLevel (std::string l_str)
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
void MdcConfig::SetVerboseTraceOptions(boost::property_tree::ptree pt, int verbose)
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
			LogComponentEnable (modName.c_str(), TranslateLogLevel(modLevel));
			++pos;

		}
	}

}


} //namespace ns3


