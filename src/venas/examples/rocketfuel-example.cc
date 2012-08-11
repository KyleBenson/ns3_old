/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** A simple example showing an mesh topology of point-to-point connected nodes
    generated using Rocketfuel. **/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/topology-read-module.h"
#include "ns3/ipv4-address-generator.h"

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
  std::string rocketfuel_file = "rocketfuel/maps/3356.cch";

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
  uint32_t network_mask = 0xffffff00;
  uint32_t host_mask = 0x000000ff;
  Ipv4Mask ip_mask("255.255.255.0");

  Ipv4InterfaceContainer router_interfaces;
  InternetStackHelper stack;
  stack.Install (routers);
  NS_LOG_INFO ("Assigning addresses and installing interfaces...");

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    //Attributes
    /*for (TopologyReader::Link::ConstAttributesIterator attr = iter->AttributesBegin();
         attr != iter->AttributesEnd(); attr++) {
      NS_LOG_INFO("Attribute: " + boost::lexical_cast<std::string>(attr->second));
      }*/

    NodeContainer from_node;
    NodeContainer to_node;
    from_node.Add(iter->GetFromNode());
    to_node.Add(iter->GetToNode());
    NodeContainer both_nodes (from_node, to_node);

    NetDeviceContainer new_devs = pointToPoint.Install(both_nodes);
    NetDeviceContainer from_dev;
    from_dev.Add(new_devs.Get(0));
    NetDeviceContainer to_dev;
    to_dev.Add(new_devs.Get(1));
    router_devices.Add(new_devs);

    Ipv4Address from_addr = Ipv4Address(iter->GetAttribute("From Address").c_str());
    Ipv4Address to_addr = Ipv4Address(iter->GetAttribute("To Address").c_str());

    // Try to find an unused address near to this one (hopefully within the subnet)
    while (Ipv4AddressGenerator::IsAllocated(from_addr) ||
           ((from_addr.Get() & host_mask) == 0) ||
           ((from_addr.Get() & host_mask) == 255)) {
      from_addr.Set(from_addr.Get() + 1);
    }

    address.SetBase (Ipv4Address( from_addr.Get() & network_mask), ip_mask, Ipv4Address( from_addr.Get() & host_mask));
    Ipv4InterfaceContainer from_iface = address.Assign (from_dev);

    // Try to find an unused address near to this one (hopefully within the subnet)
    while (Ipv4AddressGenerator::IsAllocated(to_addr) ||
           ((to_addr.Get() & host_mask) == 0) ||
           ((to_addr.Get() & host_mask) == 255)) {
      to_addr.Set(to_addr.Get() + 1);
    }

    address.SetBase (Ipv4Address( to_addr.Get() & network_mask), ip_mask, Ipv4Address( to_addr.Get() & host_mask));
    Ipv4InterfaceContainer to_iface = address.Assign (to_dev);

    router_interfaces.Add(from_iface);
    router_interfaces.Add(to_iface);
    //address.NewNetwork();
  }

  NS_LOG_INFO ("Done network setup!");

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

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);
  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(2),true);
  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(21),true)

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
