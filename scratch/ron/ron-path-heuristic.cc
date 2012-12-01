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

#include "ron-path-heuristic.h"
#include "boost/bind.hpp"

using namespace ns3;

////////////////////////////////////////////////////////////////////////////////
//////////$$$$$$$$$$      Heuristic functions      $$$$$$$$$$///////////////////
////////////////////////////////////////////////////////////////////////////////


bool 
RandomRonPathHeuristic::ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2)
{
  
  return random.GetValue () < 0.5;
  //return rand () % 2;
}


bool
OrthogonalRonPathHeuristic::ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2)
{
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//////////$$$$$$$$$$     Class definitions         $$$$$$$$$$///////////////////
////////////////////////////////////////////////////////////////////////////////



Ptr<RonPathHeuristic>
RonPathHeuristic::CreateHeuristic (Heuristic heuristic)
{
  switch (heuristic)
    {
    case ORTHOGONAL:
      return Create<OrthogonalRonPathHeuristic> ();
    default: //RANDOM
      return Create<RandomRonPathHeuristic> ();
    };
}


RonPeerEntry
RonPathHeuristic::GetNextPeer (Ptr<RonPeerEntry> destination)
{
  if (peerHeap.size () <= 0)
    throw NoValidPeerException();

  RonPeerEntry ret = peerHeap.front ();
  std::pop_heap (peerHeap.begin (), peerHeap.end (), GetPeerComparator (destination));
  peerHeap.pop_back ();
  return ret;
}


Ipv4Address
RonPathHeuristic::GetNextPeerAddress (Ptr<RonPeerEntry> destination)
{
  return GetNextPeer (destination).address;
}


void
RonPathHeuristic::SetPeerTable (Ptr<RonPeerTable> table)
{
  peers = table;

  for (RonPeerTable::Iterator itr = peers->Begin (); itr != peers->End (); itr++)
    {
      peerHeap.push_back (*itr);
    }

  std::make_heap (peerHeap.begin (), peerHeap.end (), GetPeerComparator ());
}


boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)>
//void *
RonPathHeuristic::GetPeerComparator (Ptr<RonPeerEntry> destination /* = NULL*/)
{
  return boost::bind (&RonPathHeuristic::ComparePeers, this, destination, _1, _2);
}
