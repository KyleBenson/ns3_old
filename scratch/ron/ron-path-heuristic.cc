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
#include "boost/bind.hpp"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RonPathHeuristic");
NS_OBJECT_ENSURE_REGISTERED (RonPathHeuristic);

TypeId
RonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonPathHeuristic")
    .SetParent<Object> ()
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
{} //required to keep gcc from complaining


Ptr<OverlayPath>
RonPathHeuristic::GetNextPath (Ptr<RonPeerEntry> destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  NS_ASSERT_MSG (destination, "You must specify a valid server to use the heuristic!");

  //TODO: multiple servers

  if (!m_updatedOnce)
    ZeroLikelihoods ();
  UpdateLikelihoods (destination);
  for (std::list<Ptr<RonPathHeuristic> >::iterator others = m_aggregateHeuristics.begin ();
       others != m_aggregateHeuristics.end (); others++)
    (*others)->UpdateLikelihoods (destination);

  //find the peer with highest likelihood
  //TODO: cache up to MaxAttempts of them
  //TODO: check for valid size
  std::map<Ptr<RonPeerEntry>, double>::iterator probs = m_likelihoods.begin ();
  Ptr<RonPeerEntry> bestPeer = probs->first;
  double bestLikelihood = probs->second;
  for (; probs != m_likelihoods.end (); probs++)
    {
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


void
RonPathHeuristic::AddHeuristic (Ptr<RonPathHeuristic> other)
{
  m_aggregateHeuristics.push_back (other);
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
RonPathHeuristic::SetLikelihood (Ptr<RonPeerEntry> peer, double lh)
{
  //NS_ASSERT (0.0 <= lh and lh <= 1.0);
  NS_LOG_LOGIC ("Peer " << peer->id << " has LH " << lh);
  //m_peers.SetLikelihood (peer, 
  m_likelihoods[peer] += lh*m_weight;
}


void
RonPathHeuristic::ZeroLikelihood (Ptr<RonPeerEntry> peer)
{
  NS_ASSERT_MSG (m_likelihoods.count (peer), "Peer not present in table!");
  NS_LOG_LOGIC ("Peer " << peer->id << "'s LH cleared to 0");
  m_likelihoods[peer] = 0.0;
}


void
RonPathHeuristic::ZeroLikelihoods ()
{
  // can there actually be likelihoods without peers?
  if (m_likelihoods.size () != m_peers->GetN ())
    {
      m_likelihoods.clear ();
      for (RonPeerTable::Iterator peer = m_peers->Begin ();
           peer != m_peers->End (); peer++)
        {
          ZeroLikelihood (*peer);
        }
    }
  else
    {
      std::map<Ptr<RonPeerEntry>, double>::iterator probs = m_likelihoods.begin ();
      for (; probs != m_likelihoods.end (); probs++)
        {
          probs->second = 0.0;
        }
    }
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


bool
RonPathHeuristic::SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2)
{
  //TODO: actually define regions to be more than a coordinate
  Vector3D loc1 = peer1->location;
  Vector3D loc2 = peer2->location;
  return loc1.x == loc2.x and loc1.y == loc2.y;
}


//////////////////////////////  OverlayPath  //////////////////////////////


OverlayPath::OverlayPath(Ptr<RonPeerEntry> peer)
{
  AddPeer (peer);
}


void 
OverlayPath::AddPeer (Ptr<RonPeerEntry> peer)
{
  m_path.push_back (peer);
}


OverlayPath::Iterator
OverlayPath::Begin ()
{
  return m_path.begin();
}


OverlayPath::Iterator
OverlayPath::End ()
{
  return m_path.end();
}

