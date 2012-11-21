/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** An experiment of network failure scenarios during disasters.  Randomly fails the given links/nodes
    within the given region, running a RonClient on each node, who contacts the chosen RonServer
    within the disaster region.  Can run this class multiple times, once for each set of parameters.  **/

#ifndef GEOCRON_EXPERIMENT_H
#define GEOCRON_EXPERIMENT_H

#include "ns3/core-module.h"
//#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/nix-vector-routing-module.h"
#include "ns3/topology-read-module.h"
#include "ns3/ipv4-address-generator.h" //necessary?
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"

/* REMOVE THESE INCLUDES WHEN MOVED OUT OF SCRATCH DIRECTORY!!!!!!!!!!! */
#include "ron-header.h"
#include "ron-helper.h"
#include "ron-client.h"
#include "ron-server.h"

#include <iostream>
#include <sstream>
#include <map>

#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"

namespace ns3 {

class GeocronExperiment {
public:
  GeocronExperiment ();

  void ReadTopology (std::string topologyFile);
  void ReadLatencyFile (std::string latencyFile);
  void Run ();
  /*void SetHeuristic (int newHeuristic);
  void SetDisasterLocation (std::string newLocation);
  void SetFailureProbability (double newProb);*/

  void SetTimeout (Time newTimeout);
  void SetTraceFile (std::string newTraceFile);
  
  void NextHeuristic ();
  void NextDisasterLocation ();
  void NextFailureProbability ();
  std::vector<int> * heuristics;
  std::vector<std::string> * disasterLocations;
  std::vector<double> * failureProbabilities;
  //TODO: timeouts, max retries, etc.
  uint8_t minNDevs;

  uint8_t contactAttempts;

private:
  bool IsDisasterNode (Ptr<Node> node);

  std::string currHeuristic;
  std::string currLocation;
  double currFprob;
  Time timeout;

  NodeContainer nodes;
  std::map<std::string,std::string> latencies;
  std::string topologyFile;

  std::string traceFile;
  Time appStopTime;

  // These maps hold nodes and ifaces of interest for the associated disaster region
  std::map<std::string, Ipv4InterfaceContainer> potentialIfacesToKill; //are the nested containers created automatically?
  std::map<std::string, std::map<uint32_t, Ptr <Node> > > disasterNodes;
};
} //namespace ns3
#endif //GEOCRON_EXPERIMENT_H
