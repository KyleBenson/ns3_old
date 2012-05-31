/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// Include a header file from your module to test.
#include "ns3/location.h"
#include "ns3/core-module.h"
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
  LocationTestCase (Ptr<Location> a, Ptr<Location> b,std::string name);
  virtual ~LocationTestCase ();

private:
  virtual void DoRun (void);
  Ptr<Location> la;
  Ptr<Location> lb;
};

// Add some help text to this case to describe what it is intended to test
LocationTestCase::LocationTestCase (Ptr<Location> a, Ptr<Location> b,std::string name)
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
  Ptr<Location> a = CreateObject<Location>(40.123456789,-90.0987654321);
  Ptr<Location> b = CreateObject<Location>(40.123456123,-90.0987651234);
  AddTestCase (new LocationTestCase(a,b,"Points equal within tolerance (doubles)"));

  Ptr<Location> c = CreateObject<Location>((float)39.8395659871,(float)169.6547461661674657);
  Ptr<Location> d = CreateObject<Location>((float)39.839565056123,(float)169.654746987651234);
  AddTestCase (new LocationTestCase(c,d,"Points equal within tolerance (floats)"));

  Ptr<Location> e = CreateObject<Location>(39546389,-16625885);
  Ptr<Location> f = CreateObject<Location>(39546389,-16625885);
  AddTestCase (new LocationTestCase(e,f,"Points equal (ints)"));

  std::list<Location> locs;
  locs.push_front(*a);
  locs.push_front(*c);
  locs.push_front(*e);
  Ptr<Location> comp_cent = Location::centroid(locs.begin(),locs.end());
  Ptr<Location> real_cent = CreateObject<Location>(39.836470533,20.976698667);
  AddTestCase (new LocationTestCase(comp_cent,real_cent,"Centroid works"));
}

// Do not forget to allocate an instance of this TestSuite
static LocationTestSuite locationTestSuite;

