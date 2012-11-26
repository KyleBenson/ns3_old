* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include <boost/range/functions.hpp>

using namespace ns3;

RonPeerEntry::RonPeerEntry ()
  {}


RonPeerEntry::RonPeerEntry (Ptr<Node> node)
  {
    this.node = node;
    id = node-> GetId ();
    address = GetNodeAddress (node);
    lastContact = Simulator::Now ();
    
    MobilityModel mobility = node->GetObject<MobilityModel> ();
    NS_ASSERT_MSG (mobility, "Geocron nodes need MobilityModels for determining locations!");
    location = mobility->GetPosition ();
  }

RonPeerTable::RonPeerTable ()
{}


int
RonPeerEntry::GetNPeers ()
{
  return peers.size ();
}


RonPeerEntry
RonPeerEntry::AddPeer (RonPeerEntry entry)
{
  if (peers.count (entry) != 0)
    {
      RonPeerEntry temp = *(peers.find (entry.id));
      peers[entry.id] = entry;
      return temp;
    }
  else
    return peers[entry.id] = entry;
}


RonPeerEntry
RonPeerEntry::AddPeer (Ptr<Node> node)
{
  RonPeerEntry newEntry (node);
  return AddPeer (newEntry);
}


bool
RonPeerEntry::RemovePeer (int id)
{
  bool retValue = false;
  if (peers.count (id))
    retValue = true;
  peers.erase (peers.find (id));
  return retValue;
}


Iterator
RonPeerEntry::GetPeer (int id)
{
  return peers.find (id);
}


bool
RonPeerEntry::IsInTable (int id)
{
  return IsInTable (GetPeer (id));
}


bool
RonPeerEntry::IsInTable (Iterator itr)
{
  return itr != End ();
}


Iterator
RonPeerEntry::Begin ()
{
  return boost::begin (boost::adaptors::values (peers));
/*template<typename T1, typename T2> T2& take_second(const std::pair<T1, T2> &a_pair)
{
  return a_pair.second;
}
boost::make_transform_iterator(a_map.begin(), take_second<int, string>),*/
}


Iterator
RonPeerEntry::End ()
{
  return boost::end (boost::adaptors::values (peers));
}
