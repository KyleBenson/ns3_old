/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2012 University of California, Irvine
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

#ifndef SENSED_EVENT_H
#define SENSED_EVENT_H

#include "ns3/vector.h"
#include "ns3/nstime.h"

namespace ns3 {

class SensedEvent
{
 public:
  SensedEvent (uint32_t eventId, Vector center, double radius, Time time);
  Vector GetCenter () const;
  bool WithinEventRegion (Vector loc) const;
  Time GetTime () const;
  uint32_t GetEventId () const;

 private:
  uint32_t m_eventId;
  Vector m_center;
  double m_radius;
  Time m_time;
};

}

#endif /*SENSED_EVENT_H*/
