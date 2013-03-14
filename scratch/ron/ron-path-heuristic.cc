/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
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

#include "ron-path-heuristic.h"
//#include "boost/bind.hpp"
#include <cmath>
#include <numeric> //for accumulate

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RonPathHeuristic");
NS_OBJECT_ENSURE_REGISTERED (RonPathHeuristic);

RonPathHeuristic::RonPathHeuristic ()
{
  m_masterLikelihoods = NULL;
}

TypeId
RonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonPathHeuristic")
    .SetParent<Object> ()
    .AddConstructor<RonPathHeuristic> ()
    .AddAttribute ("Weight", "Value by which the likelihood this heuristic assigns is weighted.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&RonPathHeuristic::m_weight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UpdatedOnce", "True if heuristic has already made one pass to update the likelihoods and does not need to do so again.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RonPathHeuristic::m_updatedOnce),
                   MakeBooleanChecker ())
    /*.AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("base"),
                   MakeStringAccessor (&RonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("base"),
                   MakeStringAccessor (&RonPathHeuristic::m_shortName),
                   MakeStringChecker ())*/
  ;
  return tid;
}

RonPathHeuristic::~RonPathHeuristic ()
{
  if (m_masterLikelihoods != NULL and m_topLevel)
    delete m_masterLikelihoods;
} //required to keep gcc from complaining


void
RonPathHeuristic::MakeTopLevel ()
{
  NS_ASSERT_MSG (m_topLevel == NULL or m_topLevel == this,
                 "You can't make an already-aggregated heuristic into a top-level one!");
  m_topLevel = this;
  if (m_masterLikelihoods == NULL)
    m_masterLikelihoods = new MasterPathLikelihoodTable ();
}


void
RonPathHeuristic::AddPath (Ptr<RonPath> path)
{
  PeerDestination dest = path->GetDestination ();
  // If we are not top-level and the top-level has not been notified of this path,
  // we need to start at the top and work our way down, notifying along the way.
  if ((m_topLevel != (RonPathHeuristic*)this) and
      (m_masterLikelihoods->at (dest)[*path].size () == 0))
    m_topLevel->AddPath (path);

  // We need to add a new likelihood value to the Likelihood object in the master table,
  // then add the reference to this value to our table.
  else
    {
      MasterPathLikelihoodInnerTable tab = m_masterLikelihoods->at (dest);
      Likelihood lh = tab[*path];
      lh.push_back (0.0);
      double * newLh = &(lh.back ());
      m_likelihoods[dest][*path] = newLh;

      // Then recurse to the lower-level heuristics
      for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
           others != m_aggregateHeuristics.end (); others++)
        (*others)->AddPath (path);
    }
}


void
RonPathHeuristic::BuildPaths (PeerDestination destination)
{
  DoBuildPaths (destination);
  for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->BuildPaths (destination);
}


void
RonPathHeuristic::DoBuildPaths (PeerDestination destination)
{
  // All aggregate heuristics should know about the peer table, so we only need
  // to check if any one heuristic has built paths once, i.e. if the table contains anything.
  PathLikelihoodInnerTable table = m_likelihoods[destination];
  if (table.size ())
    return;
  
  for (RonPeerTable::Iterator peerItr = m_peers->Begin ();
       peerItr != m_peers->End (); peerItr++)
    {
      //make sure not to have loops!
      if ((*peerItr) == destination)
        continue;
      Ptr<RonPath> path = Create<RonPath> ();
      path->AddPeer (*peerItr);
      path->SetDestination (destination);
      AddPath (path);
    }
}


double
RonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  return 1.0;
}


void
RonPathHeuristic::UpdateLikelihoods (const PeerDestination destination)
{
  //update us first, then the other aggregate heuristics (if any)
  DoUpdateLikelihoods (destination);
  for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->UpdateLikelihoods (destination);
}


void
RonPathHeuristic::DoUpdateLikelihoods (const PeerDestination destination)
{
  if (!m_updatedOnce)
    {
      for (PathLikelihoodInnerTable::iterator path = m_likelihoods.at (*destination).begin ();
           path != m_likelihoods[*destination].end (); path++)
        SetLikelihood (path->first, GetLikelihood (path->first));
      m_updatedOnce = true;
    }
}


Ptr<RonPath>
RonPathHeuristic::GetBestPath (PeerDestination destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  NS_ASSERT_MSG (destination, "You must specify a valid server to use the heuristic!");
  NS_ASSERT_MSG (m_masterLikelihoods, "Master likelihood table missing!  " \
                 "Make sure you aggregate heuristics to the top-level one first (before lower-levels) " \
                 "so that the shared data structures are properly organized.");

  //TODO: multiple servers

  // ensure paths built
  BuildPaths (destination);

  //find the path with highest likelihood
  //TODO: cache up to MaxAttempts of them
  //TODO: check for valid size
  if (m_likelihoods[destination].size () == 0)
    throw NoValidPeerException();

  MasterPathLikelihoodInnerTable::iterator probs = (*m_masterLikelihoods)[destination].begin ();
  Ptr<RonPath> bestPath = probs->first;
  double bestLikelihood = std::accumulate (probs->second.begin (), probs->second.end (), 0);
  for (; probs != (*m_masterLikelihoods)[destination].end (); probs++)
    {
      double nextLh = std::accumulate (probs->second.begin (), probs->second.end (), 0);
      if (nextLh > bestLikelihood)
        {
          bestPath = probs->first;
          bestLikelihood = nextLh;
        }
    }
  m_pathsAttempted.insert (bestPath);
  
  if (bestLikelihood == 0.0)
    throw NoValidPeerException();

  return (bestPath);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  notifications / updates  /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::NotifyAck (Ptr<RonPath> path, Time time)
{
  DoNotifyAck (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics.begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      (*heuristic)->NotifyAck (path, time);
    }
}


void
RonPathHeuristic::DoNotifyAck (Ptr<RonPath> path, Time time)
{
  for (RonPath::Iterator peer = path->Begin ();
       peer != path->End (); peer++)
    {
      SetLikelihood (peer, 1);
    }
  //TODO: record partial path ACKs
  //TODO: record parents (Markov chain)
}


void
RonPathHeuristic::NotifyTimeout (Ptr<RonPath> path, Time time)
{
  DoNotifyTimeout (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics.begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      heuristic.NotifyTimeout (path, time);
    }
}


void
RonPathHeuristic::DoNotifyTimeout (Ptr<RonPath> path, Time time)
{
  //TODO: handle partial path ACKs
  SetLikelihood (path, 0);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  mutators / accessors  ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::AddHeuristic (Ptr<RonPathHeuristic> other)
{
  NS_ASSERT_MSG (this != other, "What are you doing? You can't aggregate a heuristic to itself...");

  other->m_topLevel = m_topLevel;
  other->m_masterLikelihoods = m_topLevel->m_masterLikelihoods;
  m_aggregateHeuristics.push_back (other);

  //give new heuristic likelihoods for current paths
  for (MasterPathLikelihoodTable::iterator tables = m_masterLikelihoods.begin ();
       tables != m_masterLikelihoods.end (); tables++)
    {
      PathLikelihoodInnerTable newTable = other->m_likelihoods[tables->first];
      for (MasterPathLikelihoodInnerTable::iterator pathLh = tables->second.begin ();
           pathLh != tables->second.end (); pathLh++)
        {
          RonPath path = pathLh->first;
          Likelihood lh = pathLh->second;
          lh.push_back (0.0);
          double * newLh = &(lh.back ());
          newTable[*path] = newLh;
        }
    }
}


void
RonPathHeuristic::SetPeerTable (Ptr<RonPeerTable> table)
{
  m_peers = table;
}


void
RonPathHeuristic::SetSourcePeer (Ptr<RonPeerEntry> peer)
{
  m_source = peer;
}


void
RonPathHeuristic::SetLikelihood (Ptr<RonPath> path, double lh)
{
  PeerDestination dest = path->GetDestination ();
  //NS_ASSERT (0.0 <= lh and lh <= 1.0);
  NS_LOG_LOGIC ("Path " << path << " has LH " << lh);
  //m_peers.SetLikelihood (peer, 
  *(m_likelihoods[dest][*path]) = lh*m_weight;
}


Ptr<RonPeerEntry>
RonPathHeuristic::GetSourcePeer ()
{
  return m_source;
}


// boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)>
// //void *
// RonPathHeuristic::GetPeerComparator (Ptr<RonPeerEntry> destination /* = NULL*/)
// {
//   return boost::bind (&RonPathHeuristic::ComparePeers, this, destination, _1, _2);
// }


//////////////////// helper functions ////////////////////


bool
RonPathHeuristic::SameRegion (RonPeerEntry peer1, RonPeerEntry peer2)
{
  Vector3D loc1 = peer1.location;
  Vector3D loc2 = peer2.location;
  return loc1.x == loc2.x and loc1.y == loc2.y;
  //TODO: actually define regions to be more than a coordinate
  /* test this later
     return peer1->GetObject<RonPeerEntry> ()->region == peer2->GetObject<RonPeerEntry> ()->region; */
}
