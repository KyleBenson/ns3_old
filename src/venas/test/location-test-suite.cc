/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/location.h"
#include <list>

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// Tolerance for float values to be off by (a microdegree)
#define TOLERANCE 1e-6

// This is an example TestCase.
class LocationTestCase : public TestCase
{
public:
  LocationTestCase (Location * a, Location * b, std::string name);
  virtual ~LocationTestCase ();

private:
  virtual void DoRun (void);
  Location *la;
  Location *lb;
};

// Add some help text to this case to describe what it is intended to test
LocationTestCase::LocationTestCase (Location *a, Location *b,std::string name)
  : TestCase (name), la(a), lb(b) {}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
LocationTestCase::~LocationTestCase ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
LocationTestCase::DoRun (void)
{
  // A wide variety of test macros are available in src/core/test.h

  // These tests simply check if the specified locations are the same, giving
  // some tolerance in the float case.
  NS_TEST_ASSERT_MSG_EQ (la->getLatitudeMicro(), lb->getLatitudeMicro(), "Latitude microdegrees are not equal");
  NS_TEST_ASSERT_MSG_EQ (la->getLongitudeMicro(), lb->getLongitudeMicro(), "Longitude microdegrees are not equal");
  NS_TEST_ASSERT_MSG_EQ_TOL (la->getLatitude(), lb->getLatitude(), TOLERANCE, "Float degrees are not equal within tolerance");
  NS_TEST_ASSERT_MSG_EQ_TOL (la->getLongitude(), lb->getLongitude(), TOLERANCE, "Float degrees are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class LocationTestSuite : public TestSuite
{
public:
  LocationTestSuite ();
};

LocationTestSuite::LocationTestSuite ()
  : TestSuite ("location", UNIT)
{
  Location *a = new Location(40.123456789,-90.0987654321);
  Location *b = new Location(40.123456123,-90.0987651234);
  AddTestCase (new LocationTestCase(a,b,"Points equal within tolerance (doubles)"));

  Location *c = new Location((float)39.8395659871,(float)169.6547461661674657);
  Location *d = new Location((float)39.839565056123,(float)169.654746987651234);
  AddTestCase (new LocationTestCase(c,d,"Points equal within tolerance (floats)"));

  Location *e = new Location(39546389,-16625885);
  Location *f = new Location(39546389,-16625885);
  AddTestCase (new LocationTestCase(e,f,"Points equal (ints)"));

  std::list<Location> locs;
  locs.push_front(*a);
  locs.push_front(*c);
  locs.push_front(*e);
  Location *comp_cent = Location::centroid(locs.begin(),locs.end());
  Location *real_cent = new Location(39.836470533,20.976698667);
  AddTestCase (new LocationTestCase(comp_cent,real_cent,"Centroid works"));
}

// Do not forget to allocate an instance of this TestSuite
static LocationTestSuite locationTestSuite;

