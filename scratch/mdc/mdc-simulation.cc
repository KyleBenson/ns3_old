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
#include "mdc-config.h"


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

int 
main_commented (int argc, char *argv[])
{

  int verbose = 1;
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

  // Instantiate the singleton class MdcConfig and get its reference
  MdcConfig *mdcConfig = MdcConfig::GetInstance();

  // At this point, the default values are not obtained from outside the program.
  // TODO: Check with Kyle on how we should implement the default values
  // We could assign default values from the commandline parameters also.
  nSensors = mdcConfig->GetIntProperty("mdc.Sensors");//,10);
  nMdcs = mdcConfig->GetIntProperty("mdc.MDCs");//,1);
  nEvents = mdcConfig->GetIntProperty("mdc.Events");//,1);
  dataSize = mdcConfig->GetIntProperty("mdc.DataSize");//,1024);
  eventRadius = mdcConfig->GetDoubleProperty("mdc.EventRadius");//,5.0);
  mdcSpeed = mdcConfig->GetDoubleProperty("mdc.MdcSpeed");//,3.0);
  sendFullData = mdcConfig->GetBoolProperty("mdc.SendFullData");//,true);
  boundaryLength = mdcConfig->GetIntProperty("mdc.Boundary");//,100.0);
  traceFile = mdcConfig->GetStringProperty("mdc.TraceFile");//,"");
  simStartTime = mdcConfig->GetDoubleProperty("mdc.SimStartTime");//,1.0);
  simEndTime = mdcConfig->GetDoubleProperty("mdc.SimEndTime");//,10.0);
  verbose = mdcConfig->GetIntProperty("mdc.Verbose");//,1);


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


  /*
   * This tracing could be done in the helper itself but we can leave it as it is now
   */

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

  /*
   * The sensor app here then creates all the events. Again, this might be done in an event helper class but keep this for now.
   */

  if (nEvents > 1)
    sensorAppHelper.SetEventTimeRandomVariable (new UniformVariable (simStartTime, simEndTime));
  else
    sensorAppHelper.SetEventTimeRandomVariable (new ConstantVariable (simStartTime*2));
  
  ApplicationContainer sensorApps = sensorAppHelper.Install (sensors);
  sensorApps.Start (Seconds (simStartTime));
  sensorApps.Stop (Seconds (simEndTime));

  /*
   * Verbose tracing like before done for MDCs.
   */
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
