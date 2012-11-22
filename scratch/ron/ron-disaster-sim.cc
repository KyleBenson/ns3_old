/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** A simulator of network failure scenarios during disasters.  Builds a mesh topology of
    point-to-point connected nodes generated using Rocketfuel and fails random nodes/links
    within the specified city region. **/

//#include "ns3/ron-header.h"

/* REMOVE THESE INCLUDES WHEN MOVED OUT OF SCRATCH DIRECTORY!!!!!!!!!!! */
#include "ron-header.h"
#include "ron-helper.h"
#include "ron-client.h"
#include "ron-server.h"
#include "geocron-experiment.h"

#include <boost/tokenizer.hpp>
//#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
/*#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>*/
//#include "boost/system"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RonDisasterSimulation");

#define nslog(x) NS_LOG_UNCOND(x);

int 
main (int argc, char *argv[])
{
  GeocronExperiment exp;

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////     Arguments    //////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  int verbose = 0;
  bool trace_acks = true;
  bool trace_forwards = true;
  bool trace_sends = true;
  // Max number of devices a node can have to be in the overlay (to eliminate backbone routers)
  std::string fail_prob = "0.3";
  uint8_t install_stubs = 3; //all nodes have at least a loopback interface device!!!
  bool report_disaster = true;
  std::string heuristic = "0 1 2";
  std::string filename = "rocketfuel/maps/3356.cch";
  std::string latencyFile = "";
  std::string disaster_location = "Los Angeles, CA";
  bool tracing = false;
  double timeout = 1.0;

  CommandLine cmd;
  cmd.AddValue ("file", "File to read network topology from", filename);
  cmd.AddValue ("latencies", "File to read latencies from in Rocketfuel weights file format", latencyFile);
  cmd.AddValue ("tracing", "Whether to write traces to file", tracing);
  cmd.AddValue ("trace_acks", "Whether to print traces when a client receives an ACK from the server", trace_acks);
  cmd.AddValue ("trace_forwards", "Whether to print traces when a client forwards a packet", trace_forwards);
  cmd.AddValue ("trace_sends", "Whether to print traces when a client sends a packet initially", trace_sends);
  cmd.AddValue ("verbose", "Whether to print verbose log info (1=INFO, 2=LOGIC)", verbose);
  cmd.AddValue ("disaster", "Where the disaster(s) (and subsequent failures) is(are) to occur "
                "(use underscores for spaces so the command line parser will actually work)", disaster_location);
  cmd.AddValue ("fail_prob", "Probability(s) that a link in the disaster region will fail", fail_prob);
  cmd.AddValue ("install_stubs", "Install RON client only on stub nodes (have <= specified links)", install_stubs);
  cmd.AddValue ("report_disaster", "Only RON clients in the disaster region will report to the server", report_disaster);
  cmd.AddValue ("heuristic", "Which heuristic(s) to use when choosing intermediate overlay nodes.", heuristic);
  cmd.AddValue ("runs", "Number of times to run simulation on given inputs.", exp.nruns);
  cmd.AddValue ("timeout", "Seconds to wait for server reply before attempting contact through the overlay.", timeout);
  cmd.AddValue ("contact_attempts", "Number of times a reporting node will attempt to contact the server "
                "(it will use the overlay after the first attempt).  Default is 1 (no overlay).", exp.contactAttempts);

  cmd.Parse (argc,argv);

  // Parse string args for possible multiple arguments
  typedef boost::tokenizer<boost::char_separator<char> > 
    tokenizer;
  boost::char_separator<char> sep(" ");
  
  std::vector<double> * failureProbabilities = new std::vector<double> ();
  tokenizer tokens(fail_prob, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      failureProbabilities->push_back (boost::lexical_cast<double> (*tokIter));
    }

  std::vector<std::string> * disasterLocations = new std::vector<std::string> ();
  tokens = tokenizer(disaster_location, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      // Need to allow users to input '_'s instead of spaces so the parser won't truncate locations...
      //std::string correctedToken = boost::replace_all_copy(*tokIter, '_', ' ');
      //correctedToken = *(new std::string(*tokIter));
      std::string correctedToken = boost::algorithm::replace_all_copy ((std::string)*tokIter, "_", " ");
      disasterLocations->push_back (correctedToken);
    }

  std::vector<int> * heuristics = new std::vector<int> ();
  tokens = tokenizer(heuristic, sep);
  for (tokenizer::iterator tokIter = tokens.begin();
       tokIter != tokens.end(); ++tokIter)
    {
      heuristics->push_back (boost::lexical_cast<int> (*tokIter));
    }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////    Logging    /////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  if (verbose == 1)
    {
      LogComponentEnable ("RonTracers", LOG_LEVEL_INFO);
      LogComponentEnable ("RonClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("GeocronExperiment", LOG_LEVEL_INFO);
      LogComponentEnable ("RonServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("RonHeader", LOG_LEVEL_INFO);
      LogComponentEnable ("RonDisasterSimulation", LOG_LEVEL_INFO);
      LogComponentEnable ("Ipv4NixVectorRouting", LOG_LEVEL_INFO);
      //LogComponentEnable ("RocketfuelTopologyReader", LOG_LEVEL_INFO);
    }

  else if (verbose == 2)
    {
      LogComponentEnable ("GeocronExperiment", LOG_LEVEL_LOGIC);
      LogComponentEnable ("RonClientApplication", LOG_LEVEL_LOGIC);
      LogComponentEnable ("RonServerApplication", LOG_LEVEL_LOGIC);
      LogComponentEnable ("RonHeader", LOG_LEVEL_LOGIC);
      LogComponentEnable ("Ipv4NixVectorRouting", LOG_LEVEL_INFO);
      //LogComponentEnable ("RocketfuelTopologyReader", LOG_LEVEL_LOGIC);
    }

  else if (verbose == 3)
     {
      LogComponentEnable ("RonClientApplication", LOG_LEVEL_FUNCTION);
      LogComponentEnable ("RonServerApplication", LOG_LEVEL_FUNCTION);
      LogComponentEnable ("RonClientServerHelper", LOG_LEVEL_FUNCTION);
      LogComponentEnable ("RonHeader", LOG_LEVEL_FUNCTION);
      LogComponentEnable ("RocketfuelExample", LOG_LEVEL_FUNCTION);
      LogComponentEnable ("Ipv4NixVectorRouting", LOG_LEVEL_FUNCTION);
      //LogComponentEnable ("RocketfuelTopologyReader", LOG_LEVEL_FUNCTION);
    }

  ////////////////////////////////////////////////////////////////////////////////
  //////////       Create experiment and set parameters   ////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  exp.minNDevs = install_stubs;
  exp.heuristics = heuristics;
  exp.disasterLocations = disasterLocations;
  exp.failureProbabilities = failureProbabilities;
  exp.SetTimeout (Seconds (timeout));

  exp.ReadTopology (filename);
  exp.ReadLatencyFile (latencyFile);
  //exp.SetTraceFile (traceFile);
  exp.RunAllScenarios ();


  //in a loop, advance parameters
  

  return 0;
}
