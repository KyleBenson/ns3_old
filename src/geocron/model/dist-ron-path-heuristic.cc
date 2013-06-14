/* 
 * File:   dist-ron-path-heuristic.cc
 * Author: zhipengh
 *
 * Created on June 12, 2013, 2:25 PM
 */

#include "dist-ron-path-heuristic.h"
#include <cmath>

using namespace ns3;


NS_OBJECT_ENSURE_REGISTERED (DistRonPathHeuristic);

DistRonPathHeuristic::DistRonPathHeuristic ()
{
}

TypeId
DistRonPathHeuristic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DistRonPathHeuristic")
    .AddConstructor<DistRonPathHeuristic> ()
    .SetParent<RonPathHeuristic> ()
    .SetGroupName ("RonPathHeuristics")
    .AddAttribute ("SummaryName", "Short name that summarizes parameters, aggregations, etc. to be used when creating filenames",
                   StringValue ("dist"),
                   MakeStringAccessor (&DistRonPathHeuristic::m_summaryName),
                   MakeStringChecker ())
    .AddAttribute ("ShortName", "Short name that only captures the name of the heuristic",
                   StringValue ("dist"),
                   MakeStringAccessor (&DistRonPathHeuristic::m_shortName),
                   MakeStringChecker ())
    .AddAttribute ("MaxDistance", "Minimum distance we will permit for peers.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&DistRonPathHeuristic::m_minDistance),
                   MakeDoubleChecker<double> ())
    ;
  //tid.SetAttributeInitialValue ("ShortName", Create<StringValue> ("ortho"));
  //tid.SetAttributeInitialValue ("SummaryName", Create<StringValue> ("ortho"));

  return tid;
}


double
DistRonPathHeuristic::GetLikelihood (Ptr<RonPath> path)
{
  NS_ASSERT_MSG (m_source, "You must set the source peer before using the heuristic!");
  Ptr<RonPeerEntry> peer = (*(*path->Begin ())->Begin ());
  Ptr<RonPeerEntry> destination = *path->GetDestination ()->Begin ();
  
  Vector3D va = m_source->location;
  std::cout<<"source axis = "<<va.x<<","<<va.y<<std::endl;
  Vector3D vb = destination->location;
  std::cout<<"dest axis = "<<vb.x<<","<<vb.y<<std::endl;
  Vector3D vc = peer->location;
  std::cout<<"peer axis = "<<vc.x<<","<<vc.y<<std::endl;
  double sddist = CalculateDistance (va, vb);
  double distance = CalculateDistance (va, vc);

  double idealDistance = sddist / 2 ;
  std::cout<<"ideal distance is = "<<idealDistance<<std::endl;
  std::cout<<"the distance is "<<distance<<std::endl;

  if (distance <= m_minDistance)
      return 0;
  else if(distance < idealDistance and distance > m_minDistance)
  {
      double likelihood = (distance * distance) / (idealDistance * idealDistance); 
      std::cout<<"likelihood for the small distance is "<<likelihood<<std::endl;
      return likelihood;
  }
  else if(distance >= idealDistance)
  {
      double likelihood = idealDistance / distance ;
      std::cout<<"likelihood for the large distance is "<<likelihood<<std::endl;
      return likelihood;
  }
  else return 0;
  

  //TODO: transform to a LH
  //return distance - m_minDistance;
}