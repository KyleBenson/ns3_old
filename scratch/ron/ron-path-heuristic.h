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
#include "ns3/core-module.h"
#include <vector>

/*#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>*/
#include <boost/unordered_map.hpp>
//#include "boost/function.hpp"

namespace ns3 {

//TODO: enum for choosing which heuristic?

/** This class provides a simple iterable container for storing the path of peer entries. */
class OverlayPath : public SimpleRefCount<OverlayPath>
{
private:
  typedef std::list<Ptr<RonPeerEntry> > underlyingContainer;
  underlyingContainer m_path;
  bool m_reversed;
public:
  typedef underlyingContainer::iterator Iterator;
  OverlayPath(Ptr<RonPeerEntry> peer);

  /** Adds a peer to the end of the path. */
  void AddPeer (Ptr<RonPeerEntry> peer);
  //TODO: void InsertPeer ChangePeerAt?

  /** Reverses the ordering of the path.
      Actually, it just stores a flag and reverses calls to it the path. */
  void Reverse ();

  Iterator Begin ();
  Iterator End ();
};


/** This class represents a heuristic for choosing overlay paths.  Derived classes must override the ComparePeers
    function to implement the actual heuristic logic. */
class RonPathHeuristic : public Object
{
public:
  static TypeId GetTypeId (void);

  virtual ~RonPathHeuristic ();

  /** Return the best path, according to the aggregate heuristics, to the destination. */
  Ptr<OverlayPath> GetBestPath (Ptr<RonPeerEntry> destination);

  /** Return the best multicast path, according to the aggregate heuristics, to the destinations. */
  //TODO: Ptr<OverlayPath> GetBestMulticastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Return the best anycast path, according to the aggregate heuristics, to the destination. */
  //TODO: Ptr<OverlayPath> GetBestAnycastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Inform this heuristic that it will be the top-level for an aggregate heuristic group. */
  void MakeTopLevel ();

  void AddHeuristic (Ptr<RonPathHeuristic> other);
  void SetPeerTable (Ptr<RonPeerTable> table);
  void SetSourcePeer (Ptr<RonPeerEntry> peer);
  Ptr<RonPeerEntry> GetSourcePeer ();

  ////////////////////  notifications / updates  /////////////////////////////////

  /** Application will notify heuristics about ACKs. */
  void NotifyAck (Ptr<OverlayPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives an ACK notification.
      Default is to set likelihoods to 1 and update contact times. */
  virtual void DoNotifyAck (Ptr<OverlayPath> path, Time time);

  /** Application will notify heuristics about timeouts. */
  void NotifyTimeout (Ptr<OverlayPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives a timeout notification.
      Default is to set first likelihood on path to 0, unless an ACK of the partial path occurred. */
  virtual void DoNotifyTimeout (Ptr<OverlayPath> path, Time time);

  class NoValidPeerException : public std::exception
  {
  public:
    virtual const char* what() const throw()
    {
      return "No valid peers could be chosen for the overlay.";
    }
  };

protected:

  ///////////////////////// likelihood manipulation ////////////////////

  /** Sets the estimated likelihood of reaching the specified peer */
  void SetLikelihood (Ptr<RonPeerEntry> peer, double lh);

  /** Get the likelihood of peer being a successful overlay for reaching destination.
      By default, always assumes destination is reachable via returns 1.0
      The MOST IMPORTANT method to override/implement.
   Can be chained together for multiple-hop overlays. */
  virtual double GetLikelihood (Ptr<RonPeerEntry> peer, Ptr<RonPeerEntry> destination);

  /** Runs on all heuristics before UpdateLikelihoods. */
  //void PreUpdateLikelihoods (Ptr<RonPeerEntry> destination);
  /** By default, zeros likelihood of the last attempted process.
      Should be overridden if individual peer likelihoods 
      are to be based on some knowledge accumulated during the PreUpdateLikelihoods pass. */
  //virtual void DoPreUpdateLikelihoods (Ptr<RonPeerEntry> destination);

  /** Set the estimated likelihood of reaching the destination through each peer. 
      Assumes that we only need to assign random probs once, considered an off-line heuristic.
      Also assumes we should set LH to 0 if we failed to reach a peer.
      Override to change these assumptions (i.e. on-line heuristics),
      but take care to ensure good algorithmic complexity!
  */
  virtual void DoUpdateLikelihoods (Ptr<RonPeerEntry> destination);
  /** Ensures that DoUpdateLikelihoods is run for each aggregate heuristic */
  void UpdateLikelihoods (Ptr<RonPeerEntry> destination);

  /** Runs on all heuristics after UpdateLikelihoods. */
  // virtual void PostUpdateLikelihoods (Ptr<RonPeerEntry> destination);
  /** Should be overridden if individual peer likelihoods are to be
      based on some knowledge accumulated during the PostUpdateLikelihoods pass. */
  // virtual void DoPostUpdateLikelihoods (Ptr<RonPeerEntry> destination);

  /*virtual void ZeroLikelihoods ();
    virtual void ZeroLikelihood (Ptr<RonPeerEntry> destination);*/
  virtual bool SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2);

  /** May only need to assign likelihoods once.
      Set this to true to avoid clearing LHs after each newly chosen peer. */
  bool m_updatedOnce;
  Ptr<RonPeerTable> m_peers;
  UniformVariable random; //for random decisions
  Ptr<RonPeerEntry> m_source;
  std::string m_summaryName;
  std::string m_shortName;
  double m_weight;

  /** This  helps us aggregate the likelihoods together from each of the
      aggregate heuristics.  We store a list of all the LHs (1 per heuristic) and
      take a reference to a newly created heuristic's LH when it is being built. */
  typedef std::list<double> Likelihood;

  typedef std::list<Ptr<RonPeerEntry> > PeersAttempted;
  PeersAttempted m_peersAttempted;
  typedef std::list<Ptr<RonPathHeuristic> > AggregateHeuristics;
  AggregateHeuristics m_aggregateHeuristics;

  //TODO: move this to the PeerTable when it's COW
  //likelihoods are addressed in several ways by different heuristics
  /*typedef
  boost::multi_index_container<
    std::pair<double, Ptr<RonPeerEntry> >,
    indexed_by<
      ordered_unique<identity<double> >,
      hashed_unique<Ptr<RonPeerEntry> >,
      >
      >*/

  /** The likelihoods are indexed by the destination and then the path proposed for the destination. */

  typedef Ptr<RonPeerEntry> PeerDestination;
  typedef boost::unordered_map<Ptr<OverlayPath>, *double> SinglePathLikelihoodInnerTable;
  typedef boost::unordered_map<PeerDestination, SinglePathLikelihoodInnerTable> PathLikelihoodTable;
  PathLikelihoodTable m_likelihoods;

  /** Only the top-level of a group of aggregate heuristics should manage this. */
  typedef boost::unordered_map<PeerDestination, 
                               boost::unordered_map<Ptr<OverlayPath>, std::list<double> > >*
  MasterPathLikelihoodTable;
  MasterPathLikelihoodTable m_masterLikelihoods;

  bool m_topLevel;

  // typedef LikelihoodTable::nth_index<0>::type::iterator LikelihoodIterator;
  // typedef LikelihoodTable::nth_index<1>::type::iterator LikelihoodPeerIterator;
  // typedef LikelihoodTable::nth_index<1>::type LikelihoodPeerMap;  

  //boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)> GetPeerComparator (Ptr<RonPeerEntry> destination = NULL);
};
} //namespace
#endif //RON_PATH_HEURISTIC_H
