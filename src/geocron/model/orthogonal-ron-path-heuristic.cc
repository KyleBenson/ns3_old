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

#include "orthogonal-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;


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


double
OrthogonalRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer(s) before using the heuristic!");
  
  Ptr<RonPeerEntry> peer = (*(*path->Begin ())->Begin ());
  Ptr<PeerDestination> destination = path->GetDestination ();

  // don't bother solving if peer is within the source or destination's region
  if (SameRegion (m_source, peer) or SameRegion (*destination->Begin (), peer))
    {
      //std::cout << "Ignoring peer at location (" << (*peer)->location.x << "," << (*peer)->location.y << ")" <<std::endl;
      SetLikelihood (path, 0);
      return 0;
    }

  double pi = 3.14159265;
  double orthogonal = pi/2.0;

  /*  We have a triangle where the source is point a, destination is point b, and overlay peer is c.
      We compute angle c and want it to be as close to right as possible (hence orthogonal).
      We also want the distance of the line from c to a point on line ab such that the lines are perpendicular. */
  Vector3D va = m_source->location;
  Vector3D vb = (*destination->Begin ())->location;
  Vector3D vc = peer->location;

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
  return newLikelihood;
}
