/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __LOCATION_H__
#define __LOCATION_H__

#include "ns3/core-module.h"

namespace ns3 {
/** 
@author: Kyle Benson

Feel free to copy, modify, redistribute this for any purpose as long as you
give me credit and don't blame me for any problems that it causes, no matter
what the severity.
**/

/** Class for representing geographic coordinates.  Support for both float
as well as microdegrees(int).  Also provides for compressing these values to a
degree as specified.
**/
class Location : public Object {
private:
  float lat;
  float lon;

public:
  /** Create location using coordinates specified as floats/doubles. **/
  Location(float latitude,float longitude);
  Location(double latitude,double longitude);

  /** Create location using coordinates specified as an int (microdegrees). **/
  Location(int latitude,int longitude);

  static TypeId GetTypeId(void);

  /** @return - Float representation of coordinate latitude. **/
  float getLatitude();

  /** @return - Float representation of coordinate longitude. **/
  float getLongitude();

  /** @return - Int representation of coordinate longitude in microdegrees. **/
  int getLongitudeMicro();

  /** @return - Int representation of coordinate latitude in microdegrees. **/
  int getLatitudeMicro();

  //TODO: Compression
  //TODO: integrate with src/core/model/vector.h
  //TODO: support ns3 attribute integration

  /** @param - Iterator beginning and end over Locations to be considered.
      Actual Location object must be reachable within ONE dereference of iterator!
      @return - Pointer to new Location representing the centroid of all input Locations.
  **/
  template <typename Iter> static Ptr<Location> centroid(Iter locs, Iter end);
};

template <typename Iter>
Ptr<Location> Location::centroid(Iter locs, Iter end){
  float lat,lon = 0.0;
  int size = 0;
  for (;locs != end; locs++){
    lat += locs->getLatitude();
    lon += locs->getLongitude();
    size++;
  }
  return CreateObject<Location>(lat/size,lon/size);
}
  
TypeId Location::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::Location")
    .SetParent<Object>()
    //.AddConstructor<Location>()
    ;
  return tid;
}

//TODO: nearest neighbor
} //namespace ns3

#endif /* __LOCATION_H__ */

