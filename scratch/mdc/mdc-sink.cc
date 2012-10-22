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
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/node-container.h"

#include <climits>
#include "mdc-sink.h"
#include "mdc-header.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MdcSink");
NS_OBJECT_ENSURE_REGISTERED (MdcSink);

TypeId 
MdcSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MdcSink")
    .SetParent<Application> ()
    .AddConstructor<MdcSink> ()
    .AddAttribute ("MdcLocal", "The Address on which to Bind the rx socket for MDCs.",
                   AddressValue (),
                   MakeAddressAccessor (&MdcSink::m_mdcLocal),
                   MakeAddressChecker ())
    .AddAttribute ("SensorLocal", "The Address on which to Bind the rx socket for sensors.",
                   AddressValue (),
                   MakeAddressAccessor (&MdcSink::m_sensorLocal),
                   MakeAddressChecker ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&MdcSink::m_rxTrace))
  ;
  return tid;
}

MdcSink::MdcSink ()
{
  NS_LOG_FUNCTION (this);
  m_sensorSocket = 0;
  m_mdcSocket = 0;
  m_totalRx = 0;
  m_waypointRouting = true;
}

MdcSink::~MdcSink()
{
  NS_LOG_FUNCTION (this);
}

uint32_t MdcSink::GetTotalRx () const
{
  return m_totalRx;
}

void MdcSink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_sensorSocket = 0;
  m_mdcSocket = 0;
  m_acceptedSockets.clear ();
  m_mobilityModels.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void MdcSink::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the sockets if not already
  if (!m_mdcSocket)
    {
      m_mdcSocket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
      m_mdcSocket->Bind (m_mdcLocal);
      m_mdcSocket->Listen ();
    }

  if (!m_sensorSocket)
    {
      m_sensorSocket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
      m_sensorSocket->Bind (m_sensorLocal);
      m_sensorSocket->Listen ();
      m_sensorSocket->ShutdownSend ();
    }

  m_mdcSocket->SetRecvCallback (MakeCallback (&MdcSink::HandleRead, this));
  m_mdcSocket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&MdcSink::HandleAccept, this));
  m_mdcSocket->SetCloseCallbacks (
    MakeCallback (&MdcSink::HandlePeerClose, this),
    MakeCallback (&MdcSink::HandlePeerError, this));

  m_sensorSocket->SetRecvCallback (MakeCallback (&MdcSink::HandleRead, this));
}

void MdcSink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  for (std::map<uint32_t, Ptr<Socket> >::iterator itr = m_acceptedSockets.begin ();
       itr != m_acceptedSockets.end (); itr++)
    {
      Ptr<Socket> acceptedSocket = (*itr).second;
      acceptedSocket->Close ();
      m_acceptedSockets.erase (itr);
    }
  if (m_mdcSocket) 
    {
      m_mdcSocket->Close ();
      m_mdcSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  if (m_sensorSocket) 
    {
      m_sensorSocket->Close ();
      m_sensorSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void MdcSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> fullPacket = (socket == m_sensorSocket ? Create<Packet> () : m_partialPacket[socket]);
  Ptr<Packet> packet;
  Address from;

  // If we already had a full header, we already fired traces about it
  bool alreadyNotified = fullPacket->GetSize () >= MDC_HEADER_SIZE;

  MdcHeader head;
  head.SetData (0);  // for if condition for removing completed packets

  if (alreadyNotified)
    {
      fullPacket->PeekHeader (head);
    }
  
  uint32_t packetSize;
  packetSize = (head.GetData () ? head.GetData () + MDC_HEADER_SIZE : UINT_MAX);
  
  while ((packet = socket->RecvFrom (from)))
    {
      fullPacket->AddAtEnd (packet);

      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
          
      while (fullPacket->GetSize () >= MDC_HEADER_SIZE)
        {
          Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
            
          // Start of a new packet
          if (!alreadyNotified)
            //if (m_expectedBytes[socket] == 0)
            {
              fullPacket->PeekHeader (head);
              //m_expectedBytes[socket] = head.GetData () + head.GetSerializedSize () - packet->GetSize ();
              alreadyNotified = true;
              m_rxTrace (packet, from);

              if (InetSocketAddress::IsMatchingType (from))
                {
                  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                               << "s packet sink received "
                               <<  packet->GetSize () << " bytes from "
                               << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                               << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                               << " total Rx " << m_totalRx << " bytes");
                }
              else if (Inet6SocketAddress::IsMatchingType (from))
                {
                  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                               << "s packet sink received "
                               <<  packet->GetSize () << " bytes from "
                               << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                               << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                               << " total Rx " << m_totalRx << " bytes");
                }
            }
            
          // Remove completed transfers
          if (fullPacket->GetSize () >= packetSize)            
            {
              fullPacket->RemoveAtStart (packetSize);
              alreadyNotified = false;

              if (head.GetFlags () == MdcHeader::sensorFullData ||
                  head.GetFlags () == MdcHeader::sensorDataReply)
                m_totalRx += packetSize;
      
              NS_LOG_INFO ("MDC " << GetNode ()->GetId () << " completed forwarding packet from " << source );
            }
          else
            break;
        }
    }
}

void MdcSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void MdcSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void MdcSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from << Seconds (Simulator::Now ()));
  s->SetRecvCallback (MakeCallback (&MdcSink::HandleRead, this));

  // Get the mobility model associated with the MDC so that we know all their locations
  NodeContainer allNodes = NodeContainer::GetGlobal ();
  Ptr<Node> destNode;
  Ipv4Address addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

  for (NodeContainer::Iterator i = allNodes.Begin (); i != allNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      if (ipv4->GetInterfaceForAddress (addr) != -1)
        {
          destNode = node;
          break;
        }
    }

  if (!destNode)
    {
      NS_LOG_ERROR ("Couldn't find dest node given the IP" << addr);
      return;
    }
  
  uint32_t id = destNode->GetId ();
  m_acceptedSockets[id] = s;
  //m_expectedBytes[s] = 0;
  m_partialPacket[s] = Create<Packet> ();

  Ptr<WaypointMobilityModel> mobility = DynamicCast <WaypointMobilityModel> (destNode->GetObject <MobilityModel> ());
  if (mobility)
    {
      m_mobilityModels[id] = mobility;
    }
  else
    {
      m_waypointRouting = false;
    }
}

} // Namespace ns3
