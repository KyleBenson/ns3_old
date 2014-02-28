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
#include "mdc-event-sensor.h"
#include "mdc-collector.h"
#include "mdc-utilities.h"

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"

#include <iterator>
#include <set>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("MdcHelper");

namespace ns3 {

  MdcCollectorHelper::MdcCollectorHelper (uint16_t port /* = 9999*/)
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


MdcEventSensorHelper::MdcEventSensorHelper (Ipv4Address address, int nEvents /* = 0*/, uint16_t port /* = 9999*/)
{
  m_factory.SetTypeId (MdcEventSensor::GetTypeId ());
  m_nEvents = nEvents;

//  m_eventTimeRandomVariable = new UniformVariable (2.0, 10.0);
//  m_radiusRandomVariable = new ConstantVariable (5.0);
//  m_posAllocator = CreateObject<RandomRectanglePositionAllocator> ();

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
MdcEventSensorHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcEventSensorHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcEventSensorHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MdcEventSensorHelper::InstallPriv (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Application> app = m_factory.Create<MdcEventSensor> ();

  node->AddApplication (app);
  app->SetNode (node);

  ScheduleEvents (app);

  return app;
}

void
MdcEventSensorHelper::ScheduleEvents (Ptr<Application> app)
{
	for (std::list<SensedEvent>::iterator itr = m_ptrToEventList->begin (); itr != m_ptrToEventList->end (); itr++)
	{
		DynamicCast<MdcEventSensor>(app)->ScheduleEventDetection (itr->GetTime (), *itr);
	}
}


void
MdcEventSensorHelper::SetEventListReference (std::list<SensedEvent>* ptrToEventList)
{
  m_ptrToEventList = ptrToEventList;
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
////////////////////   MDC Packet Sink     ///////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


MdcSinkHelper::MdcSinkHelper ()//Address sensorAddress, Address mdcAddress)
{
  m_factory.SetTypeId ("ns3::MdcSink");
  Address local = InetSocketAddress (Ipv4Address::GetAny (), 9999);
  m_factory.Set ("SensorLocal", AddressValue (local));
  m_factory.Set ("MdcLocal", AddressValue (local));
}

void 
MdcSinkHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
MdcSinkHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcSinkHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MdcSinkHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MdcSinkHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3

