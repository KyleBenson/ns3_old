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
#include "ns3/mobility-module.h"
#include "ns3/random-variable.h"

#include "mdc-header.h"
#include <climits>
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
  m_udpSensorSocket = 0;
  m_tcpSensorSocket = 0;
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

void MdcCollector::DoConnect (Address addr)
{
  m_sinkSocket->Connect (addr);
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
  
  // Use the first address of the first non-loopback device on the node for our address
  // Use the second as the bound device for the long-range TCP network
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
  Ipv4Address loopback = Ipv4Address::GetLoopback ();
  /*Ipv4Address tcpAddr;
  Ptr<NetDevice> netDevice = NULL;
  bool loopbackFound = false;*/

  //TODO: instead figure out the bound net device and address based on the network the sink belongs to

  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = ipv4->GetAddress (i,0).GetLocal ();
      if (addr != loopback)
        {
          if (m_address.Get () == 0)
            {
              m_address = addr;
            }
        }
      /*else
        {
          loopbackFound = true;
          continue;
        }
      
      if ((loopbackFound && i == 2) || (!loopbackFound && i == 1))
        {
          NS_LOG_LOGIC ("Binding TCP socket to device with IP address: " << ipv4->GetAddress (i, 0).GetLocal ());
          netDevice = ipv4->GetNetDevice (i);
          tcpAddr = ipv4->GetAddress (i, 0).GetLocal ();p
          break;
          }*/
    }
  
  NS_LOG_LOGIC ("MDC address set to " << GetAddress ());

  UniformVariable randomStartDelay (0.0, 1.0);

  //TODO: bind sockets only to the network we talk on
  if (m_udpSensorSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_udpSensorSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (GetAddress (), m_port); //Ipv4Address::GetAny ()
      m_udpSensorSocket->Bind (local);

      NS_ASSERT (m_udpSensorSocket->SetAllowBroadcast (true));
      }
  
  if (m_sinkSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_sinkSocket = Socket::CreateSocket (GetNode (), tid);

      //NS_LOG_INFO ("sink IP address: " << m_sinkAddress << ", port: " << m_port);
      InetSocketAddress addr = InetSocketAddress (m_sinkAddress, m_port);

      NS_ASSERT (m_sinkSocket->Bind () >= 0);
      //m_sinkSocket->Connect (addr);
      Simulator::Schedule (Seconds (randomStartDelay.GetValue ()), &MdcCollector::DoConnect, this, addr);
    }

  if (m_tcpSensorSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_tcpSensorSocket = Socket::CreateSocket (GetNode (), tid);

      //NS_LOG_INFO ("sink IP address: " << m_sinkAddress << ", port: " << m_port);
      m_tcpSensorSocket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                                            MakeCallback (&MdcCollector::HandleAccept, this));
      
      NS_ASSERT (m_tcpSensorSocket->Bind (InetSocketAddress (GetAddress (), m_port)) >= 0);
      NS_ASSERT (m_tcpSensorSocket->Listen () >= 0);
    }

  m_sinkSocket->SetRecvCallback (MakeCallback (&MdcCollector::HandleRead, this));

  ScheduleTransmit (Seconds (randomStartDelay.GetValue ()));
}

void 
MdcCollector::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_udpSensorSocket != 0) 
    {
      m_udpSensorSocket->Close ();
      m_udpSensorSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_udpSensorSocket = 0;
    }

  if (m_tcpSensorSocket != 0) 
    {
      m_tcpSensorSocket->Close ();
      m_tcpSensorSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_tcpSensorSocket = 0;
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

/*
 * This is the one of the two 'sending' functions of the MDC. Note that the other is 'Forward' below.
 * MDC sends either the broadcast beacon or the messages collected from all the sensors destined to the sink.
 * This function handles the broadcast beacon part.
 */
void 
MdcCollector::Send ()
{
  NS_LOG_FUNCTION("COLL Node#" << GetNode ()->GetId () << " sending data request beacon.");

  Ptr<Packet> p = Create<Packet> ();

  // add header to packet
  Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4> ();
  NS_ASSERT (ipv4);
  //int32_t ifaceIdx = ipv4->GetInterfaceForDevice (m_udpSensorSocket->GetBoundNetDevice ());
  int32_t ifaceIdx = ipv4->GetInterfaceForAddress (GetAddress ());
  //NS_LOG_LOGIC ("Iface index = " << ifaceIdx);
  //NS_ASSERT (ifaceIdx);
  Ipv4InterfaceAddress address = ipv4->GetAddress(ifaceIdx,0);

  MdcHeader head(address.GetBroadcast (), MdcHeader::mdcDataRequest);
  head.SetOrigin (m_address);
  head.SetId (GetNode ()->GetId ());
  
  Vector pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
  head.SetPosition (pos.x, pos.y);

  p->AddHeader (head);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_requestTrace (p);
  
  m_udpSensorSocket->SendTo (p, 0, InetSocketAddress(Ipv4Address (head.GetDest ()), m_port));
  //if (m_udpSensorSocket->SendTo (p, 0, InetSocketAddress(m_sinkAddress, m_port)) <= 0)
  //NS_LOG_INFO ("packet not sent correctly");

  // Why this? Well, we want beacons to go out at m_interval durations
  // This will queue up the next beacon send event and this function gets called again.
  m_events.push_front (Simulator::Schedule (m_interval, &MdcCollector::Send, this));
}


/*
 * This is the callback function of the MDC and handles the reading of the TCP segments coming from the sensors.
 */
void 
MdcCollector::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION_NOARGS();
  // We index all the packets from the sensors so that the packet segments even if they arrive mixed up, we are able to handle it.
  Ptr<Packet> fullPacket = m_partialPacket[socket];
  //TODO: rename packet -> segment
  Ptr<Packet> packet; // a temp variable holding the segment coming from the sender.
  Address from;

  if (socket == m_sinkSocket)
    {
      //TODO: handle incoming route updates
      return;
    }

  if (fullPacket->GetSize () > 0)
	  NS_LOG_LOGIC ("*COLL Node#" << GetNode ()->GetId ()
			  << " From a PriorRecv already has " << fullPacket->GetSize ()
			  << " B from this socket on Node. "  << socket->GetNode()->GetId());

  // If we already had a full header, we already fired traces about it
  // a boolean that simply triggers a logic to send traces or not.
//TODO:  bool alreadyNotified = fullPacket->GetSize () >= MDC_HEADER_SIZE;
  bool alreadyNotified = false;

  // Prepare to format a new header here.
  MdcHeader head;
  head.SetData (0);  // for if condition for removing completed packets

//TODO:  if (alreadyNotified)
  if (fullPacket->GetSize () >= MDC_HEADER_SIZE)
	// If you have a partial header in the buffer, this may give you seg faults
  {
      fullPacket->PeekHeader (head);
      alreadyNotified = true;
  }
  else
  {
	  // Could not have forwarded to trace as there was no full header so far
	  alreadyNotified = false;
  }

  
  // compute the packetsize of the forwarding packet
  uint32_t packetSize;
  packetSize = (head.GetData () ? head.GetData () + MDC_HEADER_SIZE : UINT_MAX);
  if (packetSize == UINT_MAX)
	  NS_LOG_LOGIC ("*COLL Node#" << GetNode ()->GetId ()
		  << " PacketSize is unknown. First Segment expected."
		  );
  //else
	//  NS_LOG_LOGIC ("*COLL Node#" << GetNode ()->GetId ()
	//	  << " Computed PacketSize= " << packetSize
	//	  << " B from this socket on Node. " << socket->GetNode()->GetId());

  // Repeat this until the socket has no more data to send.

  while (packet = socket->RecvFrom (from)) // Repeat for each segment recd on the socket
    {
      NS_LOG_LOGIC ("Reading packet from socket.");
      
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_LOGIC ("**COLL Node#" << GetNode ()->GetId ()
        		  << " Processing " << packet->GetSize());

          fullPacket->AddAtEnd (packet);
          // Now your fullPacket should contain the partialPacket for the socket if any + this new segment added to it.
          
          // extract as many segments as possible from the stream
          while (fullPacket->GetSize () >= MDC_HEADER_SIZE)
          {
        	// Which means that there may be more segments to process
            Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
            fullPacket->PeekHeader (head);
            packetSize = (head.GetData () ? head.GetData () + MDC_HEADER_SIZE : UINT_MAX);

            
            /************* Changing the notification to after the last segment is recd *****
            // Start of a new packet
// TODO:    if (!alreadyNotified)
              {
                fullPacket->PeekHeader (head);
//TODO:                alreadyNotified = true;

                //TODO: Trace function does not display Source/dest pair!
                m_forwardTrace (packet);

                NS_LOG_LOGIC ("***COLL Node#" << GetNode ()->GetId ()
                		<< " received and processed segment " << packet->GetSize ()
                		<< " B of type "<< head.GetPacketType ()
                		<< " [FullPacket= " << fullPacket->GetSize() << " B "
                		<< " PacketSizeShouldBe= " << packetSize << " B] "
                        << " from " << source << " destined for " << head.GetDest ());
              }
            *******************************************************************************/
            
            // Remove completed transfers
            // OK Here we think that the sender may have combined portions of two packets on to the same segment.
            // In other words a segment just received contains a part of the previous packet and a portion of the next packet.
            // Segments have not been demarcated when they are transmitted from the sensor and so it is our job to handle it cleanly.
            // In some other words...we see 3 conditions...
            //    fullPacket contains exactly a complete packet, (Send a trace)
            //    fullPacket contains a packet and more from the next (Send a trace)
            //    fullPacket is still assembling a complete packet (Don't send a trace)
            if (!alreadyNotified)
              {
                  //TODO: Trace function does not seem to display Source/dest pair!
                  m_forwardTrace (packet);
              }

            if (fullPacket->GetSize () >= packetSize)
              {

//TODO: Trace function does not seem to display Source/dest pair!
//                  m_forwardTrace (packet);

                  NS_LOG_LOGIC ("****COLL Node#" << GetNode ()->GetId ()
                  		<< " received a FULL packet [Size=" << packet->GetSize ()
                  		<< " B] of [type="<< head.GetPacketType ()
                        << "] from [" << source << "] destined for [" << head.GetDest ()
                        << "]");
                // The fullPacket has a complete packet now.
                //    but there may be some more from the next pkt
            	// extract just the portion of the packet that and leave the rest in the fullPacket
                fullPacket->RemoveAtStart (packetSize);

                if (fullPacket->GetSize () > packetSize)
                	NS_LOG_LOGIC ("****COLL Node#" << GetNode ()->GetId ()
                		<< " Segment recd from " << source <<
                		" contained info for more than one packet. Moving " << fullPacket->GetSize()
                		<< " B to partialPacket buffer.");

                alreadyNotified = false;
              }
            else  // Meaning that there may be more segments to read from the socket
              break;

            
          } // end of while
          
          // Now that the segment is ready, just forward it.
          // Note that the packet contains a part of the next segment but we will forward it anyway.
          ForwardPacket (packet);


          // A segment has been forwarded... Now it looks like there are more segments
          if (fullPacket->GetSize () < packetSize)
        	  NS_LOG_LOGIC ("***COLL Node#" << GetNode ()->GetId ()
        			  << "[ PacketBufferSize=" << m_partialPacket[socket]->GetSize()
        			  << "  CompletePacketSize=" << packetSize
                      << "]."
                      );
        }// end of if
    } // end of while socket has no more data
}

/*void
MdcCollector::AckPacket (Ptr<Packet> packet, Address from)
{
  MdcHeader head;
  packet->PeekHeader (head);
  packet = Create<Packet> ();

  head.SetDest (head.GetOrigin ());
  head.SetOrigin (GetAddress ());

  packet->AddHeader (head);

  m_udpSensorSocket->SendTo (packet, 0, from);
  }*/


/*
 * This is the callback method called to forward the packets over to the Sink
 */
void
MdcCollector::ForwardPacket (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION ("Forwarding packet");

  //TODO: Why is this being done?
  packet->RemoveAllPacketTags ();
  packet->RemoveAllByteTags ();

  if (m_sinkSocket->Send (packet) < (int)packet->GetSize ())
  //if (m_sinkSocket->SendTo (packet, 0, InetSocketAddress(m_sinkAddress, m_port)) < (int)packet->GetSize ())
    {
      MdcHeader head;
      packet->PeekHeader (head);

      Ipv4Address destination = head.GetDest ();
      NS_LOG_UNCOND ("Error forwarding packet from " << head.GetOrigin () << " to " << destination);
    }
}

Ipv4Address
MdcCollector::GetAddress () const
{
  return m_address;
}

/*
 * A callback that accepts connections from the sensor and talk
 */
void
MdcCollector::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&MdcCollector::HandleRead, this));

  //m_expectedBytes[s] = 0;
  // create buffer for segments to be reassembled into full packets later,
  // starting with an empty placeholder packet
  m_partialPacket[s] = Create<Packet> ();
}

} // Namespace ns3
