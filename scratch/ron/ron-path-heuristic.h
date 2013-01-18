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

#ifndef RON_PATH_HEURISTIC_H
#define RON_PATH_HEURISTIC_H

#include "ron-peer-table.h"
#include "boost/function.hpp"
#include <vector>

namespace ns3 {

//TODO: enum for choosing which heuristic?

/** This class represents a heuristic for choosing overlay paths.  Derived classes must override the ComparePeers
    function to implement the actual heuristic logic. */
class RonPathHeuristic : public SimpleRefCount<RonPathHeuristic>
{
public:
  enum Heuristic
    {
      RANDOM = 0,
      ORTHOGONAL = 1
    };

  static Ptr<RonPathHeuristic> CreateHeuristic (Heuristic heuristic);
  virtual ~RonPathHeuristic ();

  RonPeerEntry GetNextPeer (Ptr<RonPeerEntry> destination);
  Ipv4Address GetNextPeerAddress (Ptr<RonPeerEntry> destination);
  void SetPeerTable (Ptr<RonPeerTable> table);
  void SetSourcePeer (Ptr<RonPeerEntry> peer);
  Ptr<RonPeerEntry> GetSourcePeer ();

  class NoValidPeerException : public std::exception
  {
  public:
    virtual const char* what() const throw()
    {
      return "No valid peers could be chosen for the overlay.";
    }
  };

protected:
  std::vector<RonPeerEntry> peerHeap;
  Ptr<RonPeerTable> peers;
  UniformVariable random; //for random decisions
  Ptr<RonPeerEntry> m_source;

  /** Returns true if peer1 has 'higher priority' than peer2, false otherwise.*/
  virtual bool ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2) = 0;
  virtual bool SameRegion (RonPeerEntry peer1, RonPeerEntry peer2);

private:
  boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)> GetPeerComparator (Ptr<RonPeerEntry> destination = NULL);
};

class RandomRonPathHeuristic : public RonPathHeuristic
{
  virtual bool ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2);
};


class OrthogonalRonPathHeuristic : public RonPathHeuristic
{
  virtual bool ComparePeers (Ptr<RonPeerEntry> destination, RonPeerEntry peer1, RonPeerEntry peer2);
};

} //namespace
#endif //RON_PATH_HEURISTIC_H
