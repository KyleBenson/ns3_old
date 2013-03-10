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

#include "ron-peer-table.h"
#include "failure-helper-functions.h"
#include "geocron-experiment.h"

#include <boost/range/functions.hpp>

using namespace ns3;

RonPeerEntry::RonPeerEntry () {}

RonPeerEntry::RonPeerEntry (Ptr<Node> node)
  {
    this->node = node;
    id = node-> GetId ();
    address = GeocronExperiment::GetNodeAddress (node);
    lastContact = Simulator::Now ();
    
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    NS_ASSERT_MSG (mobility != NULL, "Geocron nodes need MobilityModels for determining locations!");    
    location = mobility->GetPosition ();
  }


uint32_t
RonPeerTable::GetN ()
{
  return m_peers.size ();
}


Ptr<RonPeerEntry>
RonPeerTable::AddPeer (Ptr<RonPeerEntry> entry)
{
  if (m_peers.count (entry->id) != 0)
    {
      Ptr<RonPeerEntry> temp = (*(m_peers.find (entry->id))).second;
      m_peers[entry->id] = entry;
      return temp;
    }
  else
    return m_peers[entry->id] = entry;
}


Ptr<RonPeerEntry>
RonPeerTable::AddPeer (Ptr<Node> node)
{
  Ptr<RonPeerEntry> newEntry = Create<RonPeerEntry> (node);
  return AddPeer (newEntry);
}


bool
RonPeerTable::RemovePeer (int id)
{
  bool retValue = false;
  if (m_peers.count (id))
    retValue = true;
  m_peers.erase (m_peers.find (id));
  return retValue;
}


Ptr<RonPeerEntry>
RonPeerTable::GetPeer (int id)
{
  if (m_peers.count (id))
    return  ((*(m_peers.find (id))).second);
  else
    return NULL;
}


bool
RonPeerTable::IsInTable (int id)
{
  return m_peers.count (id);
}


bool
RonPeerTable::IsInTable (Iterator itr)
{
  return IsInTable ((*itr)->id);
}


RonPeerTable::Iterator
RonPeerTable::Begin ()
{
  return boost::begin (boost::adaptors::values (m_peers));
/*template<typename T1, typename T2> T2& take_second(const std::pair<T1, T2> &a_pair)
{
  return a_pair.second;
}
boost::make_transform_iterator(a_map.begin(), take_second<int, string>),*/
}


RonPeerTable::Iterator
RonPeerTable::End ()
{
  return boost::end (boost::adaptors::values (m_peers));
}
