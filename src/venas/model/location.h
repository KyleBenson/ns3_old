/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __LOCATION_H__
#define __LOCATION_H__

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
class Location {
private:
  float lat;
  float lon;

public:
  /** Create location using coordinates specified as floats/doubles. **/
  Location(float latitude,float longitude);
  Location(double latitude,double longitude);

  /** Create location using coordinates specified as an int (microdegrees). **/
  Location(int latitude,int longitude);

  /** @return - Float representation of coordinate latitude. **/
  float getLatitude();

  /** @return - Float representation of coordinate longitude. **/
  float getLongitude();

  /** @return - Int representation of coordinate longitude in microdegrees. **/
  int getLongitudeMicro();

  /** @return - Int representation of coordinate latitude in microdegrees. **/
  int getLatitudeMicro();

  //TODO: Compression


  /** @param - Iterator beginning and end over Locations to be considered.
      @return - New Location representing the centroid of all input Locations.
  **/
  template <typename Iter> static Location * centroid(Iter locs, Iter end);
};

template <typename Iter>
Location * Location::centroid(Iter locs, Iter end){
  float lat,lon = 0.0;
  int size = 0;
  for (;locs != end; locs++){
    lat += locs->getLatitude();
    lon += locs->getLongitude();
    size++;
  }
  return new Location(lat/size,lon/size);
}

//TODO: nearest neighbor

#endif /* __LOCATION_H__ */

