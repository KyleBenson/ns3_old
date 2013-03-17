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
    cachedNodes.Add (grid.GetNode (0,4));
    cachedNodes.Add (grid.GetNode (3,3));
    cachedNodes.Add (grid.GetNode (4,4));

    for (NodeContainer::Iterator itr = cachedNodes.Begin (); itr != cachedNodes.End (); itr++)
      {
        Ptr<RonPeerEntry> newPeer = Create<RonPeerEntry> (*itr);
        //newPeer->id = id++;
        cachedPeers.push_back (newPeer);
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
  : TestCase ("Base test for others: tests basic operations of RonPeerEntry objects")
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
  : TestCase ("Test basic operations of RonPeerEntry objects")
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
  Ptr<RonPeerTable> table = Create<RonPeerTable> ();

  Ptr<RonPeerEntry> peer0 = peers[0];
  table->AddPeer (peer0);
  table->AddPeer (peers[1]);
  table->AddPeer (nodes.Get (3));

  bool equality = *peers[3] == *table->GetPeer (nodes.Get (3)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer (added as node) by id");

  equality = *table->GetPeerByAddress (peer0->address) == *peer0;
  NS_TEST_ASSERT_MSG_EQ (equality, true, "getting peer by address");

  equality = *table->GetPeerByAddress (peer0->address) == *peers[1];
  NS_TEST_ASSERT_MSG_EQ (equality, false, "getting peer by address, testing false positive");

  equality = NULL == table->GetPeer (nodes.Get (4)->GetId ());
  NS_TEST_ASSERT_MSG_EQ (equality, true, "table should return NULL when peer not present");

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

private:
  virtual void DoRun (void);
};


TestRonPath::TestRonPath ()
  : TestCase ("Test basic operations of RonPeerEntry objects")
{
}

TestRonPath::~TestRonPath ()
{
}

void
TestRonPath::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}


////////////////////////////////////////////////////////////////////////////////
class TestRonPathHeuristic : public TestCase
{
public:
  TestRonPathHeuristic ();
  virtual ~TestRonPathHeuristic ();

private:
  virtual void DoRun (void);
};


TestRonPathHeuristic::TestRonPathHeuristic ()
  : TestCase ("Test basic operations of RonPeerEntry objects")
{
}

TestRonPathHeuristic::~TestRonPathHeuristic ()
{
}

void
TestRonPathHeuristic::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}


////////////////////////////////////////////////////////////////////////////////
class TestRonHeader : public TestCase
{
public:
  TestRonHeader ();
  virtual ~TestRonHeader ();

private:
  virtual void DoRun (void);
};


TestRonHeader::TestRonHeader ()
  : TestCase ("Test basic operations of RonPeerEntry objects")
{
}

TestRonHeader::~TestRonHeader ()
{
}

void
TestRonHeader::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
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

