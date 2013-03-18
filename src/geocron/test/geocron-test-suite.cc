/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/geocron-experiment.h"

// An essential include is test.h
#include "ns3/test.h"
#include "ns3/core-module.h"

#include "ns3/point-to-point-layout-module.h"

#include <vector>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

typedef std::vector<Ptr<RonPeerEntry> > PeerContainer;

class GridGenerator
{
public:
  Ipv4AddressHelper rowHelper;
  Ipv4AddressHelper colHelper;

  PeerContainer cachedPeers;
  NodeContainer cachedNodes;

  static GridGenerator GetInstance ()
  {
    static GridGenerator instance = GridGenerator ();
    return instance;
  }
  
  static NodeContainer GetNodes ()
  {
    return GetInstance ().cachedNodes;
  }

  static PeerContainer GetPeers ()
  {
    return GetInstance ().cachedPeers;
  }

  GridGenerator ()
  {
    rowHelper = Ipv4AddressHelper ("10.1.0.0", "255.255.0.0");
    colHelper = Ipv4AddressHelper ("10.128.0.0", "255.255.0.0");

   //first, build a topology for us to use
    uint32_t nrows = 5, ncols = 5;
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    PointToPointGridHelper grid = PointToPointGridHelper (nrows, ncols, pointToPoint);
    grid.BoundingBox (0, 0, 10, 10);
    InternetStackHelper internet;
    grid.InstallStack (internet);
    grid.AssignIpv4Addresses (rowHelper, colHelper);
    rowHelper.NewNetwork ();
    colHelper.NewNetwork ();
  
    cachedNodes.Add (grid.GetNode (0,0));
    cachedNodes.Add (grid.GetNode (1,1));
    cachedNodes.Add (grid.GetNode (4,1));
    cachedNodes.Add (grid.GetNode (0,4)); //should be best ortho path for 0,0 --> 4,4
    cachedNodes.Add (grid.GetNode (3,3));
    cachedNodes.Add (grid.GetNode (4,4));

    NS_ASSERT_MSG (cachedNodes.Get (0)->GetId () != cachedNodes.Get (2)->GetId (), "nodes shouldn't ever have same ID!");

    for (NodeContainer::Iterator itr = cachedNodes.Begin (); itr != cachedNodes.End (); itr++)
      {
        Ptr<RonPeerEntry> newPeer = Create<RonPeerEntry> (*itr);

        //newPeer->id = id++;
        cachedPeers.push_back (newPeer);
      }

    //add all peers to master peer table
    Ptr<RonPeerTable> master = RonPeerTable::GetMaster ();

    for (PeerContainer::iterator itr = cachedPeers.begin ();
         itr != cachedPeers.end (); itr++)
      {
        master->AddPeer (*itr);
      }
  }
};

////////////////////////////////////////////////////////////////////////////////
class TestPeerEntry : public TestCase
{
public:
  TestPeerEntry ();
  virtual ~TestPeerEntry ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};

TestPeerEntry::TestPeerEntry ()
  : TestCase ("Tests basic operations of RonPeerEntry objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestPeerEntry::~TestPeerEntry ()
{
}

void
TestPeerEntry::DoRun (void)
{
  //NS_TEST_ASSERT_MSG_EQ (false, true, "got here");

  //Test equality operator
  bool equality = (*peers[0] == *peers[0]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test same ptr failed");
  
  equality = (*peers[3] == *peers[3]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test same ptr failed");

  equality = (*peers[2] == *(Create<RonPeerEntry> (nodes.Get (2))));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test new ptr failed");
  
  equality = (*peers[3] == *peers[1]);
  NS_TEST_ASSERT_MSG_NE (equality, true, "equality test false positive");

  equality = (peers[3]->address == peers[3]->address);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "equality test address");

  equality = (peers[1]->address == (Ipv4Address)(uint32_t)0);
  NS_TEST_ASSERT_MSG_EQ (equality, false, "equality test address not 0");

  //Test < operator
  equality = *peers[0] < *peers[4];
  NS_TEST_ASSERT_MSG_EQ (equality, true, "< operator test");

  equality = *peers[0] < *peers[0];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "< operator test with self");

  equality = *peers[4] < *peers[1];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "< operator test");
}


////////////////////////////////////////////////////////////////////////////////
class TestPeerTable : public TestCase
{
public:
  TestPeerTable ();
  virtual ~TestPeerTable ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestPeerTable::TestPeerTable ()
  : TestCase ("Test basic operations of PeerTable objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestPeerTable::~TestPeerTable ()
{
}

void
TestPeerTable::DoRun (void)
{
  //Ptr<RonPeerTable> table = RonPeerTable::GetMaster ();
  Ptr<RonPeerTable> table = Create<RonPeerTable> ();
  Ptr<RonPeerTable> table0 = Create<RonPeerTable> ();

  Ptr<RonPeerEntry> peer0 = peers[0];
  table->AddPeer (peer0);
  table->AddPeer (peers[1]);
  table->AddPeer (nodes.Get (3));

  bool equality = *peers[3] == *table->GetPeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer (added as node) by id");

  equality = 3 == table->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "checking table size after adding");

  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[3]->id), true, "testing IsInTable");
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (Create<RonPeerEntry> (nodes.Get (3))->id), true, "testing IsInTable fresh copy by node");
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[2]->id), false, "testing IsInTable false positive");
  //TODO: test by peer entry ptr ref
  //NS_TEST_ASSERT_MSG_EQ (table->IsInTable (peers[2]), false, "testing IsInTable false positive");

  equality = *peers[3] == *table->GetPeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetPeer");

  equality = *table->GetPeerByAddress (peer0->address) == *peer0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer by address");

  equality = *table->GetPeerByAddress (peer0->address) == *peers[1];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "getting peer by address, testing false positive");

  equality = NULL == table->GetPeer (nodes.Get (4)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "table should return NULL when peer not present");

  //test multiple tables with fresh peer entries
  table0->AddPeer (nodes.Get (0));
  table0->AddPeer (nodes.Get (1));

  equality = *table == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing self-equality");

  //add all entries to new table and make sure its == master
  Ptr<RonPeerTable> newTable = Create<RonPeerTable> ();
  Ptr<RonPeerTable> master = RonPeerTable::GetMaster ();
  for (RonPeerTable::Iterator itr = master->Begin ();
       itr != master->End (); itr++)
    {
      newTable->AddPeer (*itr);
    }

  equality = *newTable == *master;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing self-equality GetMaster");

  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonPeerTable equality false positive");

  table0->AddPeer (nodes.Get (3));
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPeerTable equality");

  table->RemovePeer (nodes.Get (3)->GetId ());
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonPeerTable false positive equality after removal");

  table0->RemovePeer (nodes.Get (3)->GetId ());
  equality = *table0 == *table;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPeerTable false positive equality after removal");

  //test iterators
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (table0->Begin ()), true, "testing IsInTable with Iterator");
  
  table->RemovePeer ((*table->Begin ())->id);
  NS_TEST_ASSERT_MSG_EQ (table->IsInTable (table0->Begin ()), false, "testing IsInTable with Iterator false positive after removal");


  /*TODO: test removal
  equality = table->RemovePeer (nodes.Get (4)->GetId ());
  NS_TEST_ASSERT_MSG_NE (equality, true, "remove should only return true if peer existed");

  equality = table->RemovePeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "remove should only return true if peer existed");*/
}


////////////////////////////////////////////////////////////////////////////////
class TestRonPath : public TestCase
{
public:
  TestRonPath ();
  virtual ~TestRonPath ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestRonPath::TestRonPath ()
  : TestCase ("Test basic operations of RonPath objects")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestRonPath::~TestRonPath ()
{
}

void
TestRonPath::DoRun (void)
{
  Ptr<PeerDestination> start = Create<PeerDestination> (peers[0]);
  Ptr<PeerDestination> end = Create<PeerDestination> (peers.back ());

  ////////////////////////////// TEST PeerDestination  ////////////////////

  bool equality = *end == *end;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality same obj");

  equality = *start == *end;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination equality false positive");
  
  equality = *start == *Create<PeerDestination> (peers[3]);
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination equality false positive new copy");
  
  equality = *start == *Create<PeerDestination> (peers.front());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality new copy");
  
  equality = *start == *Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination equality new peer");

  equality = *(*start->Begin ()) == (*(*Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)))->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination Iterator equality new copy");

  equality = *(*end->Begin ()) == (*(*Create<PeerDestination> (peers[0])->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing PeerDestination Iterator false positive equality new copy");

  equality = *(*start->Begin ()) == (*(*Create<PeerDestination> (peers[0])->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing PeerDestination Iterator equality new copy");

  NS_TEST_ASSERT_MSG_EQ (start->GetN (), 1, "testing PeerDestination.GetN");

  //TODO: multi-peer destinations, which requires some ordering or comparing


  ////////////////////////////// TEST PATH  ////////////////////

  Ptr<RonPath> path0 = Create<RonPath> (start);
  Ptr<RonPath> path1 = Create<RonPath> (end);
  Ptr<RonPath> path2 = Create<RonPath> (peers[3]);
  Ptr<RonPath> path3 = Create<RonPath> (Create<RonPeerEntry> (nodes.Get (0)));
  Ptr<RonPath> path4 = Create<RonPath> (peers.back());

  equality = *path0 == *path0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with self");

  equality = *path0 == *Create<RonPath> (start);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, same init");

  equality = *path0 == *Create<RonPath> (peers[0]);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, same init re-retrieved");

  equality = *path4 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, one made by peer");

  equality = *path0 == *path3;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonPath equality with fresh path, also fresh PeerDestination and RonPeerEntry");

  //try accessing elements and testing for equality

  equality = *path0->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent self equality of destination");

  equality = *path0->GetDestination () == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for destination false positive");

  equality = *start == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing equality with Begin and PeerDestination");

  equality = *(Create<PeerDestination> (peers.back ())) == *(*path1->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing equality with Begin and fresh PeerDestination");

  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing false positive equality on separately built paths (start,end)");

  //now add a hop
  path0->AddHop (end);

  Ptr<PeerDestination> oldBegin = (*(path0->Begin ()));
  Ptr<PeerDestination> newBegin, newEnd, oldEnd;

  equality = *oldBegin == *(*path0->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path start after adding a PeerDestination");

  equality = *path0->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent self equality of destination after adding");

  equality = *path0->GetDestination () == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for GetDestination equality of two paths after adding to one");

  equality = *(path1->GetDestination ()) == *(Create<RonPath> (end)->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy before adding");

  equality = *(*path0->Begin ()) == *(*Create<RonPath> (start)->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same start fresh copy after adding");

  Ptr<RonPath> tempPath = Create<RonPath> (start);
  tempPath->AddHop (end);
  equality = *(path0->GetDestination ()) == *(tempPath->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy after adding to both");

  equality = 2 == path0->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path size after adding a PeerDestination");

  equality = 1 == path1->GetN ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for consistent path size of other path");

  oldBegin = *(path1->Begin ());
  newBegin = (path0->GetDestination ());

  equality = (*newBegin == *oldBegin);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using GetDestination");

  equality = (*newBegin == *path1->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using GetDestination");

  equality = (*path0 == *Create<RonPath> (start));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination");

  equality = (*path0 == *Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0)))));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination fresh PeerDestination");

  equality = (*path0 == *Create<RonPath> (Create<PeerDestination> (peers.back ())));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing for PeerDestination false positive equality after adding PeerDestination fresh PeerDestination, checking back");

  oldBegin = *(path1->Begin ());
  RonPath::Iterator pathItr1 = path0->Begin ();
  pathItr1++;
  newBegin = *(pathItr1);

  equality = (*newBegin == *oldBegin);
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing for PeerDestination in one path after added from another using Iterator");

  //paths 0 and 1 should be equal now
  oldEnd = path1->GetDestination ();
  path1->AddHop (start, path1->Begin ());

  equality = *start == *(*path1->Begin ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing expected start after inserting to front of path");

  equality = *oldEnd == *path1->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing destination after inserting to front of path");

  equality = *path0 == *path0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same path now that it's same as another");

  equality = *(path1->GetDestination ()) == *(Create<RonPath> (end)->GetDestination ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest fresh copy");

  equality = *path1->GetDestination () == *Create<RonPath> (peers.back ())->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same dest deeper fresh copy");

  pathItr1 = path0->Begin ();
  RonPath::Iterator pathItr2 = path1->Begin ();
  equality = *(*(pathItr1)) == *(*(pathItr2));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing begin iterator equality separately built paths (start,end)");

  equality = *path1->GetDestination () == *path0->GetDestination ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing same destination now that paths are same");

  equality = *(*(pathItr1)) == *(*(path2->Begin ()));
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing iterator equality false positive separately built paths (start,end)");

  pathItr1++;
  pathItr2++;
  equality = *(*(pathItr1)) == *(*(pathItr2));
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing iterator equality after incrementing separately built paths (start,end)");

  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "separately built paths (start,end)");

  //check larger paths
  path1->AddHop (peers[3]);
  
  equality = *path0 == *path1;
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing proper equality for separately built up path subsets");

  //time to test reverse
  path2->AddHop (end);
  path2->AddHop (start);
  path2->Reverse ();

  equality = *path1 == *path2;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing proper equality for reverse on separately built paths");

  //TODO: check reversing with complex paths
  //TODO: catting paths together
}


////////////////////////////////////////////////////////////////////////////////
class TestRonPathHeuristic : public TestCase
{
public:
  TestRonPathHeuristic ();
  virtual ~TestRonPathHeuristic ();
  NodeContainer nodes;
  PeerContainer peers;

private:
  virtual void DoRun (void);
};


TestRonPathHeuristic::TestRonPathHeuristic ()
  : TestCase ("Test basic operations of RonPathHeuristic objects as well as aggregation")
{
  nodes = GridGenerator::GetNodes ();
  peers = GridGenerator::GetPeers ();
}

TestRonPathHeuristic::~TestRonPathHeuristic ()
{
}

void
TestRonPathHeuristic::DoRun (void)
{
  //RonPathHeuristic has friended us so we can test internals.
  Ptr<RonPathHeuristic> h0 = CreateObject<RonPathHeuristic> ();
  Ptr<PeerDestination> dest = Create<PeerDestination> (peers.back());
  Ptr<RonPath> path = Create<RonPath> (dest);

  h0->SetPeerTable (RonPeerTable::GetMaster ());
  h0->SetSourcePeer (peers.front());

  bool equality = *peers.front () == *h0->GetSourcePeer ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetSourcePeer after setting it");

  //it must be top-level to build data structures properly
  h0->MakeTopLevel ();

  //we need to call GetBestPath before anything else as it will build all the available paths
  h0->BuildPaths (dest);
  NS_TEST_ASSERT_MSG_NE (h0->m_masterLikelihoods->at (dest).size (), 0,
                         "inner master likelihood table empty after BuildPaths");

  NS_TEST_ASSERT_MSG_EQ (h0->m_masterLikelihoods->at (dest).size (), peers.size () - 1,
                         "testing size of inner master likelihood table after BuildPaths");

  NS_TEST_ASSERT_MSG_EQ (h0->PathAttempted (path), false, "testing PathAttempted before GetBestPath");
  path = h0->GetBestPath (dest);
  NS_TEST_ASSERT_MSG_EQ (h0->PathAttempted (path), true, "testing PathAttempted after GetBestPath");

  equality = *path->GetDestination () == *dest;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing GetBestPath with basic path");

  path = Create<RonPath> (dest);
  NS_TEST_ASSERT_MSG_EQ_TOL (h0->GetLikelihood (path), 1.0, 0.01, "testing GetLikelihood before any feedback");

  h0->NotifyTimeout (path, Simulator::Now ());
  NS_TEST_ASSERT_MSG_EQ_TOL (*h0->m_likelihoods[dest][path], 0.0, 0.01, "testing m_likelihoods after timeout feedback");

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path].front (), 0.0, 0.01, "testing m_masterLikelihoods after timeout feedback");

  h0->NotifyAck (path, Simulator::Now ());
  NS_TEST_ASSERT_MSG_EQ_TOL (*h0->m_likelihoods[dest][path], 1.0, 0.01, "testing m_likelihoods after ACK feedback");


  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////  dangerous: testing the inner structs /////////////////////
  ////////////////////////////////////////////////////////////////////////////////
 

  RonPathHeuristic::PathHasher pathHasher;
  NS_TEST_ASSERT_MSG_EQ (pathHasher (path),
                         pathHasher (Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (nodes.GetN () - 1))))),
                         "testing PathHasher new copy");

  NS_TEST_ASSERT_MSG_NE (pathHasher (path),
                         pathHasher (Create<RonPath> (Create<PeerDestination> (Create<RonPeerEntry> (nodes.Get (0))))),
                         "testing PathHasher false positive new copy");

  RonPathHeuristic::MasterPathLikelihoodInnerTable * masterInner = &(*h0->m_masterLikelihoods)[dest];
  
  NS_ASSERT_MSG (masterInner == &(*h0->m_masterLikelihoods)[dest], "why aren't the inner tables in the same location?");

  RonPathHeuristic::Likelihood * lh = &(*masterInner)[path];
  lh->clear ();
  lh->push_front (0.5);

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[dest][path].front (), 0.5,
                             0.01, "testing m_masterLikelihoods accessing with same keys");

  NS_TEST_ASSERT_MSG_EQ_TOL ((*h0->m_masterLikelihoods)[Create<PeerDestination> (peers.back())][path].back (), 0.5,
                             0.01, "testing m_masterLikelihoods accessing with fresh copies of keys");
  //path->AddHop (
}


////////////////////////////////////////////////////////////////////////////////
class TestRonHeader : public TestCase
{
public:
  TestRonHeader ();
  virtual ~TestRonHeader ();
  NodeContainer nodes;
  Ptr<RonPeerTable> peers;

private:
  virtual void DoRun (void);
};


TestRonHeader::TestRonHeader ()
  : TestCase ("Test basic operations of RonHeader objects")
{
  nodes = GridGenerator::GetNodes ();

  //we want to work with a table rather than lists
  //PeerContainer peersList = GridGenerator::GetPeers ();
  peers = Create<RonPeerTable> ();

  //for (PeerContainer::iterator itr = peersList.begin ();
  for (NodeContainer::Iterator itr = nodes.Begin ();
       itr != nodes.End (); itr++)
    {
      NS_ASSERT_MSG (!peers->IsInTable ((*itr)->GetId ()), "this peer (" << (*itr)->GetId () << ") shouldn't be in the table yet!");

      peers->AddPeer (*itr);

      NS_ASSERT_MSG (peers->IsInTable ((*itr)->GetId ()), "peers should be in table now");
    }
}

TestRonHeader::~TestRonHeader ()
{
}

void
TestRonHeader::DoRun (void)
{
  NS_ASSERT_MSG (peers->GetN () == nodes.GetN (), "table size != node container size");

  Ptr<Packet> packet = Create<Packet> ();
  Ipv4Address addr0 = peers->GetPeer (nodes.Get (0)->GetId ())->address;

  uint32_t tmpId = nodes.Get (1)->GetId ();
  NS_TEST_ASSERT_MSG_EQ (peers->IsInTable (tmpId), true, "table should contain this peer");
  Ipv4Address addr1 = peers->GetPeer (tmpId)->address;
  Ipv4Address addr2 = peers->GetPeer (nodes.Get (2)->GetId ())->address;
  Ipv4Address srcAddr = Ipv4Address ("244.244.244.244");

  Ptr<RonHeader> head0 = Create<RonHeader> ();
  head0->SetDestination (addr0);
  Ptr<RonHeader> head1 = Create<RonHeader> (addr1);
  Ptr<RonHeader> head2 = Create<RonHeader> (addr2, addr1);

  //test destinations based on constructors

  bool equality = head1->GetFinalDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest with 2 diff constructors.");

  NS_TEST_ASSERT_MSG_EQ (head2->IsForward (), true, "testing IsForward");
  NS_TEST_ASSERT_MSG_EQ (head0->IsForward (), false, "testing IsForward false positive");

  equality = head1->GetFinalDest () == head1->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest should be same.");

  equality = head2->GetFinalDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonHeader GetFinalDest and GetNextDest shouldn't be same.");

  equality = head0->GetFinalDest () == Create<RonHeader> (addr0)->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing RonHeader GetFinalDest and GetNextDest with 1 constructor and SetDestination, fresh header.");

  equality = head0->GetFinalDest () == Create<RonHeader> (addr1)->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing RonHeader GetFinalDest and GetNextDest false positive equality with 1 constructor and SetDestination, fresh header.");

  //test seq#
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), 0, "testing default seq#");
  head0->SetSeq (5);
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), 5, "testing seq# after setting it");

  //test origin
  
  head2->SetOrigin (srcAddr);
  NS_TEST_ASSERT_MSG_EQ (head2->GetOrigin (), srcAddr, "testing proper origin after setting it");

  //test operator=
  RonHeader tmpHead = *head2;
  NS_TEST_ASSERT_MSG_EQ (head2->GetFinalDest (), tmpHead.GetFinalDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), tmpHead.GetNextDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetOrigin (), tmpHead.GetOrigin (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head2->GetSeq (), tmpHead.GetSeq (), "testing assignment operator");

  //test reverse
  tmpHead = *head2;
  head2->ReversePath ();
  Ipv4Address prevHop = head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (head2->GetFinalDest (), srcAddr, "testing proper origin after reversing");
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), prevHop, "testing proper next hop after reversing");

  //undo reverse
  head2->ReversePath ();
  equality = tmpHead.GetFinalDest () == head2->GetFinalDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");
  equality = tmpHead.GetNextDest () == head2->GetNextDest ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");
  equality = tmpHead.GetOrigin () == head2->GetOrigin ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "header should be back to normal");

  //test hop incrementing
  prevHop = head2->GetFinalDest ();
  head2->IncrHops ();
  NS_TEST_ASSERT_MSG_EQ (head2->GetNextDest (), prevHop, "testing proper next hop after incrementing hops");

  //test path manipulation/interface

  Ptr<PeerDestination> dest = Create<PeerDestination> (peers->GetPeerByAddress (addr2));
  Ptr<RonPath> path = Create<RonPath> (dest);
  path->AddHop (Create<PeerDestination> (peers->GetPeerByAddress (addr1)), path->Begin ());

  //path should now be the same as returned by head2
  //path: scrAddr -> addr1 -> addr2

  NS_TEST_ASSERT_MSG_EQ (head2->GetPath ()->GetN (), 2, "path pulled from header isn't right length");

  equality = *head2->GetPath () == *path;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "Checking GetPath equality");

  equality = *head0->GetPath () == *head2->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "Checking GetPath inequality");

  head0->SetPath (path);
  equality = *head0->GetPath () == *head2->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing path equality after SetPath");

  //RonHeader currently doesn't provide access to inserting hops at arbitrary locations,
  //so we reverse, add at end, and reverse again to insert at front (after source)
  head0->ReversePath ();
  head0->AddDest (addr0);
  head0->ReversePath ();
  path->AddHop (peers->GetPeerByAddress (addr0), path->Begin ());
  equality = *path == *head0->GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "Checking path equality after modifying both path and header");

  //TODO: test iterators

  //test serializing
  packet->AddHeader (*head0);
  packet->RemoveHeader (tmpHead);

  NS_TEST_ASSERT_MSG_EQ (head0->GetFinalDest (), tmpHead.GetFinalDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetNextDest (), tmpHead.GetNextDest (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetOrigin (), tmpHead.GetOrigin (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetSeq (), tmpHead.GetSeq (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->GetHop (), tmpHead.GetHop (), "testing assignment operator");
  NS_TEST_ASSERT_MSG_EQ (head0->IsForward (), tmpHead.IsForward (), "testing assignment operator");

  equality = *head0->GetPath () == *tmpHead.GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, true, "testing deserialized packet path");

  head0->ReversePath ();
  equality = *head0->GetPath () == *tmpHead.GetPath ();
  NS_TEST_ASSERT_MSG_EQ (equality, false, "testing deserialized packet path false positive after reverse");
}


////////////////////////////////////////////////////////////////////////////////
////////////////////$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$////////////////////
//////////$$$$$$$$$$   End of test cases - create test suite $$$$$$$$$$/////////
////////////////////$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$////////////////////
////////////////////////////////////////////////////////////////////////////////


// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class GeocronTestSuite : public TestSuite
{
public:
  GeocronTestSuite ();
};

GeocronTestSuite::GeocronTestSuite ()
  : TestSuite ("geocron", UNIT)
{
  //basic data structs
  AddTestCase (new TestPeerEntry);
  AddTestCase (new TestPeerTable);
  AddTestCase (new TestRonPath);
  AddTestCase (new TestRonPathHeuristic);

  //network application stuff
  AddTestCase (new TestRonHeader);
}

// Do not forget to allocate an instance of this TestSuite
static GeocronTestSuite geocronTestSuite;

