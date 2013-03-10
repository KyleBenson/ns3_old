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

////////////////////////////////////////////////////////////////////////////////
//////////$$$$$$$$$$      Heuristic functions      $$$$$$$$$$///////////////////
////////////////////////////////////////////////////////////////////////////////


void
RandomRonPathHeuristic::UpdateLikelihoods (Ptr<RonPeerEntry> destination)
{
  //only need to assign random probs once,
  //but should set them to 0 if we failed to reach a peer
  if (!m_updatedOnce)
    {
      for (RonPeerTable::Iterator peer = m_peers->Begin ();
           peer != m_peers->End (); peer++)
        SetLikelihood (*peer, random.GetValue ());
      m_updatedOnce = true;
    }
  else
    {
      //only need to update last attempted peer
      ClearLikelihood (m_peersAttempted.back ());
    }
}


void
OrthogonalRonPathHeuristic::UpdateLikelihoods (Ptr<RonPeerEntry> destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  
  if (!m_updatedOnce)
    {
      for (RonPeerTable::Iterator peer = m_peers->Begin ();
           peer != m_peers->End (); peer++)
        {
          // don't bother solving if peer is within the source or destination's region
          if (SameRegion (m_source, *peer) or SameRegion (destination, *peer))
            {
              //std::cout << "Ignoring peer at location (" << (*peer)->location.x << "," << (*peer)->location.y << ")" <<std::endl;
              ClearLikelihood (*peer);
              continue;
            }

          double pi = 3.14159265;
          double orthogonal = pi/2.0;

          /*  We have a triangle where the source is point a, destination is point b, and overlay peer is c.
              We compute angle c and want it to be as close to right as possible (hence orthogonal).
              We also want the distance of the line from c to a point on line ab such that the lines are perpendicular. */
          Vector3D va = m_source->location;
          Vector3D vb = destination->location;
          Vector3D vc = (*peer)->location;

          double ab_dist = CalculateDistance (va, vb);
          double ac_dist = CalculateDistance (va, vc);
          double bc_dist = CalculateDistance (vb, vc);

          // compute angles with law of cosines
          double c_ang = acos ((ac_dist * ac_dist + bc_dist * bc_dist - ab_dist * ab_dist) /
                               (2.0 * ac_dist * bc_dist));

          // compute bc angles for finding the perpendicular distance
          double a_ang = acos ((ac_dist * ac_dist + ab_dist * ab_dist - bc_dist * bc_dist) /
                               (2.0 * ac_dist * ab_dist));

          // find perpendicular distances using law of sines
          double perpDist = ac_dist * sin (a_ang);

          // ideal distance is when c is located halfway between a and b,
          // which would make an isosceles triangle, so legs are easy to compute
          double half_ab_dist = 0.5*ab_dist;
          double ac_ideal_dist = sqrt (2.0) * half_ab_dist;
          double ideal_dist = sqrt (ac_ideal_dist * ac_ideal_dist - half_ab_dist * half_ab_dist);
  
          // find 'normalized (0<=e<=1) percent error' from ideals
          //-exp ensures 0<=e<=1 and further penalizes deviations from the ideal
          // std::cout << "ideal_dist=" << ideal_dist <<std::endl;
          // std::cout << "c_ang=" << c_ang <<std::endl;
          // double ang_err = exp(-1*abs((c_ang - orthogonal) / orthogonal));
          // double dist_err = exp(-1*abs((perpDist - ideal_dist) / ideal_dist));
          double ang_err = (abs((c_ang - orthogonal) / orthogonal));
          double dist_err = (abs((perpDist - ideal_dist) / ideal_dist));

          // std::cout << "ang_err=" << ang_err <<std::endl;

          double newLikelihood = (ang_err + dist_err);
          SetLikelihood (*peer, newLikelihood);
          // std::cout << "LH=" << newLikelihood <<std::endl;
          //TODO: weight one error over another?
        }
      m_updatedOnce = true;
    }
  else
    {
      //only need to update last attempted peer
      ClearLikelihood (m_peersAttempted.back ());
    }
}


////////////////////////////////////////////////////////////////////////////////
//////////$$$$$$$$$$     Class definitions         $$$$$$$$$$///////////////////
////////////////////////////////////////////////////////////////////////////////

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
///############
NS_OBJECT_ENSURE_REGISTERED (OrthogonalRonPathHeuristic);

OrthogonalRonPathHeuristic::OrthogonalRonPathHeuristic ()
{
}

TypeId
OrthogonalRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OrthogonalRonPathHeuristic")
    .AddConstructor<OrthogonalRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("ortho"),
                   MakeStringAccessor (&OrthogonalRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("ortho"),
                   MakeStringAccessor (&OrthogonalRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}

NS_OBJECT_ENSURE_REGISTERED (RandomRonPathHeuristic);

RandomRonPathHeuristic::RandomRonPathHeuristic ()
{
}


TypeId
RandomRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomRonPathHeuristic")
    .AddConstructor<RandomRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("rand"),
                   MakeStringAccessor (&RandomRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("rand"),
                   MakeStringAccessor (&RandomRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    ;

  //NS_ASSERT (tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("rand")));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("rand"));

  return tid;
}

///############

RonPathHeuristic::~RonPathHeuristic ()
{} //required to keep gcc from complaining


Ptr<OverlayPath>
RonPathHeuristic::GetNextPath (Ptr<RonPeerEntry> destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  NS_ASSERT_MSG (destination, "You must specify a valid server to use the heuristic!");

  //TODO: multiple servers

  if (!m_updatedOnce)
    ClearLikelihoods ();
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
RonPathHeuristic::ClearLikelihood (Ptr<RonPeerEntry> peer)
{
  NS_ASSERT_MSG (m_likelihoods.count (peer), "Peer not present in table!");
  NS_LOG_LOGIC ("Peer " << peer->id << "'s LH cleared to 0");
  m_likelihoods[peer] = 0.0;
}


void
RonPathHeuristic::ClearLikelihoods ()
{
  if (m_likelihoods.size () != m_peers->GetN ())
    {
      m_likelihoods.clear ();
      for (RonPeerTable::Iterator peers = m_peers->Begin ();
           peers != m_peers->End (); peers++)
        {
          m_likelihoods[(*peers)] = 0.0;
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

