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

/**
 * Creates a simulation environment for a wireless sensor network and event-driven
 * data being collected by a mobile data collector (MDC).
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MDC_SIMULATION");

struct EnergyTraceConstData
{
  Ptr<OutputStreamWrapper> outputStream;
  uint32_t nodeId;
};

/// Trace function for remaining energy at node.
void
RemainingEnergySink (EnergyTraceConstData * constData, double oldValue, double remainingEnergy)
//RemainingEnergySink (Ptr<OutputStreamWrapper> constData, double oldValue, double remainingEnergy)
{
  //  NS_LOG_UNCOND ("Node " << constData.nodeId << " current remaining energy = " << remainingEnergy
  std::stringstream s;
  s << "Node " << constData->nodeId << " current remaining energy = " << remainingEnergy
    << "J at time " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *(constData->outputStream)->GetStream () << s.str() << std::endl;
}

int 
main (int argc, char *argv[])
{
  int verbose = 0;
  uint32_t nSensors = 10;
  uint32_t nMdcs = 1;
  uint32_t boundaryLength = 100;
  std::string traceFile = "";

  CommandLine cmd;
  cmd.AddValue ("sensors", "Number of sensor nodes", nSensors);
  cmd.AddValue ("mdcs", "Number of mobile data collectors (MDCs)", nMdcs);
  cmd.AddValue ("verbose", "Enable verbose logging", verbose);
  cmd.AddValue ("boundary", "Length of (one side) of the square bounding box for the geographic region under study (in meters)", boundaryLength);
  cmd.AddValue ("trace_file", "File to write traces to", traceFile);

  cmd.Parse (argc,argv);

  if (verbose == 1)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("BasicEnergySource", LOG_LEVEL_INFO);
    }

  if (verbose == 2)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_LOGIC);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_LOGIC);
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
  ////////////////////////   Create / setup nodes   ////////////////////
  //////////////////////////////////////////////////////////////////////


  NodeContainer sensors;
  sensors.Create (nSensors);
  NodeContainer sink;
  sink.Create (1);
  /*NodeContainer mdcs;
    mdcs.Create (nMdcs);*/

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

  //TODO: tweak rates, thresholds, power levels

  /* mac defaults to ad-hoc mode
     Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));*/

  NetDeviceContainer sensorDevices;
  sensorDevices = wifi.Install (phy, mac, sensors);

  NetDeviceContainer sinkDevice;
  sinkDevice = wifi.Install (phy, mac, sink);

  /*NetDeviceContainer mdcDevices;
    mdcDevices = wifi.Install (phy, mac, mdcs);*/


  //////////////////////////////////////////////////////////////////////
  ///////////////////////   Assign positions   /////////////////////////
  //////////////////////////////////////////////////////////////////////


  MobilityHelper mobility;
  Ptr<UniformRandomVariable> randomPosition = CreateObject<UniformRandomVariable> ();
  randomPosition->SetAttribute ("Max", DoubleValue (boundaryLength));

  Ptr<RandomRectanglePositionAllocator> pos = CreateObject<RandomRectanglePositionAllocator> ();
  pos->SetX (randomPosition);
  pos->SetY (randomPosition);
  mobility.SetPositionAllocator (pos);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sensors);

  // Place sink and MDCs in center of region
  Ptr<ListPositionAllocator> posAllocator = CreateObject<ListPositionAllocator> ();
  posAllocator->Add (Vector3D (boundaryLength/2.0, boundaryLength/2.0, 0.0));
  mobility.SetPositionAllocator (posAllocator);
  mobility.Install (sink);

  //TODO: mobility for MDCs
  //mobility.SetMobilityModel ("ns3::??
  //mobility.Install (mdcs);


  //////////////////////////////////////////////////////////////////////
  ////////////////////      Energy Model      //////////////////////////
  //////////////////////////////////////////////////////////////////////


  BasicEnergySourceHelper basicSourceHelper;
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer energySources = basicSourceHelper.Install (sensors);

  WifiRadioEnergyModelHelper radioEnergyHelper;
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  DeviceEnergyModelContainer deviceEnergyModels = radioEnergyHelper.Install (sensorDevices, energySources);

  if (tracing)
    {
      for (EnergySourceContainer::Iterator itr = energySources.Begin ();
           itr != energySources.End (); itr++)
        {
          EnergyTraceConstData * constData = new EnergyTraceConstData();
          constData->outputStream = outputStream;
          constData->nodeId = (*itr)->GetNode ()->GetId ();

          Ptr<BasicEnergySource> energySource = DynamicCast<BasicEnergySource> (*itr);
          NS_ASSERT (energySource != NULL);

          energySource->TraceConnectWithoutContext ("RemainingEnergy", MakeBoundCallback (&RemainingEnergySink, constData));
        }
    }
  

  //////////////////////////////////////////////////////////////////////
  ////////////////////    Network    ///////////////////////////////////
  //////////////////////////////////////////////////////////////////////


  InternetStackHelper stack;
  stack.Install (sensors);
  stack.Install (sink);
  //stack.Install (mdcs);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer sinkInterface;
  sinkInterface = address.Assign (sinkDevice);
  Ipv4InterfaceContainer sensorInterfaces;
  sensorInterfaces = address.Assign (sensorDevices);

  /*address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer mdcInterfaces;
  mdcInterfaces = address.Assign (mdcDevices);*/


  //////////////////////////////////////////////////////////////////////
  /////////////////////////  Install Applications   ////////////////////
  //////////////////////////////////////////////////////////////////////


  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (sink);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (sinkInterface.GetAddress (0, 0), 9);

  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (2.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (sensors.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Simulator::Stop (Seconds (10.0));

  /*pointToPoint.EnablePcapAll ("third");
  phy.EnablePcap ("third", apDevices.Get (0));
  csma.EnablePcap ("third", csmaDevices.Get (0), true);*/

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
