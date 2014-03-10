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
/*
 * RRAJ - Find the diffs between SocketRead and SocketAccept
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
#include "ns3/pointer.h"
#include "ns3/node-list.h"
#include "ns3/random-waypoint-mobility-model.h"

#include <climits>
#include "mdc-sink.h"
#include "mdc-header.h"
#include "mdc-utilities.h"
#include "mdc-event-tag.h"
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
//    .AddAttribute ("MDC_NC_Pointer", "The pointer to the MDC Node Container.",
//    			   PointerValue (),
//    			   MakePointerAccessor (&MdcSink::m_allMDCNodes),
//    			   MakePointerChecker<Ptr<NodeContainer> > ())
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

  std::cout << "Pos Vector contains... " << posVector.size() << " elements \n";
  for (std::vector<Vector>::iterator it = posVector.begin() ; it != posVector.end(); ++it)
	  std::cout << " [" << (*it).x << ":" << (*it).y << ":" << (*it).z << "]\n";
  std::cout << '\n';

  posVector.clear();
  m_events.clear();
  m_AllSensedEvents.clear();

  // chain up
  Application::DoDispose ();
}


// Application Methods
/*
 * Here we create the two sockets, the MDC socket and the Sensor socket.
 * Establish the callback functions on these sockets
 */
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
      m_sensorSocket->ShutdownSend (); // The Sensor socket is listen-only. The sink does not send anything to the sensor
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

/*
 * Clean up before stopping
 */
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

/* This tells the simulator to schedule a event detection at a specific simulation time
 * Whether the sink here acts on the event or not is a decision that is done in the CheckEventDetection
 */
void
MdcSink::ScheduleEventDetection (Time t, SensedEvent event)
{
  m_events.push_front (Simulator::Schedule (t, &MdcSink::CheckEventDetection, this, event));
}

/*
 * Events are all setup globally and every sensors/sink knows about the events.
 * When the event occurs, they also have a physical location of the event... An event has 3 other attributes... [time, location, size]
 * This logic then determines the location on the event and keeps track of a list locations that the MDC should be traveling to,
 * then supply a path to the MDC.
 */
void
MdcSink::CheckEventDetection (SensedEvent event)
{
  NS_LOG_LOGIC ("Node " << GetNode ()->GetId () << " checking event detection.");

//  Vector pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
  Vector pos = event.GetCenter();
  std::stringstream s, csv;
  s << "[SINK EVENT_DETECTION] Event detected at SINK Node=" << GetNode()->GetId () << " at Time=" << Simulator::Now().GetSeconds() << " seconds at Location=[" << pos << "]" << std::endl;
  csv << "SINK_EVENT_DETECTION,"<< GetNode()->GetId () << "," << Simulator::Now().GetSeconds() << "," << pos.x << "," << pos.y << "," << pos.z << std::endl;
  *(GetMDCOutputStream())->GetStream() << csv.str();
  //std::cout << s.str();
  NS_LOG_INFO(s.str());


  // This is where we keep track of the event positions to send the MDC to
  // From the event, extract the Position information.
  posVector.push_back(event.GetCenter());
  NS_LOG_FUNCTION ( " Recorded Event Location=[" << event.GetCenter() << "] [VectorSize =" << posVector.size() << "]\n");
//  for (std::vector<Vector>::iterator it = posVector.begin() ; it != posVector.end(); ++it)
//	  std::cout << " [" << (*it) << "]\n";


  m_AllSensedEvents.insert(std::pair<uint32_t, SensedEvent>(event.GetEventId(), event));
  ResetMobility();
}



/*
 * This is a socket callback function that gets called.
 * The functionality changes depending upon whether the socket is a sensorSocket or a MdcSocket
 * If this is an MDC socket, we look for a previously held partial packet and process it accordingly.
 * Previously held packets are stored in a map called m_partialPacket and is indexed by the socket id.
 */
void MdcSink::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION_NOARGS();
	if (socket == m_sensorSocket)
    {
    	return;
    }

	// We index all the packets from the MDCs so that the packet segments even if they arrive mixed up, we are able to handle it.
	Ptr<Packet> fullPacket = m_partialPacket[socket];
	Ptr<Packet> packet; // a temp variable holding the segment coming from the sender.
	Address from;
	// a boolean that simply triggers a logic to send traces or not.
	bool alreadyNotified;

	NS_LOG_LOGIC ("*SINK Node#" << GetNode ()->GetId () <<
		  " Partial Packet Size for this socket was=" << fullPacket->GetSize ());

	if (fullPacket->GetSize () > 0)
	{
		NS_LOG_LOGIC ("*SINK Node#" << GetNode ()->GetId ()
			  << " From a PriorRecv already has " << fullPacket->GetSize ()
			  << " B from this socket on Node. "  << socket->GetNode()->GetId());
	}


	// Prepare to format a new header here.
	MdcHeader head;
	head.SetData (0);  // for if condition for removing completed packets

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
		NS_LOG_LOGIC ("*SINK Node#" << GetNode ()->GetId ()
		  << " PacketSize is unknown. First Segment expected."
		  );

	// Repeat this until the socket has no more data to send.
	while (packet = socket->RecvFrom (from)) // Repeat for each segment recd on the socket
    {
		NS_LOG_LOGIC ("Reading packet from socket.");

		if (InetSocketAddress::IsMatchingType (from))
        {
			fullPacket->AddAtEnd (packet);
			// Now your fullPacket should contain the partialPacket for the socket if any + this new segment added to it.
			NS_LOG_LOGIC("**3** FullPktSize=" << fullPacket->GetSize () << " ExpectedPktSize=" << packetSize << " CurrentPktSize=" << packet->GetSize());

			// Which means that there may be more segments to process
			Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
			fullPacket->PeekHeader (head);
			packetSize = (head.GetData () ? head.GetData () + MDC_HEADER_SIZE : UINT_MAX);

			if (!alreadyNotified)
			{
//	            m_rxTrace (packet, from); // This writes a trace output that the first segment of a message was received and processed.
				alreadyNotified = true;
			}

			NS_LOG_LOGIC("**4** FullPktSize=" << fullPacket->GetSize () << " ExpectedPktSize=" << packetSize << " CurrentPktSize=" << packet->GetSize());
			if (fullPacket->GetSize () >= packetSize)
			{

				NS_LOG_LOGIC ("****SINK Node#" << GetNode ()->GetId ()
						<< " received a FULL packet TOTAL=" << fullPacket->GetSize()
						<< "B and FRAGMENT Size=" << packet->GetSize ()
						<< "B of [type="<< head.GetPacketType ()
						<< "] from [" << source << "] destined for [" << head.GetDest ()
						<< "]");
				// The fullPacket has a complete packet now.

//				m_rxTrace (fullPacket, from); // This writes a trace output.



				//    but there may be some more from the next pkt
				// extract just the portion of the packet that and leave the rest in the fullPacket... that was one idea.

				fullPacket->RemoveAtStart(packetSize);
				alreadyNotified = false; // So that a next packet from this socket gets traced appropriately.
				if (fullPacket->GetSize() >= MDC_HEADER_SIZE )
				{
					fullPacket->PeekHeader (head); // Reset the packetSize
					packetSize = (head.GetData () ? head.GetData () + MDC_HEADER_SIZE : UINT_MAX);
				}
				else
					packetSize = 0;

				NS_LOG_LOGIC("**5** FullPktSize=" << fullPacket->GetSize () << " ExpectedPktSize=" << packetSize << " CurrentPktSize=" << packet->GetSize());
				if (fullPacket->GetSize () > packetSize)
				{
					NS_LOG_LOGIC ("****SINK Node#" << GetNode ()->GetId ()
						<< " Segment recd from " << source
						<< " contained info beyond the boundary of one packet. Ignoring the rest... FULLPACKETSIZE=" << fullPacket->GetSize()
						<< " B and ExpectedPACKETSIZE=" << packetSize << "B. Cleaning up the partialPacket buffer.");
				}
			}

			// Now that the segment is ready, just forward it.
			// Note that the packet contains a part of the next segment but we will forward it anyway.
			// IF YOU NEED TO FORWARD THE PACKET TO ANOTHER NODE/NETWORK, This is the place to do it.
			// ForwardPacket (packet);
			ProcessPacket(packet);
			SinkPacketRecvTrace(packet, from);

			// We might want to change the path/route whenever a new event shows up.
			//ResetMobility();


			NS_LOG_LOGIC("**6** FullPktSize=" << fullPacket->GetSize () << " ExpectedPktSize=" << packetSize << " CurrentPktSize=" << packet->GetSize());
			// A segment has been forwarded... Now it looks like there are more segments
			if (fullPacket->GetSize () > packetSize)
				NS_LOG_LOGIC ("*SINK Node#" << GetNode ()->GetId ()
					  << "Expecting more data... PacketBufferSize=" << m_partialPacket[socket]->GetSize()
					  << "  CompletePacketSize=" << packetSize
					  << "."
					  );

		}// end of if
    } // end of while socket has no more data
}

/*
 * This is a normal socket close callback function registered on the socket
 */
void MdcSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
/*
 * This is a socket close callback function registered when there is an error
 */
void MdcSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
/*
 * The socket callback function registered and invoked when a connection from the 'from' ip address is received.
 * A new secket is created and the HandleRead callback function is attached to it.
 */
void MdcSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this->GetTypeId() << s << from << Seconds (Simulator::Now ()));
	s->SetRecvCallback (MakeCallback (&MdcSink::HandleRead, this));
	// Get the mobility model associated with the MDC so that we know all their locations


	// All you have is an IP address and so you need to find out what node this is.
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
/********************
	string s1;
	s1 = destNode->GetTypeId().GetName();
	Vector pos;

	if (s1.compare("ns3::MdcCollector")==0)
	{
		// This is indeed an MDC node and so grab its pos.ition... reset the mobility if needed
//		mdcMobility = DynamicCast <WaypointMobilityModel> (destNode->GetObject <MobilityModel> ());
		pos = destNode->GetObject <MobilityModel> ()->GetPosition();
		Ptr<ListPositionAllocator> listPosAllocator = CreateObject<ListPositionAllocator> ();
		Vector vDepotPos = Vector3D (0.0, 0.0, 0.0);

		RecomputePosAllocator(pos, vDepotPos, &posVector, listPosAllocator);
		(destNode->GetObject <MobilityModel> ())->SetAttribute("PositionAllocator", PointerValue (listPosAllocator));
//		posVector.push_back(pos);
	}
	else
	{
		// Do nothing for now.
	}
**************************/

	m_acceptedSockets[id] = s;
	//m_expectedBytes[s] = 0;
	m_partialPacket[s] = Create<Packet> ();
}

	/*
	 * A placeholder to reset the mobility of an MDC.
	 */
	void MdcSink::ResetMobility()
	{
		// TODO: For now, I will be setting the route of all the MDCs. Need to have an ability to send each MDC on its own path.

		Ptr<MobilityModel> mdcMobility;

		std::vector<Ptr<Node> >  vNodes = GetMDCNodeVector();
		Ptr<Node> pN;
		for (uint32_t i=0; i<vNodes.size();i++)
		{

			Vector pos;
			pN = vNodes.at(i);

//			mdcMobility = DynamicCast <WaypointMobilityModel> (pN->GetObject <MobilityModel> ());
			mdcMobility = pN->GetObject <MobilityModel> ();

			// This is indeed an MDC node and so grab its pos.ition... reset the mobility if needed
//			pos = (pN->GetObject <MobilityModel> ())->GetPosition();
			pos = mdcMobility->GetPosition();

//			std::cout << pos << std::endl;

			Ptr<ListPositionAllocator> listPosAllocator = CreateObject<ListPositionAllocator> ();
			// Consider the currentPos, depotPos and the list of pending vectors and then come with a most efficient path here.
			Vector vDepotPos = Vector3D (0.0, 0.0, 0.0);
			RecomputePosAllocator(pos, vDepotPos, &posVector, listPosAllocator);


			// Apply the new path to the MDC node
//			(pN->GetObject <MobilityModel> ())->SetAttribute("PositionAllocator", PointerValue (listPosAllocator));
			mdcMobility->SetAttribute("PositionAllocator", PointerValue (listPosAllocator));
		}

	}

	/*
	 * A placeholder for any logic that would work on the packet data.
	 */
	void MdcSink::ProcessPacket(Ptr<Packet> packet)
	{
		MdcEventTag eventTag;

		std::string s1, s2;

		ByteTagIterator bti = packet->GetByteTagIterator();
		while (bti.HasNext())
		{
			ns3::ByteTagIterator::Item bt = bti.Next();
			s1 = (bt.GetTypeId()).GetName();
			s2 = (eventTag.GetTypeId()).GetName();

			if (s1.compare(s2)==0)
			{
				bt.GetTag(eventTag);
				NS_LOG_FUNCTION (eventTag.GetEventId() << "-" << eventTag.GetTime() << "\n");

				std::map<uint32_t, SensedEvent>::iterator it;
				it = m_AllSensedEvents.find(eventTag.GetEventId());
				if (it != m_AllSensedEvents.end())
				{
					SensedEvent se = (*it).second;
					NS_LOG_FUNCTION ( "Pos Vector contains... " << posVector.size() << " elements \n");
					NS_LOG_INFO ( "  Removing Event <" << it->first << "> at Vector...[ " << se.GetCenter() << " ] \n");
					RemoveVectorElement (&posVector, se.GetCenter());
					NS_LOG_FUNCTION ( "Pos Vector conatins... " << posVector.size() << " elements \n");
				}
				break;
			}
		}

	}


	// Trace function for the Sink
	void
	MdcSink::SinkPacketRecvTrace (Ptr<const Packet> packet, const Address & from)
	{
		NS_LOG_FUNCTION_NOARGS ();

		MdcHeader head;
		packet->PeekHeader (head);
		std::stringstream s;
		Ipv4Address fromAddr = InetSocketAddress::ConvertFrom(from).GetIpv4 ();

		// Ignore MDC's data requests, especially since they'll hit both interfaces and double up...
		// though this shouldn't happen as broadcast should be to a network?
		if (   (head.GetFlags () == MdcHeader::mdcDataForward)
			|| (head.GetFlags () == MdcHeader::mdcDataRequest)
			|| (head.GetFlags () == MdcHeader::sensorDataNotify)
			)
		{
			s << "[SINK__TRACE] IGNORED....."
					<< GetNode ()->GetId ()
					<< " received ["
					<< head.GetPacketType ()
					<< "] (" << head.GetData () + head.GetSerializedSize ()
					<< "B) from node "
					<< head.GetId ()
					<< " via "
					<< fromAddr
					<< " at "
					<< Simulator::Now ().GetSeconds ();

	//		NS_LOG_LOGIC (s.str ());
		}

		if (	(head.GetFlags () == MdcHeader::sensorDataReply)
			)
		{
			// Complete pkt received. Send an indication
			uint32_t expectedPktSize = head.GetData() + head.GetSerializedSize ();
			uint32_t recdPktSize = packet->GetSize();
			std::stringstream s, csv;

			if (recdPktSize < expectedPktSize)
			{
				s.str("");
				csv.str("");
				s		<< "[SINK__TRACE] Sink Node#"
						<< GetNode ()->GetId ()
						<< " FIRST_SEGMENT ["
						<< head.GetPacketType ()
						<< "] SegmentSize=" << recdPktSize
						<< "B ExpectedFullPacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " via "
						<< fromAddr
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "SINK__TRACE,"
						<< GetNode ()->GetId ()
						<< ",FIRST_SEGMENT,"
						<< head.GetPacketType ()
						<< "," << recdPktSize
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< fromAddr
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();
				NS_LOG_INFO (s.str ());
				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
			}
			else if (recdPktSize > expectedPktSize)
			{
				s.str("");
				csv.str("");
				s		<< "[SINK__TRACE] Sink Node#"
						<< GetNode ()->GetId ()
						<< " FULL_PACKET ["
						<< head.GetPacketType ()
						<< "] CompletePacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " via "
						<< fromAddr
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "SINK__TRACE,"
						<< GetNode ()->GetId ()
						<< ",FULL_PACKET,"
						<< head.GetPacketType ()
						<< "," << expectedPktSize
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< fromAddr
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();
				NS_LOG_INFO (s.str ());

				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
				PrintEventTrace(0, packet );

				// Reset s here for the next message for the partial pkt recd...
				s.str("");
				csv.str("");
				s		<< "[SINK__TRACE] Sink Node#"
						<< GetNode ()->GetId ()
						<< " FIRST_SEGMENT ["
						<< head.GetPacketType ()
						<< "] SegmentSize=" << (recdPktSize - expectedPktSize)
						<< "B ExpectedFullPacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " via "
						<< fromAddr
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "SINK__TRACE,"
						<< GetNode ()->GetId ()
						<< ",FIRST_SEGMENT,"
						<< head.GetPacketType ()
						<< "," << (recdPktSize - expectedPktSize)
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< fromAddr
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();


				NS_LOG_INFO (s.str ());
				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
			}
			else // the recdPktsize and expectedpacketsize are equal
			{
				s.str("");
				csv.str("");
				s		<< "[SINK__TRACE] Sink Node#"
						<< GetNode ()->GetId ()
						<< " FULL_PACKET ["
						<< head.GetPacketType ()
						<< "] CompletePacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " via "
						<< fromAddr
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "SINK__TRACE,"
						<< GetNode ()->GetId ()
						<< ",FULL_PACKET,"
						<< head.GetPacketType ()
						<< "," << expectedPktSize
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< fromAddr
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();
				NS_LOG_INFO (s.str ());
				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
				PrintEventTrace(0, packet );
			}
		}

		return;
	}



} // Namespace ns3
