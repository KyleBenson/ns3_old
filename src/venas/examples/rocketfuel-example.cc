/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** A simple example showing an mesh topology of point-to-point connected nodes
    generated using ORBIS. **/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/topology-read-module.h"
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
//#include "boost/system"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RocketfuelExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  std::string rocketfuel_file = "rocketfuel/maps/3356.r1.cch";

  CommandLine cmd;
  cmd.AddValue ("file", "File to read Rocketfuel topology from", rocketfuel_file);
  cmd.AddValue ("verbose", "Whether to print verbose log info", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("RocketfuelExample", LOG_LEVEL_INFO);
      LogComponentEnable ("RocketfuelTopologyReader", LOG_LEVEL_INFO);
    }

  //Setup nodes/devices and install stacks/IP addresses
  if (! boost::filesystem::exists(rocketfuel_file)) {
    NS_LOG_ERROR("File does not exist: " + rocketfuel_file);
    exit(-1);
  }

  RocketfuelTopologyReader topo_reader;
  topo_reader.SetFileName(rocketfuel_file);
  NodeContainer routers = topo_reader.Read();
  NS_LOG_INFO("Nodes read from file: " + boost::lexical_cast<std::string>(routers.GetN()));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //For each link in topology, add a connection between routers and assign IP
  //addresses to the new network they've created between themselves.
  NetDeviceContainer router_devices;
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer router_interfaces;
  InternetStackHelper stack;
  stack.Install (routers);

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    NodeContainer new_nodes;
    new_nodes.Add(iter->GetFromNode());
    new_nodes.Add(iter->GetToNode());

    NetDeviceContainer new_devs = pointToPoint.Install(new_nodes);
    router_devices.Add(new_devs);

    router_interfaces.Add(address.Assign (new_devs));
    address.NewNetwork();
  }
  //router_devices = pointToPoint.Install(routers.Get(2),routers.Get(3))

  //Application
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (routers.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (router_interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (routers.Get (2));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  printf("making routes...\n");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  printf("generated routes!\n");
  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);
  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(2),true);
  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(21),true)

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
