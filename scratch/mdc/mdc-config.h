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

#include <ns3/string.h>
#include <ns3/double.h>
#include <ns3/integer.h>
#include <ns3/boolean.h>

namespace ns3 {

/* A Configuration class for the Mobile Data Collector (MDC) sensors, collectors, and sinks.
 */
typedef Ptr<StringValue> PropValue;

class MdcConfig
{
	private:

		std::map<std::string, PropValue > m_configMap;
		std::map<std::string, PropValue >::iterator m_configMapIt;
		static bool instanceFlag;
		static MdcConfig *instance;
		// Private constructor
		MdcConfig() {};

		void ProcessConfigFile (std::string fileName);
		LogLevel TranslateLogLevel (std::string l_str);
		void SetVerboseTraceOptions(boost::property_tree::ptree pt, int verbose);

		// These two are to prevent the compiler generating the copy constructor
		MdcConfig(MdcConfig const& copy);
		MdcConfig& operator=(MdcConfig const& copy);

	public:
		static MdcConfig* GetInstance();

  /**
   * This method returns the value of the MdcConfig property as a std::string,
   * or whatever type you specify in the two template arguments.
   * Note that you must specify the intermediate ns3::AttributeValue derived
   * type that will give you access to your desired primitive.
   */
  template <class AVType, class PrimType>
  PrimType GetProperty (std::string propName)//, std::string defaultValue = "")
  {
    std::map<std::string, PropValue >::iterator mapIt;

    mapIt = m_configMap.find(propName);
    if (mapIt == m_configMap.end())
      {
        NS_ASSERT_MSG (false, "Property Key " << propName << " not found!");
        // Key was not found
        // Get the default if one wasn't specified, otherwise use the default argument passed here
        /* TODO:
           if (defaultValue != "")
          ipropValue = GetDefaultValue (propName);
        else
        ipropValue = defaultValue; */
      }

    AVType intermediate;
    bool success = intermediate.DeserializeFromString (mapIt->second->Get (), NULL);
    NS_ASSERT_MSG (success, "Error accessing PropValue: " << mapIt->second->Get ());

    return intermediate.Get ();
  }

  int GetIntProperty (std::string propName)
  {
    return GetProperty<IntegerValue, int> (propName);
  }

  double GetDoubleProperty (std::string propName)
  {
    return GetProperty<DoubleValue, double> (propName);
  }

  bool GetBoolProperty (std::string propName)
  {
    return GetProperty<BooleanValue, bool> (propName);
  }

  std::string GetStringProperty (std::string propName)
  {
    return GetProperty<StringValue, std::string> (propName);
  }

  ~MdcConfig() { instanceFlag = false;}
};

} //namespace ns3

#endif
