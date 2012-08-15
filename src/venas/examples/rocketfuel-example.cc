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
#include "ns3/ron-header.h"

#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
//#include "boost/system"
#include <iostream>
#include <sstream>
#include <set>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RocketfuelExample");

std::map<std::string,std::string>
ReadLatencies (std::string fileName)
{
  std::map<std::string,std::string> latencies;

  return latencies;
}

// Traced source callbacks
static void
AckReceived (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();
  
  std::stringstream s;
  s << "Node " << nodeId << " received " << (usedOverlay ? "indirect" : "direct") << " ACK at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

static void
PacketForwarded (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  
  std::stringstream s;
  s << "Node " << nodeId << " forwarded packet (hop=" << head.GetHop () << ") at " << Simulator::Now ().GetSeconds ();

  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

static void
PacketSent (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, uint32_t nodeId)
{
  RonHeader head;
  p->PeekHeader (head);
  bool usedOverlay = head.IsForward ();

  std::stringstream s;
  s << "Node " << nodeId << " sent " << (usedOverlay ? "indirect" : "direct") << " packet at " << Simulator::Now ().GetSeconds ();
  
  NS_LOG_INFO (s.str ());
  *stream->GetStream () << s.str() << std::endl;
}

static void
FailIpv4 (Ptr<Ipv4> ipv4, uint32_t iface)
{
  ipv4->SetDown (iface);
  ipv4->SetForwarding (iface, false);
}

static void
FailNode (Ptr<Node> node)
{
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  for (uint32_t iface = 0; iface < ipv4->GetNInterfaces (); iface++)
    {
      FailIpv4 (ipv4, iface);
    }

  for (uint32_t app = 0; app < node->GetNApplications (); app++)
    {
      node->GetApplication (app)->SetAttribute ("StopTime", (TimeValue(Time(0))));
    }
}

int 
main (int argc, char *argv[])
{
  bool verbose = false;
  bool trace_acks = true;
  bool trace_forwards = true;
  bool trace_sends = true;
  bool install_stubs = true;
  bool report_disaster = true;
  std::string rocketfuel_file = "rocketfuel/maps/3356.cch";
  std::string latency_file = "";
  std::string disaster_location = "Los Angeles, CA";
  std::string trace_file = "";
  double failure_probability = 0.1;

  CommandLine cmd;
  cmd.AddValue ("file", "File to read Rocketfuel topology from", rocketfuel_file);
  cmd.AddValue ("latencies", "File to read latencies from in Rocketfuel weights file format", latency_file);
  cmd.AddValue ("trace_file", "File to write traces to", trace_file);
  cmd.AddValue ("trace_acks", "Whether to print traces when a client receives an ACK from the server", trace_acks);
  cmd.AddValue ("trace_forwards", "Whether to print traces when a client forwards a packet", trace_forwards);
  cmd.AddValue ("trace_sends", "Whether to print traces when a client sends a packet initially", trace_sends);
  cmd.AddValue ("verbose", "Whether to print verbose log info", verbose);
  cmd.AddValue ("disaster", "Where the disaster (and subsequent failures) is to occur "
                "(use underscores for spaces so the command line parser will actually work)", disaster_location);
  cmd.AddValue ("fail_prob", "Probability that a link in the disaster region will fail", failure_probability);
  cmd.AddValue ("install_stubs", "Install RON client only on stub nodes (have only one link)", install_stubs);
  cmd.AddValue ("report_disaster", "Only RON clients in the disaster region will report to the server", report_disaster);

  cmd.Parse (argc,argv);

  // Need to allow users to input _s instead of spaces so the parser won't truncate locations...
  std::replace(disaster_location.begin(), disaster_location.end(), '_', ' ');

  if (verbose)
    {
      LogComponentEnable ("RonClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("RonServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("RonHeader", LOG_LEVEL_INFO);
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

  // Open trace file if requested
  bool tracing = false;
  Ptr<OutputStreamWrapper> outputStream;
  if (trace_file != "")
    {
      AsciiTraceHelper asciiTraceHelper;
      outputStream = asciiTraceHelper.CreateFileStream (trace_file);
      tracing = true;
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
  std::map<uint32_t, Ptr <Node> > disasterNodes;
  
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

    // If the node is in the disaster region add them to the list
    if (iter->GetAttribute ("From Location") == disaster_location)
      {
        disasterNodes.insert(std::pair<uint32_t, Ptr<Node> > (from_node->GetId (), from_node));
      }
    if (iter->GetAttribute ("To Location") == disaster_location)
      {
        disasterNodes.insert(std::pair<uint32_t, Ptr<Node> > (to_node->GetId (), to_node));
      }

    // Failure model: if either endpoint is in the disaster city, fail link with failure_probability
    if ((iter->GetAttribute ("From Location") == disaster_location ||
         iter->GetAttribute ("To Location") == disaster_location) &&
        random.GetValue () <= failure_probability)
      {
        ifacesToKill.Add(new_interfaces.Get (0));
        ifacesToKill.Add(new_interfaces.Get (1));
      }
}

  // Now we need to choose the server and the clients
  Ptr<Node> serverNode = routers.Get (0);
  NodeContainer overlayNodes;
  NodeContainer failNodes;

  for (NodeContainer::Iterator node = routers.Begin ();
       node != routers.End (); node++)
    {
      // We may only install the overlay application on clients attached to stub networks,
      // so we just choose the stub network routers here (note that all nodes have a loopback device)
      if (!install_stubs or (*node)->GetNDevices () <= 2) 
        {
          if (random.GetValue () < failure_probability and disasterNodes[(*node)->GetId ()] != 0)
            failNodes.Add (*node);
          else
            overlayNodes.Add (*node);
          //TODO: failed nodes may still be in the overlay, but we aren't looking at techniques for choosing overlay nodes yet
        }

      // We'll choose the router with the most connections to attach the server to so that we
      // minimize the chance that it is cut off from the rest of the network.
      // We also assume that the server is outside of the disaster region (could be several).
      if ((*node)->GetNDevices () > serverNode->GetNDevices ()
          and disasterNodes[(*node)->GetId ()] != 0)
        serverNode = *node;
    }

  // add another node to the router picked as the server node to be the actual server
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  Ptr<Node> serverProvider = serverNode;
  serverNode = CreateObject<Node> ();
  stack.Install (serverNode);
  NetDeviceContainer serverAndProviderDevs = pointToPoint.Install (serverNode,serverProvider);
  Ipv4Address serverAddress = address.Assign (serverAndProviderDevs).Get (0).first->GetAddress (1,0).GetLocal ();

  NS_LOG_INFO ("Done network setup!");

  //Application
  RonServerHelper ronServer (9);

  ApplicationContainer serverApps = ronServer.Install (serverNode);
  //ApplicationContainer serverApps = ronServer.Install (router_interfaces.Get (0).first->GetNetDevice (0)->GetNode ());
  //ApplicationContainer serverApps = ronServer.Install (ronServerNode);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //Ipv4Address echo_addr = router_interfaces.Get (0).first->GetAddress (2,0).GetLocal ();
  //Ipv4Address echo_addr = routers.Get (0)->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
  
  RonClientHelper ronClient (serverAddress, 9, 10);
  ronClient.SetAttribute ("MaxPackets", UintegerValue (1));
  ronClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  ronClient.SetAttribute ("PacketSize", UintegerValue (1024));
  //ronClient.SetAttribute ("OverlayPort", UintegerValue (9));
  
  ApplicationContainer clientApps;
  for (NodeContainer::Iterator node = overlayNodes.Begin ();
       node != overlayNodes.End (); node++)
    {
      if (report_disaster && disasterNodes[(*node)->GetId ()] != 0)
          ronClient.SetAttribute ("MaxPackets", UintegerValue (0));
      else
          ronClient.SetAttribute ("MaxPackets", UintegerValue (1));
      clientApps.Add (ronClient.Install (*node));
    }

  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  if (tracing)
    {
      for (ApplicationContainer::Iterator itr = clientApps.Begin ();
           itr != clientApps.End (); itr++)
        {
          if (trace_acks)
            (*itr)->TraceConnectWithoutContext ("Ack", MakeBoundCallback (&AckReceived, outputStream));
          if (trace_forwards)
            (*itr)->TraceConnectWithoutContext ("Forward", MakeBoundCallback (&PacketForwarded, outputStream));
          if (trace_sends)
            (*itr)->TraceConnectWithoutContext ("Send", MakeBoundCallback (&PacketSent, outputStream));
        }
    }

  NS_LOG_INFO ("Populating routing tables; please be patient it takes a while...");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Fail the links that were chosen
  for (Ipv4InterfaceContainer::Iterator iface = ifacesToKill.Begin ();
       iface != ifacesToKill.End (); iface++)
    {
      FailIpv4 (iface->first, iface->second);
    }

  // Fail the nodes that were chosen
  for (NodeContainer::Iterator node = failNodes.Begin ();
       node != failNodes.End (); node++)
    {
      FailNode (*node);
    }

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
