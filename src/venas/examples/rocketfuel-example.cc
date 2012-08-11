/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** A simple example showing an mesh topology of point-to-point connected nodes
    generated using Rocketfuel. **/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/topology-read-module.h"
#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
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
  //address.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer router_interfaces;
  InternetStackHelper stack;
  stack.Install (routers);

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    //Attributes
    for (TopologyReader::Link::ConstAttributesIterator attr = iter->AttributesBegin();
         attr != iter->AttributesEnd(); attr++) {
      NS_LOG_INFO("Attribute: " + boost::lexical_cast<std::string>(attr->second));
    }

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

    // Split addresses into /24 network address and 8-bit base address
    std::string from_addr = iter->GetAttribute("From Address");
    std::string to_addr = iter->GetAttribute("To Address");
    std::vector<std::string> from_addr_vec;
    std::vector<std::string> to_addr_vec;
    boost::algorithm::split (from_addr_vec, from_addr, boost::algorithm::is_any_of("."));
    boost::algorithm::split (to_addr_vec, to_addr, boost::algorithm::is_any_of("."));

    address.SetBase (Ipv4Address( (from_addr_vec[0] + "." + from_addr_vec[1] + "." + from_addr_vec[2] + ".0").c_str() ),
                     Ipv4Mask("255.255.255.0"), Ipv4Address( ("0.0.0." + from_addr_vec[3]).c_str() ));
    std::cout << "This right?: " << from_addr << " == " << Ipv4Address(from_addr.c_str()) << std::endl;
    Ipv4InterfaceContainer from_iface = address.Assign (from_dev);
    router_interfaces.Add(from_iface);

    address.SetBase (Ipv4Address( (to_addr_vec[0] + "." + to_addr_vec[1] + "." + to_addr_vec[2] + ".0").c_str() ),
                     Ipv4Mask("255.255.255.0"), Ipv4Address( ("0.0.0." + to_addr_vec[3]).c_str() ));
    std::cout << "This right?: " << to_addr << " == " << Ipv4Address(to_addr.c_str()) << std::endl;
    Ipv4InterfaceContainer to_iface = address.Assign (to_dev);
    router_interfaces.Add(to_iface);

    std::cout << iter->GetAttribute("From Address") << " should = " << from_iface.GetAddress(0) << std::endl;
    std::cout << iter->GetAttribute("To Address") << " should = " << to_iface.GetAddress(0) << std::endl;
    //address.NewNetwork();
  }

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
