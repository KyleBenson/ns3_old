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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"
#include "ns3/aodv-module.h"

#include "mdc-header.h"
#include "mdc-helper.h"
#include "mdc-event-sensor.h"
#include "mdc-collector.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree_fwd.hpp>


/**
 * Creates a simulation environment for a wireless sensor network and event-driven
 * data being collected by a mobile data collector (MDC).
 */

using
	namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MdcSimulation");

struct TraceConstData
{
  Ptr<OutputStreamWrapper> outputStream;
  uint32_t nodeId;
};

/// Trace function for remaining energy at node.
void
RemainingEnergySink (TraceConstData * constData, double oldValue, double remainingEnergy)
{
  //  NS_LOG_UNCOND ("Node " << constData.nodeId << " current remaining energy = " << remainingEnergy
  std::stringstream s;
  s << "Node " << constData->nodeId << " current remaining energy = " << remainingEnergy
    << "J at time " << Simulator::Now ().GetSeconds ();

  //NS_LOG_INFO (s.str ());
  *(constData->outputStream)->GetStream () << s.str() << std::endl;
}

/// Trace function for the sink
void
SinkPacketReceive (TraceConstData * constData, Ptr<const Packet> packet, const Address & from)
{
  MdcHeader head;
  packet->PeekHeader (head);

  // Ignore MDC's data requests, especially since they'll hit both interfaces and double up...
  // though this shouldn't happen as broadcast should be to a network?
  if (head.GetFlags () == MdcHeader::mdcDataRequest)
    return;

  std::stringstream s;
  Ipv4Address fromAddr = InetSocketAddress::ConvertFrom(from).GetIpv4 ();
  s << "Sink " << constData->nodeId << " received "  << head.GetPacketType ()
    << " (" << head.GetData () + head.GetSerializedSize () << "B) from node "
    << head.GetId () << " via " << fromAddr << " at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *(constData->outputStream)->GetStream () << s.str() << std::endl;
}

/// Trace function for sensors sending a packet
static void
SensorDataSent (TraceConstData * constData, Ptr<const Packet> packet)
{
  MdcHeader head;
  packet->PeekHeader (head);

  std::stringstream s;
  s << "Sensor " << constData->nodeId << " sent " << head.GetPacketType () << " (" << packet->GetSize () << "B) at " << Simulator::Now ().GetSeconds ();
  
  NS_LOG_INFO (s.str ());
  *(constData->outputStream)->GetStream () << s.str() << std::endl;
}

/// Trace function for MDC forwarding a packet
static void
MdcPacketForward (TraceConstData * constData, Ptr<const Packet> packet)
{
  MdcHeader head;
  packet->PeekHeader (head);

  std::stringstream s;
  s << "MDC " << constData->nodeId << " forwarding " << head.GetPacketType () << " (" << packet->GetSize () << "B) from "
    << head.GetOrigin () << " to " << head.GetDest () << " at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
}

/// This function translates the Log level string specified in the mdcconfig file to equivalent enum values
static LogLevel
TranslateLogLevel (std::string l_str)
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

int 
main (int argc, char *argv[])
{

  int verbose = 0;
  uint32_t nSensors = 10;
  uint32_t nMdcs = 1;
  uint32_t nEvents = 1;
  uint32_t dataSize = 1024;
  double eventRadius = 5.0;
  double mdcSpeed = 3.0;
  bool sendFullData = true;
  uint32_t boundaryLength = 100;
  std::string traceFile = "";
  double simStartTime = 1.0;
  double simEndTime = 10.0;


  std::string filename = "mdcconfig.xml";
  CommandLine cmd;
  cmd.AddValue ("configfile", "Configuration File", filename);
  cmd.Parse (argc,argv);

  // Loads config_settings from the specified XML file
  using boost::property_tree::ptree;
  // Create an empty property tree object
  boost::property_tree::ptree pt;

  // Load the XML file into the property tree. If reading fails
  // (cannot open file, parse error), an exception is thrown.
  read_xml(filename, pt);

  nSensors = pt.get("mdc.startup-options.sensors", 10);
  nMdcs = pt.get("mdc.startup-options.mdcs",1);
  nEvents = pt.get("mdc.startup-options.events",1);
  dataSize = pt.get("mdc.startup-options.data_size",1024);
  eventRadius = pt.get("mdc.startup-options.event_radius",5.0);
  mdcSpeed = pt.get("mdc.startup-options.mdc_speed",3.0);
  sendFullData = pt.get("mdc.startup-options.send_full_data",true);
  boundaryLength = pt.get("mdc.startup-options.boundary",1000);
  traceFile = pt.get("mdc.startup-options.trace_file","");
  simStartTime = pt.get("mdc.startup-options.sim_start_time",1.0);
  simEndTime = pt.get("mdc.startup-options.sim_end_time",10.0);

  verbose = pt.get("mdc.startup-options.verbose",0);

  // Position on the tree containing all log levels
  if (verbose>=1)
  {
	  boost::property_tree::ptree ptLogLevels = pt.get_child("mdc.verbose-logging.1");
	  boost::property_tree::ptree ptModule;
	  std::string modName;
	  std::string modLevel;

	  for (ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
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
  if (verbose>=2)
  {
	  boost::property_tree::ptree ptLogLevels = pt.get_child("mdc.verbose-logging.2");
	  boost::property_tree::ptree ptModule;
	  std::string modName;
	  std::string modLevel;

	  for (ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
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
  if (verbose>=3)
  {
	  boost::property_tree::ptree ptLogLevels = pt.get_child("mdc.verbose-logging.3");
	  boost::property_tree::ptree ptModule;
	  std::string modName;
	  std::string modLevel;

	  for (ptree::iterator pos = ptLogLevels.begin(); pos != ptLogLevels.end();)
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


  // Open trace file if requested
  bool tracing = false;
  Ptr<OutputStreamWrapper> outputStream;
  if (traceFile != "")
    {
      AsciiTraceHelper asciiTraceHelper;
      outputStream = asciiTraceHelper.CreateFileStream (traceFile);
      tracing = true;
    }

  //////////////////////////////////////////////////////////////////////
  //////////////////   Create / setup nodes & devices   ////////////////
  //////////////////////////////////////////////////////////////////////


  NodeContainer sensors;
  sensors.Create (nSensors);
  NodeContainer sink;
  sink.Create (1);
  NodeContainer mdcs;
  mdcs.Create (nMdcs);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
  wifi.SetRemoteStationManager ("ns3::AarfcdWifiManager");

  //TODO: tweak rates, thresholds, power levels

  NetDeviceContainer sensorDevices;
  sensorDevices = wifi.Install (phy, mac, sensors);
  NetDeviceContainer sinkSensorDevice;
  sinkSensorDevice = wifi.Install (phy, mac, sink);
  NetDeviceContainer mdcSensorDevices;
  mdcSensorDevices = wifi.Install (phy, mac, mdcs);

  // Setup MDCs and associated sink interface
  /*Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));*/
  
  NetDeviceContainer mdcSinkDevices;
  mdcSinkDevices = wifi.Install (phy, mac, mdcs);

  /*mac.SetType ("ns3::ApWifiMac",
    "Ssid", SsidValue (ssid));*/

  NetDeviceContainer sinkMdcDevice;
  sinkMdcDevice = wifi.Install (phy, mac, sink);

  //////////////////////////////////////////////////////////////////////
  ///////////////////////   Assign positions   /////////////////////////
  //////////////////////////////////////////////////////////////////////


  MobilityHelper mobility;
  Ptr<UniformRandomVariable> randomPosition = CreateObject<UniformRandomVariable> ();
  randomPosition->SetAttribute ("Max", DoubleValue (boundaryLength));

  Ptr<RandomRectanglePositionAllocator> randomPositionAllocator = CreateObject<RandomRectanglePositionAllocator> ();
  randomPositionAllocator->SetX (randomPosition);
  randomPositionAllocator->SetY (randomPosition);
  mobility.SetPositionAllocator (randomPositionAllocator);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sensors);

  // Place sink in center of region
  Ptr<ListPositionAllocator> centerPositionAllocator = CreateObject<ListPositionAllocator> ();
  Vector center = Vector3D (boundaryLength/2.0, boundaryLength/2.0, 0.0);
  centerPositionAllocator->Add (center);
  mobility.SetPositionAllocator (centerPositionAllocator);
  mobility.Install (sink);

  // Place MDCs in center of region
  Ptr<ConstantRandomVariable> constRandomSpeed = CreateObject<ConstantRandomVariable> ();
  constRandomSpeed->SetAttribute ("Constant", DoubleValue (mdcSpeed));
  Ptr<ConstantRandomVariable> constRandomPause = CreateObject<ConstantRandomVariable> (); //default = 0

  mobility.SetPositionAllocator (randomPositionAllocator);
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                             "Pause", PointerValue (constRandomPause),
                             "Speed", PointerValue (constRandomSpeed),
                             "PositionAllocator", PointerValue (randomPositionAllocator));
  mobility.Install (mdcs);
  
  for (NodeContainer::Iterator itr = mdcs.Begin ();
       itr != mdcs.End (); itr++)
    {
      (*itr)->GetObject<MobilityModel> ()->SetPosition (center);
    }

  //////////////////////////////////////////////////////////////////////
  ////////////////////      Energy Model      //////////////////////////
  //////////////////////////////////////////////////////////////////////


  //BasicEnergySourceHelper basicSourceHelper;
  //basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initEnergy));
  //EnergySourceContainer energySources = basicSourceHelper.Install (sensors);
  
  LiIonEnergySourceHelper energySourceHelper;
  //energySourceHelper.Set ("LiIonEnergySourceInitialEnergyJ", DoubleValue (initEnergy));
  //energySourceHelper.Set ("TypCurrent", DoubleValue ());
  energySourceHelper.Set ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (10.)));
  EnergySourceContainer energySources = energySourceHelper.Install (sensors);

  // if not using a helper (such as for the SimpleDeviceEnergyModel that can be used to mimic a processor/sensor), you must do 
  // Ptr<DeviceEnergyModel> energyModel = CreateObject<SimpleDeviceEnergyModel> ();
  // energyModel->SetCurrentA (WHATEVER);
  // energyModel->SetEnergySource (energySource);
  // and energySource->AppendDeviceEnergyModel (energyModel);
  
  WifiRadioEnergyModelHelper radioEnergyHelper;
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  DeviceEnergyModelContainer deviceEnergyModels = radioEnergyHelper.Install (sensorDevices, energySources);

  if (tracing)
    {
      for (EnergySourceContainer::Iterator itr = energySources.Begin ();
           itr != energySources.End (); itr++)
        {
          TraceConstData * constData = new TraceConstData();
          constData->outputStream = outputStream;
          constData->nodeId = (*itr)->GetNode ()->GetId ();

          Ptr<LiIonEnergySource> energySource = DynamicCast<LiIonEnergySource> (*itr);
          NS_ASSERT (energySource != NULL);

          energySource->TraceConnectWithoutContext ("RemainingEnergy", MakeBoundCallback (&RemainingEnergySink, constData));
        }
    }
  

  //////////////////////////////////////////////////////////////////////
  ////////////////////    Network    ///////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  InternetStackHelper stack;
  AodvHelper aodv;
  aodv.Set ("EnableHello", BooleanValue (false));
  aodv.Set ("EnableBroadcast", BooleanValue (false));

  // For some reason, static routing is necessary for the MDCs to talk to the sink.
  // Something to do with them having 2 interfaces, as the issue goes away if one is
  // removed, even though the sink still has 2...
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper listRouting;
  listRouting.Add (staticRouting, 5);
  listRouting.Add (aodv, 1);
  stack.SetRoutingHelper (listRouting);

  stack.Install (sensors);
  stack.Install (sink);
  stack.Install (mdcs);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer sinkSensorInterface;
  sinkSensorInterface = address.Assign (sinkSensorDevice);
  Ipv4InterfaceContainer sensorInterfaces;
  sensorInterfaces = address.Assign (sensorDevices);
  Ipv4InterfaceContainer mdcSensorInterfaces;
  mdcSensorInterfaces = address.Assign (mdcSensorDevices);

  // The long-range wifi between sink and MDCs uses a different network
  address.NewNetwork ();
  Ipv4InterfaceContainer sinkMdcInterface;
  sinkMdcInterface = address.Assign (sinkMdcDevice);
  Ipv4InterfaceContainer mdcSinkInterfaces;
  mdcSinkInterfaces = address.Assign (mdcSinkDevices);
  
  //////////////////////////////////////////////////////////////////////
  /////////////////////////  Install Applications   ////////////////////
  //////////////////////////////////////////////////////////////////////


  TraceConstData * constData = new TraceConstData();
  constData->outputStream = outputStream;
  constData->nodeId = sink.Get (0)->GetId ();
  
  // SINK   ---   TCP sink for data from MDCs, UDP sink for data/notifications directly from sensors
  
  MdcSinkHelper sinkHelper;
  ApplicationContainer sinkApps = sinkHelper.Install (sink);
  sinkApps.Start (Seconds (simStartTime));
  sinkApps.Stop (Seconds (simEndTime));

  if (tracing)
    {
      sinkApps.Get (0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&SinkPacketReceive, constData));
    }

  // MOBILE DATA COLLECTORS
  //BulkSendHelper mdcAppHelper ("ns3::TcpSocketFactory", sinkDestAddress);
  MdcCollectorHelper mdcAppHelper;
  mdcAppHelper.SetAttribute ("RemoteAddress", Ipv4AddressValue (Ipv4Address::ConvertFrom (sinkMdcInterface.GetAddress (0))));
  mdcAppHelper.SetAttribute ("Interval", TimeValue (Seconds (3.0)));
  //mdcAppHelper.SetAttribute ("Port", UIntegerValue (9999));
  ApplicationContainer mdcApps = mdcAppHelper.Install (mdcs);
  mdcApps.Start (Seconds (simStartTime));
  mdcApps.Stop (Seconds (simEndTime));

  if (verbose)
    {
      //NS_LOG_INFO ("MDCs talk to sink at " << sinkMdcInterface.GetAddress (0, 0));
      //Address sinkDestAddress (InetSocketAddress (sinkMdcInterface.GetAddress (0, 0), 9999)); //FIX THIS TO MDC

      for (uint8_t i = 0; i < mdcApps.GetN (); i++)
        {
          constData = new TraceConstData();
          constData->nodeId = mdcApps.Get (i)->GetNode ()->GetId ();
          mdcApps.Get (i)->TraceConnectWithoutContext ("Forward", MakeBoundCallback (&MdcPacketForward, constData));
        }
    }

  // SENSORS
  MdcEventSensorHelper sensorAppHelper (sinkSensorInterface.GetAddress (0, 0), nEvents);
  sensorAppHelper.SetAttribute ("PacketSize", UintegerValue (dataSize));
  sensorAppHelper.SetAttribute ("SendFullData", BooleanValue (sendFullData));
  sensorAppHelper.SetEventPositionAllocator (randomPositionAllocator);
  sensorAppHelper.SetRadiusRandomVariable (new ConstantVariable (eventRadius));

  if (nEvents > 1)
    sensorAppHelper.SetEventTimeRandomVariable (new UniformVariable (simStartTime, simEndTime));
  else
    sensorAppHelper.SetEventTimeRandomVariable (new ConstantVariable (simStartTime*2));
  
  ApplicationContainer sensorApps = sensorAppHelper.Install (sensors);
  sensorApps.Start (Seconds (simStartTime));
  sensorApps.Stop (Seconds (simEndTime));

  for (ApplicationContainer::Iterator itr = sensorApps.Begin ();
       itr != sensorApps.End (); itr++)
    {
      if (tracing)
        {
          constData = new TraceConstData();
          constData->outputStream = outputStream;
          constData->nodeId = (*itr)->GetNode ()->GetId ();
          
          (*itr)->TraceConnectWithoutContext ("Send", MakeBoundCallback (&SensorDataSent, constData));
        }
     
      //DynamicCast<MdcEventSensor> (*itr)->ScheduleEventDetection (Seconds (3.0));
    }
  
  Simulator::Stop (Seconds (simEndTime));

  /*pointToPoint.EnablePcapAll ("third");
  phy.EnablePcap ("third", apDevices.Get (0));
  csma.EnablePcap ("third", csmaDevices.Get (0), true);*/

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
