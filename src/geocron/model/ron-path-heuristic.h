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

#include "ns3/core-module.h"
#include <vector>

#include "ron-peer-table.h"
#include "ron-path.h"

/*#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>*/
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#include <set>
//#include "boost/function.hpp"

namespace ns3 {

/** This class represents a heuristic for choosing overlay paths.  Derived classes must override the ComparePeers
    function to implement the actual heuristic logic. */
class RonPathHeuristic : public Object
{
public:
  static TypeId GetTypeId (void);

  RonPathHeuristic ();
  virtual ~RonPathHeuristic ();

  /** Return the best path, according to the aggregate heuristics, to the destination. */
  Ptr<RonPath> GetBestPath (Ptr<PeerDestination> destination);

  /** Return the best multicast path, according to the aggregate heuristics, to the destinations. */
  //TODO: Ptr<RonPath> GetBestMulticastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Return the best anycast path, according to the aggregate heuristics, to the destination. */
  //TODO: Ptr<RonPath> GetBestAnycastPath (itr<Ptr<RonPeerEntry???> destinations);

  /** Inform this heuristic that it will be the top-level for an aggregate heuristic group. */
  void MakeTopLevel ();

  void AddHeuristic (Ptr<RonPathHeuristic> other);
  void SetPeerTable (Ptr<RonPeerTable> table);
  void SetSourcePeer (Ptr<RonPeerEntry> peer);
  Ptr<RonPeerEntry> GetSourcePeer ();

  ////////////////////  notifications / updates  /////////////////////////////////

  /** Application will notify heuristics about ACKs. */
  void NotifyAck (Ptr<RonPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives an ACK notification.
      Default is to set likelihoods to 1 and update contact times. */
  virtual void DoNotifyAck (Ptr<RonPath> path, Time time);

  /** Application will notify heuristics about timeouts. */
  void NotifyTimeout (Ptr<RonPath> path, Time time);

  /** Override this function to control what happens when this heuristic receives a timeout notification.
      Default is to set first likelihood on path to 0, unless an ACK of the partial path occurred. */
  virtual void DoNotifyTimeout (Ptr<RonPath> path, Time time);

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
  void SetLikelihood (Ptr<RonPath> path, double lh);

  /** Get the likelihood of path being a successful overlay for reaching destination.
      By default, always assumes destination is reachable, i.e. always returns 1.0.
      The MOST IMPORTANT method to override/implement.
   Can be chained together for multiple-hop overlays. */
  virtual double GetLikelihood (Ptr<RonPath> path);

  /** Adds the given path to the master likelihood table of the top-level heuristic,
      updates all of the other likelhood tables. */
  void AddPath (Ptr<RonPath> path);

  /** Runs DoBuildPaths on all heuristics. */
  void BuildPaths (Ptr<PeerDestination> destination);

  /** Generate enough paths for the heuristics to use.
      By default, it will generate all possible one-hop paths to the destination the first
      time it sees that destination.  It will NOT generate duplicates.
      Override for multi-hop paths or to prune this search tree.
      NOTE: make sure you check for duplicate paths when using aggregate heuristics! */
  virtual void DoBuildPaths (Ptr<PeerDestination> destination);

  /** Handles adding a path to the master likelihood table and updating all other heuristics
      and their associated likelihood tables. */
  void AddPath (RonPath path);

  /** Set the estimated likelihood of reaching the destination through each peer. 
      Assumes that we only need to assign random probs once, considered an off-line heuristic.
      Also assumes we should set LH to 0 if we failed to reach a peer.
      Override to change these assumptions (i.e. on-line heuristics),
      but take care to ensure good algorithmic complexity!
  */
  virtual void DoUpdateLikelihoods (Ptr<PeerDestination> destination);
  /** Ensures that DoUpdateLikelihoods is run for each aggregate heuristic */
  void UpdateLikelihoods (Ptr<PeerDestination> destination);

  bool SameRegion (Ptr<RonPeerEntry> peer1, Ptr<RonPeerEntry> peer2);

  /** Returns true if the given path has been attempted, false otherwise.
      Specifying the recordPath argument will add the given path to the set of
      attempted paths if true. */
  bool PathAttempted (Ptr<RonPath> path, bool recordPath=false);

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

  typedef std::list<Ptr<RonPathHeuristic> > AggregateHeuristics;
  AggregateHeuristics m_aggregateHeuristics;

  /** The likelihoods are indexed by the destination and then the path proposed for the destination. */
private:
  struct PathTestEqual
  {
    inline bool operator() (const Ptr<RonPath> path1, const Ptr<RonPath> path2) const
    {
      return *path1 == *path2;
    }
  };

  struct PeerDestinationHasher
  {
    inline size_t operator()(const Ptr<PeerDestination> hop) const
    {
      boost::hash<uint32_t> hasher;
      Ptr<RonPeerEntry> firstPeer = (*hop->Begin ());
      uint32_t id = firstPeer->id;
      return hasher (id);
    }
  };

  //TODO: need to combine the unique IDs of the whole path for a real comparison in multi-hop
  struct PathHasher
  {
    inline size_t operator()(const Ptr<RonPath> path) const
    {
      Ptr<PeerDestination> hop = *path->Begin ();
      PeerDestinationHasher hasher;
      return hasher (hop);
    }
  };

  typedef boost::unordered_set<Ptr<RonPath>, PathHasher, PathTestEqual> PathsAttempted;
  //typedef std::set<Ptr<RonPath>> PathsAttempted;
  PathsAttempted m_pathsAttempted;

  typedef Ptr<RonPath> PathLikelihoodInnerTableKey;
  typedef PathTestEqual PathLikelihoodInnerTableTestEqual;
  typedef PathHasher PathLikelihoodInnerTableHasher;

  //outer table
  
  typedef Ptr<PeerDestination> PathLikelihoodTableKey;
  //compare the pointers by attributes of their contained elements
  struct PathLikelihoodTableTestEqual
  {
    bool operator() (const PathLikelihoodTableKey key1, const PathLikelihoodTableKey key2) const
    {
      return (*key1->Begin ())->id == (*key2->Begin ())->id;
    }
  };

  //hash 
  typedef PeerDestinationHasher PathLikelihoodTableHasher;

  //table definitions

  typedef boost::unordered_map<PathLikelihoodInnerTableKey, double *,
                               PathLikelihoodInnerTableHasher, PathLikelihoodInnerTableTestEqual> PathLikelihoodInnerTable;

  //typedef std::map<RonPath, double *> PathLikelihoodInnerTable;
  typedef boost::unordered_map<PathLikelihoodTableKey, PathLikelihoodInnerTable,
                               PathLikelihoodTableHasher, PathLikelihoodTableTestEqual> PathLikelihoodTable;
  //typedef std::map<Ptr<PeerDestination>, PathLikelihoodInnerTable> PathLikelihoodTable;
  PathLikelihoodTable m_likelihoods;

  /** Only the top-level of a group of aggregate heuristics should manage this. */
  typedef boost::unordered_map<PathLikelihoodInnerTableKey, std::list<double>,
                               PathLikelihoodInnerTableHasher, PathLikelihoodInnerTableTestEqual> MasterPathLikelihoodInnerTable;
  //typedef std::map<RonPath, std::list<double> > MasterPathLikelihoodInnerTable;
  typedef boost::unordered_map<PathLikelihoodTableKey, MasterPathLikelihoodInnerTable,
                               PathLikelihoodTableHasher, PathLikelihoodTableTestEqual> MasterPathLikelihoodTable;
  //typedef std::map<Ptr<PeerDestination>, MasterPathLikelihoodInnerTable> MasterPathLikelihoodTable;
  MasterPathLikelihoodTable * m_masterLikelihoods;

  Ptr<RonPathHeuristic> m_topLevel;







 
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

  // typedef LikelihoodTable::nth_index<0>::type::iterator LikelihoodIterator;
  // typedef LikelihoodTable::nth_index<1>::type::iterator LikelihoodPeerIterator;
  // typedef LikelihoodTable::nth_index<1>::type LikelihoodPeerMap;  

  //boost::function<bool (RonPeerEntry peer1, RonPeerEntry peer2)> GetPeerComparator (Ptr<RonPeerEntry> destination = NULL);
};
} //namespace
#endif //RON_PATH_HEURISTIC_H
