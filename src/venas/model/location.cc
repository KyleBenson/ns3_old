/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/** Class for representing geographic coordinates.  Support for both decimal
as well as microdegrees.  Also provides for compressing these values to a
degree as specified.

@author: Kyle Benson

Feel free to copy, modify, redistribute this for any purpose as long as you
give me credit and don't blame me for any problems that it causes, no matter
what the severity.
**/

#include "location.h"

Location::Location(float latitude,float longitude):
  lat(latitude),lon(longitude){}

Location::Location(double latitude,double longitude):
  lat(latitude),lon(longitude){}

Location::Location(int latitude,int longitude):
  lat(latitude*1e-6),lon(longitude*1e-6){}

float Location::getLatitude(){
  return lat;
}

float Location::getLongitude(){
  return lon;
}

int Location::getLongitudeMicro(){
  return (int)lon*1e+6;
}

int Location::getLatitudeMicro(){
  return (int)lat*1e+6;
}

  //TODO: Compression


