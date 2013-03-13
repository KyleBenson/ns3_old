/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** An experiment of network failure scenarios during disasters.  Randomly fails the given links/nodes
    within the given region, running a RonClient on each node, who contacts the chosen RonServer
    within the disaster region.  Can run this class multiple times, once for each set of parameters.  **/

#include "geocron-experiment.h"
#include "ron-trace-functions.h"
#include "failure-helper-functions.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/heap/priority_queue.hpp>
 
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GeocronExperiment");
  //NS_OBJECT_ENSURE_REGISTERED (GeocronExperiment);

GeocronExperiment::GeocronExperiment () : NO_LOCATION_VECTOR (0.0, 0.0, -1.0)
{
  appStopTime = Time (Seconds (30.0));
  simulationLength = Seconds (10.0);
  overlayPeers = Create<RonPeerTable> ();

  maxNDevs = 1;
  //cmd.AddValue ("install_stubs", "If not 0, install RON client only on stub nodes (have <= specified links - 1 (for loopback dev))", maxNDevs);

  currLocation = "";
  currFprob = 0.0;
  currRun = 0;
  contactAttempts = 10;
  traceFile = "";
  nruns = 1;
  nServerChoices = 10;
}


void
GeocronExperiment::SetTimeout (Time newTimeout)
{
  timeout = newTimeout;
}


void
GeocronExperiment::NextHeuristic ()
{
  //heuristicIdx++;
}

void
GeocronExperiment::NextDisasterLocation ()
{
  return;
  //locationIdx++;
}


void
GeocronExperiment::NextFailureProbability ()
{
  return;
  //fpropIdx++;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////   Helper Functions     ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool
GeocronExperiment::IsDisasterNode (Ptr<Node> node)
{
  return disasterNodes[currLocation].count ((node)->GetId ());
}


void
GeocronExperiment::ReadLatencyFile (std::string latencyFile)
{
  // Load latency information if requested
  if (latencyFile != "")
    {
      if (! boost::filesystem::exists(latencyFile))
        {
          NS_LOG_ERROR("File does not exist: " + latencyFile);
          exit(-1);
        }
      latencies = RocketfuelTopologyReader::ReadLatencies (latencyFile);
    }
}


void
GeocronExperiment::ReadLocationFile (std::string locationFile)
{
  if (locationFile != "")
    {
      if (! boost::filesystem::exists (locationFile))
        {
          NS_LOG_ERROR("File does not exist: " + locationFile);
          exit(-1);
        }

      std::ifstream infile (locationFile.c_str ());
      std::string loc, line;
      double lat = 0.0, lon = 0.0;
      std::vector<std::string> parts; //for splitting up line

      while (std::getline (infile, line))
        {
          boost::split (parts, line, boost::is_any_of ("\t"));
          loc = parts[0];
          lat = boost::lexical_cast<double> (parts[1]);
          lon = boost::lexical_cast<double> (parts[2]);

          //std::cout << "Loc=" << loc << ", lat=" << lat << ", lon=" << lon << std::endl;
          locations[Location(loc)] = Vector (lat, lon, 1.0); //z position of 1 means we do have a location
        }
    }
}


/** Read network topology, store nodes and devices. */
void
GeocronExperiment::ReadTopology (std::string topologyFile)
{
  this->topologyFile = topologyFile;

  if (! boost::filesystem::exists(topologyFile)) {
    NS_LOG_ERROR("File does not exist: " + topologyFile);
    exit(-1);
  }
  
  RocketfuelTopologyReader topo_reader;
  topo_reader.SetFileName(topologyFile);
  nodes = topo_reader.Read();
  NS_LOG_INFO ("Nodes read from file: " + boost::lexical_cast<std::string> (nodes.GetN()));

  NS_LOG_INFO ("Assigning addresses and installing interfaces...");

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // NixHelper to install nix-vector routing
  Ipv4NixVectorHelper nixRouting;
  nixRouting.SetAttribute("FollowDownEdges", BooleanValue (true));
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper routingList;
  routingList.Add (staticRouting, 0);
  routingList.Add (nixRouting, 10);
  InternetStackHelper stack;
  stack.SetRoutingHelper (routingList); // has effect on the next Install ()
  stack.Install (nodes);

  //For each link in topology, add a connection between nodes and assign IP
  //addresses to the new network they've created between themselves.
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.252");
  //NetDeviceContainer router_devices;
  //Ipv4InterfaceContainer router_interfaces;

  NS_LOG_INFO ("Generating links and checking failure model.");

  for (TopologyReader::ConstLinksIterator iter = topo_reader.LinksBegin();
       iter != topo_reader.LinksEnd(); iter++) {
    Location fromLocation = iter->GetAttribute ("From Location");
    Location toLocation = iter->GetAttribute ("To Location");

    // Set latency for this link if we loaded that information
    if (latencies.size())
      {
        LatenciesMap::iterator latency = latencies.find (fromLocation + " -> " + toLocation);
        //if (iter->GetAttributeFailSafe ("Latency", latency))
        
        if (latency != latencies.end())
          {
            pointToPoint.SetChannelAttribute ("Delay", StringValue (latency->second + "ms"));
          }
        else
          {
            latency = latencies.find (toLocation + " -> " + fromLocation);
            if (latency != latencies.end())
              pointToPoint.SetChannelAttribute ("Delay", StringValue (latency->second + "ms"));
            else
              pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
          }
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
    //router_devices.Add(new_devs);

    Ipv4InterfaceContainer new_interfaces = address.Assign (new_devs);
    //router_interfaces.Add(new_interfaces);
    address.NewNetwork();
    // Mobility model to set positions for geographically-correlated information
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator> ();
    Vector fromPosition = locations.count (fromLocation)  ? (locations.find (fromLocation))->second : NO_LOCATION_VECTOR;
    Vector toPosition = locations.count (toLocation) ? (locations.find (toLocation))->second : NO_LOCATION_VECTOR;
    positionAllocator->Add (fromPosition);
    positionAllocator->Add (toPosition);
    mobility.SetPositionAllocator (positionAllocator);
    mobility.Install (both_nodes);

    //aggregate RonPeerEntries to Nodes if we haven't already
    Ptr<RonPeerEntry> fromPeer = from_node->GetObject<RonPeerEntry> ();
    if (!fromPeer)
      {
        fromPeer = CreateObject<RonPeerEntry> (from_node);
        fromPeer->region = fromLocation;
        from_node->AggregateObject (fromPeer);
      }
    Ptr<RonPeerEntry> toPeer = to_node->GetObject<RonPeerEntry> ();
    if (!toPeer)
      {
        toPeer = CreateObject<RonPeerEntry> (to_node);
        toPeer->region = toLocation;
        to_node->AggregateObject (toPeer);
      }
    
    NS_LOG_DEBUG ("Link from " << fromLocation << "[" << fromPosition << "] to " << toLocation<< "[" << toPosition << "]");

    // If the node is in a disaster region, add it to the corresponding list
    // Also add potential servers (randomly chosen from node's with well-defined locations that lie outside that disaster region)
    for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
         disasterLocation != disasterLocations->end (); disasterLocation++)
      {
        if (fromLocation == *disasterLocation)
          {
            disasterNodes[*disasterLocation].insert(std::pair<uint32_t, Ptr<Node> > (from_node->GetId (), from_node));
          }
 
        if (toLocation == *disasterLocation)
          {
            disasterNodes[*disasterLocation].insert(std::pair<uint32_t, Ptr<Node> > (to_node->GetId (), to_node));
          }

        // Failure model: if either endpoint is in the disaster location,
        // add it to the list of potential ifaces to kill
        if (iter->GetAttribute ("From Location") == *disasterLocation ||
            iter->GetAttribute ("To Location") == *disasterLocation)
          //random.GetValue () <= currFprob)
          {
            potentialIfacesToKill[*disasterLocation].Add(new_interfaces.Get (0));
            potentialIfacesToKill[*disasterLocation].Add(new_interfaces.Get (1));
          }

        // Used for debugging to make sure locations are being found properly
        #ifdef NS3_LOG_ENABLE
        if (!HasLocation (to_node)){
          NS_LOG_DEBUG ("Node " << to_node->GetId () << " has no position at location " << toLocation);}
        if (!HasLocation (from_node)){
          NS_LOG_DEBUG ("Node " << from_node->GetId () << " has no position at location " << fromLocation);}
        #endif
      }
  } //end link iteration


  ////////////////////////////////////////////////////////////////////////////////


  NS_LOG_INFO ("Topology finished.  Choosing & installing clients.");

  // We want to narrow down the number of server choices by picking about nServerChoices of the highest-degree
  // nodes.  We want at least nServerChoices of them, but will allow more so as not to include some of degree d
  // but exclude others of degree d.
  //
  // So we keep an ordered list of nodes and keep track of the sums
  //
  // invariant: if the group of smallest degree nodes were removed from the list,
  // its size would be < nServerChoices
  typedef std::pair<uint32_t/*degree*/, Ptr<Node> > DegreeType;
  typedef boost::heap::priority_queue<DegreeType, boost::heap::compare<std::greater<DegreeType> > > DegreePriorityQueue;
  DegreePriorityQueue potentialServerNodeCandidates;
  uint32_t nLowestDegreeNodes = 0, lowDegree = 0;

  NodeContainer overlayNodes;
  for (NodeContainer::Iterator node = nodes.Begin ();
       node != nodes.End (); node++)
    {
      uint32_t degree = GetNodeDegree (*node);
      // Sanity check that a node has some actual links, otherwise remove it from the simulation
      // this happened with some disconnected Rocketfuel models and made null pointers
      if (degree <= 0)
        {
          NS_LOG_LOGIC ("Node " << (*node)->GetId () << " has no links!");
          //node.erase ();
          disasterNodes[currLocation].erase ((*node)->GetId ());
          continue;
        }
      
      // We may only install the overlay application on clients attached to stub networks,
      // so we just choose the stub network nodes here
      // (note that all nodes have a loopback device)
      if (!maxNDevs or degree <= maxNDevs) 
        {
          overlayNodes.Add (*node);
          overlayPeers->AddPeer (*node);
        }

      else if (degree > maxNDevs and HasLocation (*node))
        {
          //add potential server
          if (degree >= lowDegree or potentialServerNodeCandidates.size () < nServerChoices)
            potentialServerNodeCandidates.push (std::pair<uint32_t, Ptr<Node> > (degree, *node));
          else
            continue;

          //base condition to set lowDegree the 1st time
          if (!lowDegree)
            lowDegree = degree;

          if (degree == lowDegree)
            nLowestDegreeNodes++;
          else if (degree < lowDegree)
            {
              lowDegree = degree;
              nLowestDegreeNodes = 1;
            }

          //remove group of lowest degree nodes if it's okay to do so
          if (potentialServerNodeCandidates.size () - nLowestDegreeNodes >= nServerChoices)
            {
              #ifdef NS3_LOG_ENABLE
              uint32_t nThrownAway = 0;
              #endif
              while (potentialServerNodeCandidates.top ().first == lowDegree)
                {
                  potentialServerNodeCandidates.pop ();
                  #ifdef NS3_LOG_ENABLE
                  nThrownAway++;
                  #endif
                }
              NS_LOG_DEBUG ("Throwing away " << nThrownAway << " vertices of degree " << lowDegree);

              // recalculate nLowestDegreeNodes
              lowDegree = GetNodeDegree (potentialServerNodeCandidates.top ().second);
              nLowestDegreeNodes = 0;
              for (DegreePriorityQueue::iterator itr = potentialServerNodeCandidates.begin ();
                   itr != potentialServerNodeCandidates.end () and (itr)->first == lowDegree; itr++)
                nLowestDegreeNodes++;
              NS_LOG_DEBUG (nLowestDegreeNodes << " nodes have new lowest degree = " << lowDegree);
            }
        }
    }
  
  RonClientHelper ronClient (9);
  ronClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  ronClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ronClient.SetAttribute ("Timeout", TimeValue (timeout));
  
  // Install client app and set number of server contacts they make based on whether all nodes
  // should report the disaster or only the ones in the disaster region.
  for (NodeContainer::Iterator node = overlayNodes.Begin ();
       node != overlayNodes.End (); node++)
    {
      ApplicationContainer newApp = ronClient.Install (*node);
      clientApps.Add (newApp);
      Ptr<RonClient> newClient = DynamicCast<RonClient> (newApp.Get (0));
      newClient->SetAttribute ("MaxPackets", UintegerValue (0)); //explicitly enable sensor reporting when disaster area set
      newClient->SetPeerTable (overlayPeers); //TODO: make this part of the helper?? or some p2p overlay algorithm? or the table itself? give out a callback function with that nodes peers?
    }
  
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (appStopTime);

  // Now cache the actual choices of server nodes for each disaster region
  NS_LOG_DEBUG (potentialServerNodeCandidates.size () << " total potentialServerNodeCandidates.");
  for (DegreePriorityQueue::iterator itr = potentialServerNodeCandidates.begin ();
       itr != potentialServerNodeCandidates.end (); itr++)
    {
      NS_LOG_DEBUG ("Node " << itr->second->GetId () << " has degree " << GetNodeDegree (itr->second));
      Ptr<Node> serverCandidate = itr->second;
      Location loc = serverCandidate->GetObject<RonPeerEntry> ()->region;

      for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
           disasterLocation != disasterLocations->end (); disasterLocation++)
        {
          if (*disasterLocation != loc)
            serverNodeCandidates[*disasterLocation].Add (serverCandidate);
        }
    }
}


void
GeocronExperiment::SetDisasterLocation (Location newDisasterLocation)
{
  currLocation = newDisasterLocation;
}


void
GeocronExperiment::SetFailureProbability (double newFailureProbability)
{
  currFprob = newFailureProbability;
}


/*void
GeocronExperiment::SetHeuristic (newHeuristic)
{

}*/



void
GeocronExperiment::SetTraceFile (std::string newTraceFile)
{
  NS_LOG_INFO ("New trace file is: " + newTraceFile);

  traceFile = newTraceFile;
}


void
GeocronExperiment::AutoSetTraceFile ()
{
  boost::filesystem::path newTraceFile ("ron_output");
  newTraceFile /= boost::filesystem::path(topologyFile).stem ();
  newTraceFile /= boost::algorithm::replace_all_copy (currLocation, " ", "_");

  // round the fprob to 1 decimal place
  std::ostringstream fprob;
  std::setprecision (1);
  fprob << currFprob;
  newTraceFile /= fprob.str ();
  //newTraceFile /= boost::lexical_cast<std::string> (currFprob);

  // extract unique filename from heuristic to summarize parameters, aggregations, etc.
  TypeId::AttributeInformation info;
  NS_ASSERT (currHeuristic->GetTypeId ().LookupAttributeByName ("SummaryName", &info));
  StringValue summary;
  info.checker->Check (summary);
  newTraceFile /= summary.Get ();

  // set output number if start_run_number is specified
  uint32_t outnum = currRun;
  if (start_run_number and nruns > 1)
    outnum = currRun + start_run_number;
  
  std::string fname = "run";
  fname += boost::lexical_cast<std::string> (outnum);
  newTraceFile /= (fname);
  newTraceFile.replace_extension(".out");

  // Change name to avoid overwriting
  uint32_t copy = 0;
  while (boost::filesystem::exists (newTraceFile))
    {
      newTraceFile.replace_extension (".out(" + boost::lexical_cast<std::string> (copy++) + ")");
    }

  boost::filesystem::create_directories (newTraceFile.parent_path ());

  SetTraceFile (newTraceFile.string ());
}


/** Run through all the possible combinations of disaster locations, failure probabilities,
    heuristics, and other parameters for the given number of runs. */
void
GeocronExperiment::RunAllScenarios ()
{
  SeedManager::SetSeed(std::time (NULL));
  int runSeed = 0;
  for (std::vector<Location>::iterator disasterLocation = disasterLocations->begin ();
       disasterLocation != disasterLocations->end (); disasterLocation++)
    {
      SetDisasterLocation (*disasterLocation); 
      for (std::vector<double>::iterator fprob = failureProbabilities->begin ();
           fprob != failureProbabilities->end (); fprob++)
        {
          SetFailureProbability (*fprob);
          for (currRun = 0; currRun < nruns; currRun++)
            {
              // We want to compare each heuristic to each other for each configuration of failures
              ApplyFailureModel ();
              SetNextServers ();
              for (uint32_t h = 0; h < heuristics->size (); h++)
                {
                  currHeuristic = heuristics->at (h);
                  SeedManager::SetRun(runSeed++);
                  AutoSetTraceFile ();
                  Run ();
                }
              UnapplyFailureModel ();
            }
        }
    }
}


/** Connect the defined traces to the specified client applications. */
void
GeocronExperiment::ConnectAppTraces ()
{
  //need to prevent stupid waf from throwing errors for unused definitions...
  (void) PacketForwarded;
  (void) PacketSent;
  (void) AckReceived;

  if (traceFile != "")
    {
      Ptr<OutputStreamWrapper> traceOutputStream;
      AsciiTraceHelper asciiTraceHelper;
      traceOutputStream = asciiTraceHelper.CreateFileStream (traceFile);
  
      for (ApplicationContainer::Iterator itr = clientApps.Begin ();
           itr != clientApps.End (); itr++)
        {
          Ptr<RonClient> app = DynamicCast<RonClient> (*itr);
          app->ConnectTraces (traceOutputStream);
        }
    }
}


//////////////////////////////////////////////////////////////////////
/////************************************************************/////
////////////////        APPLY FAILURE MODEL         //////////////////
/////************************************************************/////
//////////////////////////////////////////////////////////////////////
/**   Fail the links that were chosen with given probability */
void
GeocronExperiment::ApplyFailureModel () {
  NS_LOG_INFO ("Applying failure model.");

  // Keep track of these and unfail them later
  failNodes = NodeContainer ();
  for (std::map<uint32_t, Ptr <Node> >::iterator nodeItr = disasterNodes[currLocation].begin ();
       nodeItr != disasterNodes[currLocation].end (); nodeItr++)
    {
      Ptr<Node> node = nodeItr->second;
      // Fail nodes within the disaster region with some probability
      if (random.GetValue () < currFprob)
        {
          failNodes.Add (node);
          NS_LOG_LOGIC ("Node " << (node)->GetId () << " will fail.");
        }
    }
  for (NodeContainer::Iterator node = failNodes.Begin ();
       node != failNodes.End (); node++)
    {
      FailNode (*node);
    }

  // Same with these
  ifacesToKill = Ipv4InterfaceContainer ();
  for (Ipv4InterfaceContainer::Iterator iface = potentialIfacesToKill[currLocation].Begin ();
       iface != potentialIfacesToKill[currLocation].End (); iface++)
    {
      if (random.GetValue () < currFprob)
        ifacesToKill.Add (*iface);
    }
  for (Ipv4InterfaceContainer::Iterator iface = ifacesToKill.Begin ();
       iface != ifacesToKill.End (); iface++)
    {
        FailIpv4 (iface->first, iface->second);
    }
}


void
GeocronExperiment::UnapplyFailureModel () {
  // Unfail the links that were chosen
  for (Ipv4InterfaceContainer::Iterator iface = ifacesToKill.Begin ();
       iface != ifacesToKill.End (); iface++)
    {
      UnfailIpv4 (iface->first, iface->second);
    }

  // Unfail the nodes that were chosen
  for (NodeContainer::Iterator node = failNodes.Begin ();
       node != failNodes.End (); node++)
    {
      UnfailNode (*node, appStopTime);
    }


  clientApps.Start (Seconds (2.0));
  clientApps.Stop (appStopTime);
}


void
GeocronExperiment::SetNextServers () {
  NS_LOG_LOGIC ("Choosing from " << serverNodeCandidates[currLocation].GetN () << " server provider candidates.");

  Ptr<Node> serverNode = (serverNodeCandidates[currLocation].GetN () ?
                          serverNodeCandidates[currLocation].Get (random.GetInteger (0, serverNodeCandidates[currLocation].GetN () - 1)) :
                          nodes.Get (random.GetInteger (0, serverNodeCandidates[currLocation].GetN () - 1)));

  //Application
  RonServerHelper ronServer (9);

  ApplicationContainer serverApps = ronServer.Install (serverNode);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (appStopTime);

  serverPeers = Create<RonPeerTable> ();
  serverPeers->AddPeer (serverNode);
}


void
GeocronExperiment::Run ()
{
  int numDisasterPeers = 0;
  // Set some parameters for this run
  for (std::map<uint32_t, Ptr <Node> >::iterator nodeItr = disasterNodes[currLocation].begin ();
       nodeItr != disasterNodes[currLocation].end (); nodeItr++)
    /*  for (ApplicationContainer::Iterator app = disasterNodes[currLocation].Begin ();
        app != clientApps.End (); app++)*/
    {
      for (uint32_t i = 0; i < nodeItr->second->GetNApplications (); i++) {
        Ptr<RonClient> ronClient = DynamicCast<RonClient> (nodeItr->second->GetApplication (0));
        if (ronClient == NULL)
          continue;
        //TODO: different heuristics
        Ptr<RonPathHeuristic> heuristic = currHeuristic->Create<RonPathHeuristic> ();
        // Must set heuristic first so that source will be set and heuristic can make its heap
        ronClient->SetHeuristic (heuristic);
        ronClient->SetServerPeerTable (serverPeers);
        heuristic->SetPeerTable (overlayPeers);
        ronClient->SetAttribute ("MaxPackets", UintegerValue (contactAttempts));
        numDisasterPeers++;
      }
    }

  //NS_LOG_INFO ("Populating routing tables; please be patient it takes a while...");
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //NS_LOG_INFO ("Done populating routing tables!");
  
  ConnectAppTraces ();

  // pointToPoint.EnablePcap("rocketfuel-example",router_devices.Get(0),true);

  NS_LOG_UNCOND ("Starting simulation on map file " << topologyFile << ": " << std::endl
                 << nodes.GetN () << " total nodes" << std::endl
                 << overlayPeers->GetN () << " total overlay nodes" << std::endl
                 << disasterNodes[currLocation].size () << " nodes in " << currLocation << " total" << std::endl
                 << numDisasterPeers << " overlay nodes in " << currLocation << std::endl //TODO: get size from table
                 << std::endl << "Failure probability: " << currFprob << std::endl
                 << failNodes.GetN () << " nodes failed" << std::endl
                 << ifacesToKill.GetN () / 2 << " links failed");

  Simulator::Stop (simulationLength);
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("Next simulation run...");

  //serverNode->GetObject<Ipv4NixVectorRouting> ()->FlushGlobalNixRoutingCache ();
  
  // reset apps
  for (ApplicationContainer::Iterator app = clientApps.Begin ();
       app != clientApps.End (); app++)
    {
      Ptr<RonClient> ronClient = DynamicCast<RonClient> (*app);
      ronClient->Reset ();
    }
}

//////////////////////////////////////////////////////////////////////
//////////////////// Helper functions ////////////////////////////////
//////////////////////////////////////////////////////////////////////

Vector
GeocronExperiment::GetLocation (Ptr<Node> node)
{
  return node->GetObject<MobilityModel> ()->GetPosition ();
}


bool
GeocronExperiment::HasLocation (Ptr<Node> node)
{
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
  if (mob == NULL)
    {
      NS_LOG_UNCOND ("Node " << node->GetId () << " has no MobilityModel!");
      return false;
    }
  if (mob->GetPosition () == NO_LOCATION_VECTOR)
    {
      return false;
    }
  return true;
}

namespace ns3 {
//global helper functions must be in ns3 namespace
Ipv4Address GetNodeAddress(Ptr<Node> node)
{
  return ((Ipv4Address)(node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()));
  // for interfaces: //ronServer.Install (router_interfaces.Get (0).first->GetNetDevice (0)->GetNode ());
}

uint32_t GetNodeDegree(Ptr<Node> node)
{
  return node->GetNDevices() - 1; //assumes PPP links
}
}//ns3
    /* Was used in a (failed) attempt to mimic the actual addresses from the file to exploit topology information...


  uint32_t network_mask = 0xffffff00;
  uint32_t host_mask = 0x000000ff;
  Ipv4Mask ip_mask("255.255.255.0");

    Ipv4Address from_addr = Ipv4Address(iter->GetAttribute("From Address").c_str());
    Ipv4Address to_addr = Ipv4Address(iter->GetAttribute("To Address").c_str());
    // Try to find an unused address near to this one (hopefully within the subnet)    while (Ipv4AddressGenerator::IsAllocated(from_addr) ||
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
