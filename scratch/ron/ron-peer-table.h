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

#ifndef RON_PEER_TABLE_H
#define RON_PEER_TABLE_H

#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include <boost/range/adaptor/map.hpp>
#include <map>

//TODO: enum for choosing which heuristic?

namespace ns3 {

class RonPeerEntry : public SimpleRefCount<RonPeerEntry>
{
public:
  RonPeerEntry ();
  RonPeerEntry (Ptr<Node> node);
  
  //Ron Attributes
  int id;
  Ipv4Address address;
  Ptr<Node> node;
  Time lastContact;
  //TODO: label enum for grouping?

  //Geocron Attributes
  Vector location;
  //TODO: failures reported counter / timer
};  

class RonPeerTable : public SimpleRefCount<RonPeerTable>
{
 private:
  typedef std::map<int, RonPeerEntry> underlyingMapType;
  typedef boost::range_detail::select_second_mutable_range<underlyingMapType> underlyingIterator;
 public:
  typedef boost::range_iterator<underlyingIterator>::type Iterator;

  int GetN ();

  /** Returns old entry if it existed, else new one. */
  RonPeerEntry AddPeer (RonPeerEntry entry);
  /** Returns old entry if it existed, else new one. */
  RonPeerEntry AddPeer (Ptr<Node> node);
  /** Returns true if entry existed. */
  bool RemovePeer (int id);
  /** Returns iterator to requested entry. Use IsInTable to verify its prescence in the table. */
  Ptr<RonPeerEntry> GetPeer (int id);
  bool IsInTable (int id);
  bool IsInTable (Iterator itr);
  //TODO: other forms of get/remove
  Iterator Begin ();
  Iterator End ();

 private:
  std::map<int, RonPeerEntry> peers;
};

} //namespace
#endif //RON_PEER_TABLE_H
