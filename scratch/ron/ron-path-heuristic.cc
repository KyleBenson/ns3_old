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


bool 
RandomRonPathHeuristic::ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2)
{
  return random.GetValue () < 0.5;
  //return rand () % 2;
}


bool
OrthogonalRonPathHeuristic::ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");

  // don't bother solving if either choice is within the source or destination's region
  if (SameRegion (*m_source, peer1) or SameRegion (*destination, peer1))
    return false;
  if (SameRegion (*m_source, peer2) or SameRegion (*destination, peer2))
    return true;

  double pi = 3.14159265;
  double orthogonal = pi/2.0;

  /*  We have a triangle where the source is point a, destination is point b, and overlay peer is c.
      We compute angle c and want it to be as close to right as possible (hence orthogonal).
      We also want the distance of the line from c to a point on line ab such that the lines are perpendicular. */
  Vector3D va = m_source->location;
  Vector3D vb = destination->location;
  Vector3D vc1 = peer1.location;
  Vector3D vc2 = peer2.location;

  double ab_dist = CalculateDistance (va, vb);
  double ac1_dist = CalculateDistance (va, vc1);
  double ac2_dist = CalculateDistance (va, vc2);
  double bc1_dist = CalculateDistance (vb, vc1);
  double bc2_dist = CalculateDistance (vb, vc2);

  // compute angles with law of cosines
  double c1_ang = acos ((ac1_dist * ac1_dist + bc1_dist * bc1_dist - ab_dist * ab_dist) /
                        (2 * ac1_dist * bc1_dist));
  double c2_ang = acos ((ac2_dist * ac2_dist + bc2_dist * bc2_dist - ab_dist * ab_dist) /
                        (2 * ac2_dist * bc2_dist));

  // compute bc angles for finding the perpendicular distance
  double a1_ang = acos ((ac1_dist * ac1_dist + ab_dist * ab_dist - bc1_dist * bc1_dist) /
                        (2 * ac1_dist * ab_dist));
  double a2_ang = acos ((ac2_dist * ac2_dist + ab_dist * ab_dist - bc2_dist * bc2_dist) /
                        (2 * ac2_dist * ab_dist));

  // find perpendicular distances using law of sines
  double perpDist1 = ac1_dist * sin (a1_ang);
  double perpDist2 = ac2_dist * sin (a2_ang);

  // ideal distance is when c is located halfway between a and b,
  // which would make an isosceles triangle, so legs are easy to compute
  double ac_ideal_dist = sqrt ((ab_dist * ab_dist) / 2);
  double ideal_dist = sqrt (ac_ideal_dist * ac_ideal_dist - ab_dist * ab_dist / 4);
  
  // find 'percent error' from ideals
  double ang1_err = abs ((c1_ang - orthogonal) / orthogonal);
  double ang2_err = abs ((c2_ang - orthogonal) / orthogonal);
  double dist1_err = abs ((perpDist1 - ideal_dist) / ideal_dist);
  double dist2_err = abs ((perpDist2 - ideal_dist) / ideal_dist);

  // we want to minimize the square of these two values to further penalize deviations from the ideal
  return (ang1_err * ang1_err + dist1_err * dist1_err) < (ang2_err * ang2_err + dist2_err * dist2_err);
}


////////////////////////////////////////////////////////////////////////////////
//////////$$$$$$$$$$     Class definitions         $$$$$$$$$$///////////////////
////////////////////////////////////////////////////////////////////////////////



Ptr<RonPathHeuristic>
RonPathHeuristic::CreateHeuristic (Heuristic heuristic)
{
  switch (heuristic)
    {
    case ORTHOGONAL:
      return Create<OrthogonalRonPathHeuristic> ();
    default: //RANDOM
      return Create<RandomRonPathHeuristic> ();
    };
}


RonPathHeuristic::~RonPathHeuristic ()
{} //required to keep gcc from complaining


RonPeerEntry
RonPathHeuristic::GetNextPeer (Ptr<RonPeerEntry> destination)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");

  if (peerHeap.size () <= 0)
    throw NoValidPeerException();

  RonPeerEntry ret = peerHeap.front ();
  std::pop_heap (peerHeap.begin (), peerHeap.end (), GetPeerComparator (destination));
  peerHeap.pop_back ();
  return ret;
}


Ipv4Address
RonPathHeuristic::GetNextPeerAddress (Ptr<RonPeerEntry> destination)
{
  return GetNextPeer (destination).address;
}


void
RonPathHeuristic::SetPeerTable (Ptr<RonPeerTable> table)
{
  peers = table;

  for (RonPeerTable::Iterator itr = peers->Begin (); itr != peers->End (); itr++)
    {
      peerHeap.push_back (*itr);
    }

  std::make_heap (peerHeap.begin (), peerHeap.end (), GetPeerComparator ());
}


void
RonPathHeuristic::SetSourcePeer (Ptr<RonPeerEntry> peer)
{
  m_source = peer;
}


Ptr<RonPeerEntry>
RonPathHeuristic::GetSourcePeer ()
{
  return m_source;
}


boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)>
//void *
RonPathHeuristic::GetPeerComparator (Ptr<RonPeerEntry> destination /* = NULL*/)
{
  return boost::bind (&RonPathHeuristic::ComparePeers, this, destination, _1, _2);
}


bool
RonPathHeuristic::SameRegion (RonPeerEntry peer1, RonPeerEntry peer2)
{
  Vector3D loc1 = peer1.location;
  Vector3D loc2 = peer2.location;
  return loc1.x == loc2.x and loc1.y == loc2.y;
}
