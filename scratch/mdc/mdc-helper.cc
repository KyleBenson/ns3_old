/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "mdc-helper.h"
#include "mdc-server.h"
#include "mdc-client.h"

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/log.h"

#include <iterator>

NS_LOG_COMPONENT_DEFINE ("MdcEventSensorServerHelper");

namespace ns3 {

MdcCollectorHelper::MdcCollectorHelper (uint16_t port)
{
  m_factory.SetTypeId (MdcCollector::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
MdcCollectorHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
MdcCollectorHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcCollectorHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcCollectorHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MdcCollectorHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Application> app = m_factory.Create<MdcCollector> ();
  node->AddApplication (app);
  app->SetNode (node);

  return app;
}


  //////********************************************//////////////
  //////********************************************//////////////
  //////******************  SENSOR  ****************//////////////
  //////********************************************//////////////
  //////********************************************//////////////


MdcEventSensorHelper::MdcEventSensorHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (MdcEventSensor::GetTypeId ());
  SetAttribute ("RemoteAddress", Ipv4AddressValue (address));
  SetAttribute ("Port", UintegerValue (port));
}

void 
MdcEventSensorHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
MdcEventSensorHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<MdcEventSensor>()->SetFill (fill);
}

void
MdcEventSensorHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<MdcEventSensor>()->SetFill (fill, dataLength);
}

void
MdcEventSensorHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<MdcEventSensor>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
MdcEventSensorHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcEventSensorHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcEventSensorHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MdcEventSensorHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Application> app = m_factory.Create<MdcEventSensor> ();

  node->AddApplication (app);
  app->SetNode (node);

  return app;
}

template<class ForwardIter>
void
MdcEventSensorHelper::AddPeers (ForwardIter peersBegin, ForwardIter peersEnd, ApplicationContainer apps)
{
  while (peersBegin != peersEnd)
    {
      for (ApplicationContainer::Iterator app = apps.Begin (); app != apps.End (); app++)
        {
          Ptr<MdcEventSensor> rca = DynamicCast<MdcEventSensor> (*app);
          rca->AddPeer (*peersBegin);
        }
      peersBegin++;
    }
}

} // namespace ns3

/* working with packet headers
int main (int argc, char *argv[])
{
  // instantiate a header.
  MdcHeader sourceHeader;
  sourceHeader.SetData (2);

  // instantiate a packet
  Ptr<Packet> p = Create<Packet> ();

  // and store my header into the packet.
  p->AddHeader (sourceHeader);

  // print the content of my packet on the standard output.
  p->Print (std::cout);
  std::cout << std::endl;

  // you can now remove the header from the packet:
  MdcHeader destinationHeader;
  p->RemoveHeader (destinationHeader);

  // and check that the destination and source
  // headers contain the same values.
  NS_ASSERT (sourceHeader.GetData () == destinationHeader.GetData ());

  return 0;
}
*/
