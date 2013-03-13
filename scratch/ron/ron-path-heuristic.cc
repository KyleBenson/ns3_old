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
  if (m_masterLikelihoods != NULL)
    delete m_masterLikelihoods;
} //required to keep gcc from complaining


void
RonPathHeuristic::MakeTopLevel ()
{
  m_topLevel = true;
  if (m_masterLikelihoods == NULL)
    m_masterLikelihoods = new MasterPathLikelihoodTable ();
}


void
RandomRonPathHeuristic::UpdateLikelihoods (PeerDestination destination)
{
  //update us first, then the other aggregate heuristics (if any)
  DoUpdateLikelihoods (destination);
  for (AggregateHeuristics::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->UpdateLikelihoods (destination);
}


void
RandomRonPathHeuristic::DoUpdateLikelihoods (PeerDestination destination)
{
  if (!m_updatedOnce)    {
      for (RonPeerTable::Iterator peer = m_peers->Begin ();
           peer != m_peers->End (); peer++)
        SetLikelihood (*peer, GetLikelihood (*peer, destination));
      m_updatedOnce = true;
    }
}


Ptr<OverlayPath>
RonPathHeuristic::GetBestPath (PeerDestination destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  NS_ASSERT_MSG (destination, "You must specify a valid server to use the heuristic!");

  //TODO: multiple servers

  //if (!m_updatedOnce)
  if (m_likelihoods[destination]TODO: build paths
      virtual EnsurePathsBuilt
    

  //find the peer with highest likelihood
  //TODO: cache up to MaxAttempts of them
  //TODO: check for valid size
  LikelihoodPeerIterator probs = m_likelihoods.begin ();
  Ptr<RonPeerEntry> bestPeer = probs->first;
  double bestLikelihood = probs->second;
  for (; probs != m_likelihoods.end (); probs++)
    {
    TODO: sum up likelihoods
      if (probs->second > bestLikelihood)
        {
          bestPeer = probs->first;
          bestLikelihood = probs->second;
        }
    }
  m_peersAttempted.push_back (bestPeer);
  
  if (bestLikelihood == 0.0)
    throw NoValidPeerException();

  return Create<OverlayPath> (bestPeer);
  //TODO: implement multiple overlay peers along path
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  notifications / updates  /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::NotifyAck (Ptr<OverlayPath> path, Time time)
{
  DoNotifyAck (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics->begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      heuristic->NotifyAck (path, time);
    }
}


void
RonPathHeuristic::DoNotifyAck (Ptr<OverlayPath> path, Time time)
{
  for (OverlayPath::Iterator peer = path->Begin ();
       peer != path.End (); peer++)
    {
      SetLikelihood (*peer, 1);
    }
  //TODO: record partial path ACKs
  //TODO: record parents (Markov chain)
}


void
RonPathHeuristic::NotifyTimeout (Ptr<OverlayPath> path, Time time)
{
  DoNotifyTimeout (path, time);
  for (AggregateHeuristics::iterator heuristic = m_aggregateHeuristics->begin ();
       heuristic != m_aggregateHeuristics.end (); heuristic++)
    {
      heuristic->NotifyTimeout (path, time);
    }
}


void
RonPathHeuristic::DoNotifyTimeout (Ptr<OverlayPath> path, Time time)
{
  //TODO: handle partial path ACKs
  SetLikelihood (*path, 0);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////  mutators / accessors  ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
RonPathHeuristic::AddHeuristic (Ptr<RonPathHeuristic> other)
{
  m_aggregateHeuristics.push_back (other);

  //give new heuristic likelihoods for current paths
  for (MasterPathLikelihoodTable tables = m_masterLikelihoods.begin ();
       tables != m_masterLikelihoods.end (); tables++)
    {
      SinglePathLikelihoodInnerTable newTable = other->m_likelihoods[tables->first];
      for (PathLikelihoodTable::iterator pathLh = tables->second.begin ();
           pathLh != tables->end (); pathLh++)
        {
          OverlayPath path = pathLh->first;
          Likelihood lh = pathLh->second;
          lh.push_back (0.0);
          double * newLh = &(lh.back ());
          newTable[path] = newLh;
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
RonPathHeuristic::SetLikelihood (PeerDestination dest, Ptr<OverlayPath> path, double lh)
{
  //NS_ASSERT (0.0 <= lh and lh <= 1.0);
  NS_LOG_LOGIC ("Peer " << peer->id << " has LH " << lh);
  //m_peers.SetLikelihood (peer, 
  *(m_likelihoods[dest][path]) = lh*m_weight;
}

/*
void
RonPathHeuristic::ZeroLikelihood (Ptr<RonPeerEntry> peer)
{
  NS_ASSERT_MSG (m_likelihoods.count (peer), "Peer not present in table!");
  NS_LOG_LOGIC ("Peer " << peer->id << "'s LH cleared to 0");
  m_likelihoods[peer] = 0.0;
}
*/
   /*
void
RonPathHeuristic::ZeroLikelihoods ()
{
  //build it first time ?
  if (m_likelihoods.size () != m_peers->GetN ())
    {
      m_likelihoods.clear ();
      for (RonPeerTable::Iterator peer = m_peers->Begin ();
           peer != m_peers->End (); peer++)
        {
          probs->second = 0.0;
        }
    }
  else
    {
      LikelihoodPeerIterator probs = m_likelihoods.begin ();
      for (; probs != m_likelihoods.end (); probs++)
        {
          probs->second = 0.0;
        }
    }
}
   */

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
RonPathHeuristic::SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2)
{
  //TODO: actually define regions to be more than a coordinate
  Vector3D loc1 = peer1->location;
  Vector3D loc2 = peer2->location;
  return loc1.x == loc2.x and loc1.y == loc2.y;
  /* test this later
     return peer1->GetObject<RonPeerEntry> ()->region == peer2->GetObject<RonPeerEntry> ()->region; */
}


//////////////////////////////  OverlayPath  //////////////////////////////


OverlayPath::OverlayPath(Ptr<RonPeerEntry> peer)
{
  AddPeer (peer);
  m_reversed = false;
}


void 
OverlayPath::AddPeer (Ptr<RonPeerEntry> peer)
{
  if (m_reversed)
    m_path.push_front (peer);
  m_path.push_back (peer);
}

void
OverlayPath::Reverse ()
{
  m_reversed = !m_reversed;
}


OverlayPath::Iterator
OverlayPath::Begin ()
{
  if (m_reversed)
    return m_path.rbegin();
  return m_path.begin();
}


OverlayPath::Iterator
OverlayPath::End ()
{
  if (m_reversed)
    return m_path.rend();
  return m_path.end();
}

