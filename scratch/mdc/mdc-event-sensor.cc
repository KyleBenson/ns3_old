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

#include "mdc-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MdcEventSensorApplication");
NS_OBJECT_ENSURE_REGISTERED (MdcEventSensor);

TypeId
MdcEventSensor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MdcEventSensor")
    .SetParent<Application> ()
    .AddConstructor<MdcEventSensor> ()
    .AddAttribute ("Port", "Port on which we send packets to sink nodes or MDCs.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&MdcEventSensor::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Ipv4Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&MdcEventSensor::m_servAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Timeout", "Time to wait for a reply before trying to resend.",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&MdcEventSensor::m_timeout),
                   MakeTimeChecker ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&MdcEventSensor::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MdcEventSensor::SetDataSize,
                                         &MdcEventSensor::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MdcEventSensor::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Send", "A new packet is created and is sent to the sink or an MDC",
                     MakeTraceSourceAccessor (&MdcEventSensor::m_sendTrace))
    .AddTraceSource ("Ack", "An ACK packet originating from the server is received",
                     MakeTraceSourceAccessor (&MdcEventSensor::m_ackTrace))
    .AddTraceSource ("Forward", "A packet originating from an overlay node is received and forwarded to the server",
                     MakeTraceSourceAccessor (&MdcEventSensor::m_forwardTrace))
  ;
  return tid;
}

MdcEventSensor::MdcClient ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_sent = 0;
  m_socket = 0;
  m_data = 0;
  m_dataSize = 0;
  m_nextPeer = 0;

  m_address = Ipv4Address((uint32_t)0);
  m_peers = std::vector<Ipv4Address>();
}

MdcEventSensor::~MdcClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;

  delete [] m_data;

  m_data = 0;
  m_dataSize = 0;
}

void 
MdcEventSensor::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_servAddress = ip;
  //m_port = port; //should already be set
}

void
MdcEventSensor::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void 
MdcEventSensor::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_startTime >= m_stopTime)
    {
      NS_LOG_LOGIC ("Cancelling application (start > stop)");
      //DoDispose ();
      return;
    }

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);

      NS_LOG_LOGIC ("Socket bound");
    }

  // Use the address of the first non-loopback device on the node for our address
  if (m_address.Get () == 0)
    {
      m_address = GetNode ()->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    }

  m_socket->SetRecvCallback (MakeCallback (&MdcEventSensor::HandleRead, this));

  if (m_sent < m_count)
    ScheduleTransmit (Seconds (0.));
}

void 
MdcEventSensor::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  CancelEvents ();
}

void
MdcEventSensor::CancelEvents ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::list<EventId>::iterator itr = m_events.begin ();
       itr != m_events.end (); itr++)
    Simulator::Cancel (*itr);
}

void 
MdcEventSensor::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
MdcEventSensor::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
MdcEventSensor::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
MdcEventSensor::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
MdcEventSensor::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void 
MdcEventSensor::ScheduleTransmit (Time dt, bool noData /*= false*/)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_events.push_front (Simulator::Schedule (dt, &MdcEventSensor::Send, this, viaOverlay));
}

void 
MdcEventSensor::Send (bool noData)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_LOGIC ("Sending " << (noData ? "data notify" : "full data") << " packet.");

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "MdcEventSensor::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "MdcEventSensor::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care 
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In 
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }

  // add MDC header to packet
  MdcHeader head;
  //head.SetSeq (m_sent);
  head.SetOrigin (m_address);
  head.SetDest (m_servAddress);

  Ptr<Node> node = GetNode ();
  mobility stuff
  head.SetPosition (xPos, mPos);

  // If only notifying of an event and not sending the full data,
  // don't add fill and set the data size to be 0.
  if (noData)
    {
      head.SetData (0);
    }
  else
    {
      head.SetData (p->GetSize ());
    }

  p->AddHeader (head);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_sendTrace (p, GetNode ()->GetId ());
  m_socket->SendTo (p, 0, InetSocketAddress(head.GetNextDest (), m_port));
  //ScheduleTimeout (m_sent++);
}

void 
MdcEventSensor::HandleRead (Ptr<Socket> socket)
{
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

          // If the packet is for us, process the ACK
          if (head.GetFinalDest () == head.GetNextDest ())
            {
              ProcessAck (packet, source);
            }

          // Else forward it
          else
            {
              ForwardPacket (packet, source);
            }
        }
    }
}

void
MdcEventSensor::ForwardPacket (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("Forwarding packet");

  packet->RemoveAllPacketTags ();
  packet->RemoveAllByteTags ();

  // Edit the packet's current RON header and get next hop
  MdcHeader head;
  packet->RemoveHeader (head);

  // If this is the first hop on the route, we need to set the source
  // since NS3 doesn't give us a convenient way to access which interface
  // the original packet was sent out on.
  if (head.GetHop () == 0)
    head.SetOrigin (source);

  head.IncrHops ();
  Ipv4Address destination = head.GetNextDest ();
  packet->AddHeader (head);

  m_forwardTrace (packet, GetNode ()-> GetId ());
  m_socket->SendTo (packet, 0, InetSocketAddress(destination, m_port));
}

void
MdcEventSensor::ProcessAck (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("ACK received");

  MdcHeader head;
  packet->PeekHeader (head);
  uint32_t seq = head.GetSeq ();
  
  m_ackTrace (packet, GetNode ()-> GetId ());
  m_outstandingSeqs.erase (seq);
  //TODO: handle an ack from an old seq number

  CancelEvents ();
  //TODO: store path? send more data?
}

void
MdcEventSensor::ScheduleTimeout (uint32_t seq)
{
  m_events.push_front (Simulator::Schedule (m_timeout, &MdcEventSensor::CheckTimeout, this, seq));
  m_outstandingSeqs.insert (seq);
}

void
MdcEventSensor::AddPeer (Ipv4Address addr)
{
  if (addr != m_address)
    m_peers.push_back (addr);
}

void
MdcEventSensor::SetPeerList (std::vector<Ipv4Address> peers)
{
  m_peers = peers;
}

Ipv4Address
MdcEventSensor::GetAddress () const
{
  return m_address;
}

void
MdcEventSensor::CheckTimeout (uint32_t seq)
{
  std::set<uint32_t>::iterator itr = m_outstandingSeqs.find (seq);

  // If it's timed out, we should try a different path to the server
  if (itr != m_outstandingSeqs.end ())
    {
      NS_LOG_LOGIC ("Packet with seq# " << seq << " timed out.");
      m_outstandingSeqs.erase (itr);
      
      if (m_sent < m_count)
        ScheduleTransmit (Seconds (0.0), true);
    }
}

} // Namespace ns3
