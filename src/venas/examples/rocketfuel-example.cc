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

std::map<std::string,std::string>
ReadLatencies (std::string fileName)
{
  std::map<std::string,std::string> latencies;

  return latencies;
}

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  std::string rocketfuel_file = "rocketfuel/maps/3356.cch";
  std::string latency_file = "";
  std::string disaster_location = "Los Angeles, CA";
  double failure_probability = 0.1;

  CommandLine cmd;
  cmd.AddValue ("file", "File to read Rocketfuel topology from", rocketfuel_file);
  cmd.AddValue ("latencies", "File to read latencies from in Rocketfuel weights file format", latency_file);
  cmd.AddValue ("verbose", "Whether to print verbose log info", verbose);
  cmd.AddValue ("disaster", "Where the disaster (and subsequent failures) is to occur "
                "(use underscores for spaces so the command line parser will actually work", disaster_location);
  cmd.AddValue ("fail_prob", "Probability that a link in the disaster region will fail", failure_probability);

  cmd.Parse (argc,argv);

  // Need to allow users to input _s instead of spaces so the parser won't truncate locations...
  std::replace(disaster_location.begin(), disaster_location.end(), '_', ' ');

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("RocketfuelExample", LOG_LEVEL_INFO);
      LogComponentEnable ("RocketfuelTopologyReader", LOG_LEVEL_INFO);
    }

  if (! boost::filesystem::exists(rocketfuel_file)) {
    NS_LOG_ERROR("File does not exist: " + rocketfuel_file);
    exit(-1);
  }

  // Load latency information if requested
  std::map<std::string,std::string> latencies;
  if (latency_file != "")
    {
      if (! boost::filesystem::exists(latency_file))
        {
          NS_LOG_ERROR("File does not exist: " + latency_file);
          exit(-1);
        }
      latencies = ReadLatencies (latency_file);
    }

  RocketfuelTopologyReader topo_reader;
  topo_reader.SetFileName(rocketfuel_file);
  NodeContainer routers = topo_reader.Read();
  NS_LOG_INFO("Nodes read from file: " + boost::lexical_cast<std::string>(routers.GetN()));

  NS_LOG_INFO ("Assigning addresses and installing interfaces...");

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  InternetStackHelper stack;
  stack.Install (routers);

  //For each link in topology, add a connection between routers and assign IP
  //addresses to the new network they've created between themselves.
  NetDeviceContainer router_devices;
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.252");
  Ipv4InterfaceContainer router_interfaces;
  Ipv4InterfaceContainer ifacesToKill;

  // Random variable for determining if links fail during the disaster
  UniformVariable random;

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    // Set latency for this link if we loaded that information
    if (latencies.size())
      {
        std::string latency;
        if (iter->GetAttributeFailSafe ("Latency", latency))
          {
            pointToPoint.SetChannelAttribute ("Delay", StringValue (latency + "ms"));
          }
        else
            pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
      }

    Ptr<Node> from_node = iter->GetFromNode();
    Ptr<Node> to_node = iter->GetToNode();
    NodeContainer both_nodes (from_node);
    both_nodes.Add (to_node);

    NetDeviceContainer new_devs = pointToPoint.Install (both_nodes);
    NetDeviceContainer from_dev;
    from_dev.Add(new_devs.Get(0));
    NetDeviceContainer to_dev;
    to_dev.Add(new_devs.Get(1));
    router_devices.Add(new_devs);

    Ipv4InterfaceContainer new_interfaces = address.Assign (new_devs);
    router_interfaces.Add(new_interfaces);
    address.NewNetwork();

    // Failure model: if either endpoint is in the disaster city, fail node with failure_probability
    if ((iter->GetAttribute ("From Location") == disaster_location ||
         iter->GetAttribute ("To Location") == disaster_location) &&
        random.GetValue () <= failure_probability)
      {
        //new_devs.Get (0)->GetChannel ()->SetDown ();
        //channelToKill = new_devs.Get (0)->GetChannel ();
        ifacesToKill.Add(new_interfaces.Get (0));
        ifacesToKill.Add(new_interfaces.Get (1));
      }
  }

  // Now we need to choose the server and the clients
  Ptr<Node> serverNode = routers.Get (0);
  NodeContainer overlayNodes;

  for (NodeContainer::Iterator node = routers.Begin ();
       node != routers.End (); node++)
    {
      // We'll only install the overlay application on clients attached to stub networks,
      // so we just choose the stub network routers here
      if ((*node)->GetNDevices () == 1)
        overlayNodes.Add (*node);

      // We'll choose the router with the most connections to attach the server to so that we
      // minimize the chance that it is cut off from the rest of the network.
      if ((*node)->GetNDevices () > serverNode->GetNDevices ())
        serverNode = *node;
    }

  NS_LOG_INFO (overlayNodes.GetN ());

  // add another node to the router picked as the server node to be the actual server
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  Ptr<Node> serverProvider = serverNode;
  serverNode = CreateObject<Node> ();
  stack.Install (serverNode);
  NetDeviceContainer serverAndProviderDevs = pointToPoint.Install (serverNode,serverNode);
  Ipv4Address serverAddress = address.Assign (serverAndProviderDevs).Get (1).first->GetAddress (2,0).GetLocal ();

  NS_LOG_INFO ("Done network setup!");

  //Application
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (serverNode);
  //ApplicationContainer serverApps = echoServer.Install (router_interfaces.Get (0).first->GetNetDevice (0)->GetNode ());
  //ApplicationContainer serverApps = echoServer.Install (echoServerNode);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //Ipv4Address echo_addr = router_interfaces.Get (0).first->GetAddress (2,0).GetLocal ();
  //Ipv4Address echo_addr = routers.Get (0)->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
  
  UdpEchoClientHelper echoClient (serverAddress, 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  for (NodeContainer::Iterator node = overlayNodes.Begin ();
       node != overlayNodes.End (); node++)
    {
      ApplicationContainer clientApps = echoClient.Install (*node);
      clientApps.Start (Seconds (2.0));
      clientApps.Stop (Seconds (10.0));
    }
      
  // Fail the links that were chosen
  for (Ipv4InterfaceContainer::Iterator iface = ifacesToKill.Begin ();
       iface != ifacesToKill.End (); iface++)
    {
      iface->first->SetDown (iface->second);
      iface->first->SetForwarding (iface->second, false);
    }

  NS_LOG_INFO ("Populating routing tables; please be patient it takes a while...");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);
 
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


    /* Was used in a (failed) attempt to mimic the actual addresses from the file to exploit topology information...


  uint32_t network_mask = 0xffffff00;
  uint32_t host_mask = 0x000000ff;
  Ipv4Mask ip_mask("255.255.255.0");

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
    router_interfaces.Add(to_iface);*/



        //Attributes
        /*        for (TopologyReader::Link::ConstAttributesIterator attr = iter->AttributesBegin();
             attr != iter->AttributesEnd(); attr++) {
          //NS_LOG_INFO("Attribute: " + boost::lexical_cast<std::string>(attr->second));
          }*/
