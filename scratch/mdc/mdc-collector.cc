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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4.h"
#include "ns3/vector.h"
#include "ns3/mobility-model.h"

#include "mdc-collector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MdcCollectorApplication");
NS_OBJECT_ENSURE_REGISTERED (MdcCollector);

TypeId
MdcCollector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MdcCollector")
    .SetParent<Application> ()
    .AddConstructor<MdcCollector> ()
    .AddAttribute ("Port", "Port on which we send packets to sink nodes.",
                   UintegerValue (9999),
                   MakeUintegerAccessor (&MdcCollector::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Ipv4Address of the sink",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&MdcCollector::m_sinkAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Interval", 
                   "The time to wait between data request packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&MdcCollector::m_interval),
                   MakeTimeChecker ())
    .AddTraceSource ("Request", "The MDC sends a data request beacon to nearby sensor nodes",
                     MakeTraceSourceAccessor (&MdcCollector::m_requestTrace))
    .AddTraceSource ("Forward", "A packet originating from a sensor node is received and forwarded to the sink",
                     MakeTraceSourceAccessor (&MdcCollector::m_forwardTrace))
  ;
  return tid;
}

MdcCollector::MdcCollector ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_sinkSocket = 0;
  m_sensorSocket = 0;
  m_address = Ipv4Address((uint32_t)0);
}

void 
MdcCollector::SetSink (Ipv4Address ip, uint16_t port /* = 0*/)
{
  m_sinkAddress = ip;

  if (port)
    m_port = port; //may already be set
}

void
MdcCollector::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void 
MdcCollector::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_startTime >= m_stopTime)
    {
      NS_LOG_LOGIC ("Cancelling application (start > stop)");
      //DoDispose ();
      return;
    }

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_sensorSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_sensorSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_sensorSocket->Bind (local);

      NS_LOG_LOGIC ("Socket bound");
    }

  if (m_sinkSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_sinkSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (GetAddress (), m_port);
      m_sinkSocket->Bind (local);

      NS_LOG_LOGIC ("Socket bound");
    }

  // Use the first address of the first non-loopback device on the node for our address
  if (m_address.Get () == 0)
    {
      Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
      Ipv4Address loopback = Ipv4Address::GetLoopback ();
      
      for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++)
        {
          Ipv4Address addr = ipv4->GetAddress (i,0).GetLocal ();
          if (addr != loopback)
            m_address = addr;
        }
    }

  m_sensorSocket->SetRecvCallback (MakeCallback (&MdcCollector::HandleRead, this));
  //TODO: callback for sinkSocket?

  ScheduleTransmit (Seconds (0.));
}

void 
MdcCollector::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_sensorSocket != 0) 
    {
      m_sensorSocket->Close ();
      m_sensorSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_sensorSocket = 0;
    }

  if (m_sinkSocket != 0) 
    {
      m_sinkSocket->Close ();
      m_sinkSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_sinkSocket = 0;
    }

  CancelEvents ();
}

void
MdcCollector::CancelEvents ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::list<EventId>::iterator itr = m_events.begin ();
       itr != m_events.end (); itr++)
    Simulator::Cancel (*itr);
}

void 
MdcCollector::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_events.push_front (Simulator::Schedule (dt, &MdcCollector::Send, this));
}

void 
MdcCollector::Send ()
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_LOGIC ("Sending MDC data request packet.");

  Ptr<Packet> p = Create<Packet> ();

  // add header to packet
  MdcHeader head(Ipv4Address::GetBroadcast ());
  head.SetOrigin (m_address);
  
  Vector2D pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
  head.SetPosition (pos.x, pos.y);

  p->AddHeader (head);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_sendTrace (p, GetNode ()->GetId ());
  m_sensorSocket->SendTo (p, 0, InetSocketAddress(head.GetDest (), m_port));
}

void 
MdcCollector::HandleRead (Ptr<Socket> socket)
{

  //TODO: use TCP instead! immediately ACK packets from sensors then use TCP to handle reliably getting the to the sink

  Ptr<Packet> packet;
  Address from;

  while (packet = socket->RecvFrom (from))
    {
      NS_LOG_LOGIC ("Reading packet from socket.");

      if (InetSocketAddress::IsMatchingType (from))
        {
          Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

          MdcHeader head;
          packet->PeekHeader (head);

          // If the packet is for the sink, forward it
          if (head.GetDest () == m_sinkAddress)
            {
              ForwardPacket (packet, source);
            }

          // Else its for us
          else
            {
              return; //TODO: handle incoming routing schedule
            }
        }
    }
}

void
MdcCollector::ForwardPacket (Ptr<Packet> packet)
{
  NS_LOG_LOGIC ("Forwarding packet");

  packet->RemoveAllPacketTags ();
  packet->RemoveAllByteTags ();

  MdcHeader head;
  packet->PeekHeader (head);

  Ipv4Address destination = head.GetDest ();

  m_forwardTrace (packet, GetNode ()-> GetId ());
  m_sinkSocket->SendTo (packet, 0, InetSocketAddress(destination, m_port));
}

Ipv4Address
MdcCollector::GetAddress () const
{
  return m_address;
}

} // Namespace ns3
