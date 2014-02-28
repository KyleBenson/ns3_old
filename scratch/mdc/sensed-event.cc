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

#include "sensed-event.h"

namespace ns3 {

  SensedEvent::SensedEvent (uint32_t eventId, Vector center, double radius, Time time)
{
  m_eventId = eventId;
  m_center = center;
  m_radius = radius;
  m_time = time;
}

uint32_t
SensedEvent::GetEventId () const
{
  return m_eventId;
}

void
SensedEvent::SetEventId (uint32_t eventId)
{
	m_eventId = eventId;
}


Vector
SensedEvent::GetCenter () const
{
  return m_center;
}

Time
SensedEvent::GetTime () const
{
  return m_time;
}

double
SensedEvent::GetRadius () const
{
  return m_radius;
}

bool
SensedEvent::WithinEventRegion (Vector loc) const
{
  return CalculateDistance (m_center, loc) <= m_radius;
}

}
