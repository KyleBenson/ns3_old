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
#include "ron-header.h"

#include "ron-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RonClientApplication");
NS_OBJECT_ENSURE_REGISTERED (RonClient);

TypeId
RonClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RonClient")
    .SetParent<Application> ()
    .AddConstructor<RonClient> ()
    .AddAttribute ("OverlayPort", "Port on which we listen for incoming packets from overlay nodes.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&RonClient::m_overPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemotePort", 
                   "The destination port of packets going directly to the server",
                   UintegerValue (9),
                   MakeUintegerAccessor (&RonClient::m_servPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Ipv4Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&RonClient::m_servAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Timeout", "Time to wait for a reply before trying to resend or send along an overlay node.",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&RonClient::m_timeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RonClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&RonClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RonClient::SetDataSize,
                                         &RonClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Send", "A new packet is created and is sent to the server or an overlay node",
                     MakeTraceSourceAccessor (&RonClient::m_sendTrace))
    .AddTraceSource ("Ack", "An ACK packet originating from the server is received",
                     MakeTraceSourceAccessor (&RonClient::m_ackTrace))
    .AddTraceSource ("Forward", "A packet originating from an overlay node is received and forwarded to the server",
                     MakeTraceSourceAccessor (&RonClient::m_forwardTrace))
  ;
  return tid;
}

RonClient::RonClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_servSocket = 0;
  m_overSocket = 0;
  m_data = 0;
  m_dataSize = 0;
}

RonClient::~RonClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_servSocket = 0;
  m_overSocket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void 
RonClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_servAddress = ip;
  m_servPort = port;
}

void
RonClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void 
RonClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_startTime >= m_stopTime)
    return;

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_servSocket == 0)
    {
      m_servSocket = Socket::CreateSocket (GetNode (), tid);
      m_servSocket->Bind ();
      m_servSocket->Connect (InetSocketAddress (m_servAddress, m_servPort));
    }

  if (m_overSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_overSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_overPort);
      m_overSocket->Bind (local);

      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_overSocket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  m_servSocket->SetRecvCallback (MakeCallback (&RonClient::HandleRead, this));
  m_overSocket->SetRecvCallback (MakeCallback (&RonClient::HandleRead, this));

  ScheduleTransmit (Seconds (0.));
}

void 
RonClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_servSocket != 0) 
    {
      m_servSocket->Close ();
      m_servSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_servSocket = 0;

      m_overSocket->Close ();
      m_overSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_overSocket = 0;
    }

  CancelSends ();
}

void
RonClient::CancelSends ()
{
  for (std::list<EventId>::iterator itr = m_sendEvents.begin ();
       itr != m_sendEvents.end (); itr++)
    Simulator::Cancel (*itr);
}

void 
RonClient::SetFill (std::string fill)
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
RonClient::SetFill (uint8_t fill, uint32_t dataSize)
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
RonClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
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
RonClient::SetDataSize (uint32_t dataSize)
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
RonClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void 
RonClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendEvents.push_front (Simulator::Schedule (dt, &RonClient::Send, this));
}

void 
RonClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "RonClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "RonClient::Send(): m_dataSize but no m_data");
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

  // Create a RON header and add it to packet
  RonHeader head (m_servAddress);
  head.SetSeq (m_sent);
  p->AddHeader (head);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_sendTrace (p, GetNode ()->GetId ());
  m_servSocket->Send (p);
  ScheduleTimeout (m_sent++);
  //TODO: modify m_size based on size of the header
  NS_LOG_INFO ("Sent " << m_size << " bytes to " << m_servAddress);

  if (m_sent < m_count) 
    {
      ScheduleTransmit (m_interval);
    }
}

void 
RonClient::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  bool forward = socket == m_overSocket;  //forward this packet thru overlay

  while (packet = socket->RecvFrom (from))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
          NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " << source);

          // If the packet is destined for an overlay node, forward it
          if (forward)
            {
              ForwardPacket (packet, source);
            }

          // Else it's from server, so process the ACK
          else
            {
              ProcessAck (packet, source);
            }
        }
    }
}

void
RonClient::ForwardPacket (Ptr<Packet> packet, Ipv4Address source)
{
  packet->RemoveAllPacketTags ();
  packet->RemoveAllByteTags ();

  // Edit the packet's current RON header and get next hop
  RonHeader head;
  packet->RemoveHeader (head);

  // If this is the first hop on the route, we need to set the source
  // since NS3 doesn't give us a convenient way to access which interface
  // the original packet was sent out on.
  if (head.GetHop () == 0)
    head.SetOrigin (source);

  head.IncrHops ();
  Ipv4Address destination (head.GetNextDest ());
  packet->AddHeader (head);

  NS_LOG_LOGIC ("Forwarding packet");

  m_forwardTrace (packet, GetNode ()-> GetId ());
  m_overSocket->SendTo (packet, 0, destination);
}

void
RonClient::ProcessAck (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("ACK received");

  RonHeader head;
  packet->PeekHeader (head);
  uint32_t seq = head.GetSeq ();
  
  m_ackTrace (packet, GetNode ()-> GetId ());
  m_outstandingSeqs.erase (seq);

  CancelSends ();
  //TODO: store path? send more data?
}

void
RonClient::ScheduleTimeout (uint32_t seq)
{
  Simulator::Schedule (m_timeout, &RonClient::CheckTimeout, this, seq);
  m_outstandingSeqs.insert (seq);
}

void
RonClient::CheckTimeout (uint32_t seq)
{
  std::set<uint32_t>::iterator itr = m_outstandingSeqs.find (seq);

  // If it's still here, we should try a different path to the server
  if (itr != m_outstandingSeqs.end ())
    {
      NS_LOG_INFO ("Packet with seq# " << seq << " timed out.");
      m_outstandingSeqs.erase (itr);
      
      //TODO: send along next link...
      /* keep a list of the overlay nodes and a counter to loop thru them, starting at 0 of course */
    }
}

} // Namespace ns3
