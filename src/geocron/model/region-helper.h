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

#ifndef REGION_HELPER_H
#define REGION_HELPER_H

#include "ns3/core-module.h"
#include "region.h"

#include <boost/unordered_map.hpp>
#include <map>

namespace ns3 {

/** Simple helper class for assigning Regions for specific location Vectors. */
class RegionHelper : public Object
{
  //boost::unordered_map<Vector, Location> regions;
  std::map<Vector, Location> regions;
public:
  RegionHelper () {}

  static TypeId GetTypeId () {
    static TypeId tid = TypeId ("ns3::RegionHelper")
      .SetParent<Object> ()
      .AddConstructor<RegionHelper> ()
      ;
    return tid;
  }

  Location GetRegion (Vector loc) {
    NS_ASSERT_MSG(regions.count(loc), "No region for this location!");
    return regions[loc];
  }

  // Sets the Region associated with the given Vector location
  // Returns the old Location, if one existed, the new one otherwise
  Location SetRegion (Vector loc, Location newRegion) {
    Location retLoc = (regions.count(loc) ? regions[loc] : newRegion);
    regions[loc] = newRegion;
    return retLoc;
  }
};  

// For Rocketfuel topologies, we have to explicitly set Region info anyway so...
typedef RegionHelper RocketfuelRegionHelper;

//TODO: implement!
typedef RegionHelper BriteRegionHelper;

} //namespace
#endif //REGION_HELPER_H
