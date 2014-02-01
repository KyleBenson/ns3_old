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
#include "ns3/boolean.h"

#include "mdc-event-sensor.h"
#include "mdc-header.h"

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
                   UintegerValue (9999),
                   MakeUintegerAccessor (&MdcEventSensor::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Ipv4Address of the sink",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&MdcEventSensor::m_sinkAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Timeout", "Time to wait for a reply before trying to resend.",
                   TimeValue (Seconds (3)),
                   MakeTimeAccessor (&MdcEventSensor::m_timeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxRetries", 
                   "The maximum number of times the application will attempt to resend a failed packet",
                   UintegerValue (3),
                   MakeUintegerAccessor (&MdcEventSensor::m_retries),
                   MakeUintegerChecker<uint8_t> ())
//    DEPRECATING SENDFULLDATA Flag...
    .AddAttribute ("SendFullData",
                   "Whether or not to send the full data during event detection.  If false, sends notification only and will send the full data to an MDC.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MdcEventSensor::m_sendFullData),
                   MakeBooleanChecker ())
    .AddAttribute ("RandomEventDetectionDelay",
                   "The random variable from which a random delay for the time between an event occurring and a sensor detecting it is taken",
                   RandomVariableValue (UniformVariable ()),
                   MakeRandomVariableAccessor (&MdcEventSensor::m_randomEventDetectionDelay),
                   MakeRandomVariableChecker ())
    .AddAttribute ("PacketSize", "Size of sensed data in outbound packets",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&MdcEventSensor::SetDataSize,
                                         &MdcEventSensor::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Send", "A new packet is created and is sent to the sink or an MDC",
                     MakeTraceSourceAccessor (&MdcEventSensor::m_sendTrace))
    .AddTraceSource ("Rcv", "A packet is received",
                     MakeTraceSourceAccessor (&MdcEventSensor::m_rcvTrace))
  ;
  return tid;
}

MdcEventSensor::MdcEventSensor ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_sent = 0;
  m_udpSocket = 0;
  m_tcpSocket = 0;
  m_data = 0;
  m_dataSize = 0;
  m_nOutstandingReadings = 0;

  m_randomEventDetectionDelay = UniformVariable ();

  m_address = Ipv4Address((uint32_t)0);

  // TODO: Need to create config parameters for this
  m_mdcProximityLimit = 5; // This is a distance within which the Sensors will consider as within proximity of an MDC.
  m_sensorProcessingDelay = 0.05; // This is a duration of time in seconds that the Sensors endure before transmitting a reply.
  m_sensorTransmissionDelay = 0.01; // Transmission delay in seconds

}

MdcEventSensor::~MdcEventSensor()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_udpSocket = 0;
  m_tcpSocket = 0;
  
  delete [] m_data;

  m_data = 0;
  m_dataSize = 0;

  for (unsigned i=0; i< m_packetBuffer.size(); i++)
	  m_packetBuffer[i].Cleanup();
}

void 
MdcEventSensor::SetSink (Ipv4Address ip, uint16_t port /* = 0*/)
{
  m_sinkAddress = ip;

  if (port)
    m_port = port; //may already be set
}

void
MdcEventSensor::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

/*
 * The event sensor sends notifications on the UDP socket as there is no necessity to have a reliability of transfer for them.
 *
 */
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

  if (m_udpSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_udpSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_udpSocket->Bind (local);

      NS_LOG_LOGIC ("Udp Socket bound");
    }

  if (m_tcpSocket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_tcpSocket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_tcpSocket->Bind (local);

      NS_LOG_LOGIC ("Tcp Socket bound");
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

  m_udpSocket->SetRecvCallback (MakeCallback (&MdcEventSensor::HandleRead, this));
  m_tcpSocket->SetRecvCallback (MakeCallback (&MdcEventSensor::HandleRead, this));
}

void 
MdcEventSensor::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_tcpSocket != 0) 
    {   
      m_tcpSocket->Close ();
      m_tcpSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_tcpSocket = 0;
    }

  if (m_udpSocket != 0) 
    {   
      m_udpSocket->Close ();
      m_udpSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_udpSocket = 0;
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

/*
 * Deletes the content of the packet buffer if one exists and then...
 * Fills the Packet buffer with the string passed called "fill"
 * Sets the packet size attributes accordingly.
 * The new packet will be the size of the string provided (+1)
 */
void 
MdcEventSensor::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize) // re-allocate the entire packet buffer
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

/*
 * This method fills the packet with a specific character all the way till the dataSize.
 */
void 
MdcEventSensor::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)// re-allocate the entire packet buffer
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

/*
 * This method fills the packet with a specific character all the way till the dataSize.
 */
void 
MdcEventSensor::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)// re-allocate the entire packet buffer
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize); // Just don't go beyond the dataSize specified
      // TODO: m_size is not set here???
      return;
    }

  // TODO: Can we not use the SetFill a specific char to a specific size instead of this convoluted logic?

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

/*
 * The data packet buffer content is released and the size set to a specific value as a side-effect of this.
 */
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

/*
 * Getter for the m_size parameter.
 */
uint32_t 
MdcEventSensor::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

/* This tells the simulator to schedule a event detection at a specific simulation time
 * Whether the sensor here acts on the event or not is a decision that is done in the CheckEventDetection
 */
void
MdcEventSensor::ScheduleEventDetection (Time t, SensedEvent event)
{
  m_events.push_front (Simulator::Schedule (t, &MdcEventSensor::CheckEventDetection, this, event));
}

/*
 * Events are all setup globally and every sensor knows about the events.
 * When the event occurs, they also have a physical location of the event... An event has 3 other attributes... [time, location, size]
 * This logic then determines if the sensor is within the area or not and then if it is, then simulate that the event was detected by the sensor.
 */
void
MdcEventSensor::CheckEventDetection (SensedEvent event)
{
  NS_LOG_LOGIC ("Node " << GetNode ()->GetId () << " checking event detection.");

  Vector pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();

  if (event.WithinEventRegion (pos))
    {
      NS_LOG_INFO ("[EVENT_DETECTION] Event detected at Node=" << GetNode()->GetId () << " at Time=" << Simulator::Now().GetSeconds() << " seconds at Location=[" << pos << "]");

      //WARNING: remember that its possible for the event to be detected, tx to be queued,
      //and the MDC to request data before expiration, invoking an immediate data reply.
      //
      //It seems realistic enough as the sensor could check for recent data requests and reply
      //to them when done processing anyway...

//      ScheduleTransmit (Seconds (m_randomEventDetectionDelay.GetValue ()), InetSocketAddress(m_sinkAddress, m_port));
	  RecordSensorData(InetSocketAddress(m_sinkAddress, m_port));
	  // We expect the sensor to keep track that
      // an event has occurred. We keep track of the count of such events processed there.

    }
}

/*
 * Just pushes the event data into a buffer
 */
void
MdcEventSensor::RecordSensorData (Address dest)
{
	// Some time in the future, the dest address can be diff from Sink
	//TODO: We may want to identify the conditions better... esp. if the destination address needs to be other than the sink.
//	Ipv4Address destAddr = InetSocketAddress::ConvertFrom (dest).GetIpv4 ();

	// Format the message


	MdcHeader::Flags flags;
	flags = MdcHeader::sensorDataReply; // Thus means the sensor is sending to a MDC in reply to a mdcDataRequest message
	MdcHeader head (m_sinkAddress, flags); // ultimate destination is the sink anyway
	head.SetOrigin (m_address); // address of this sensor
	head.SetId (GetNode ()->GetId ()); // The id of this sensor

	Vector pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
	head.SetPosition (pos.x, pos.y); // The position attribute of this sensor


	// Now format the rest of the message/packet from the sensor.
	Ptr<Packet> p;
	p = Create<Packet> (m_size);
	head.SetData (p->GetSize ());

	// only schedule timeouts for full data
	//ScheduleTimeout (seq, dest);

	p->AddHeader (head);

    // call to the trace sinks before the packet is actually sent,
    // so that tags added to the packet can be traced as well
    m_sendTrace (p);

	m_packetBuffer.push_back(*p);
    // Event data is captured and will be held in a buffer for the MDC to come and pick up
    m_nOutstandingReadings++;

/******************************************************
 * Adding a trailer will simply add to the number of packets floating in the system.
    // Add a Trailer to mark the end of the message
	MdcTrailer trailer (m_sinkAddress, MdcTrailer::sensorDataTrailer); // ultimate destination is the sink anyway
	trailer.SetOrigin (m_address); // address of this sensor
	trailer.SetId (GetNode ()->GetId ()); // The id of this sensor
	trailer.SetPosition (pos.x, pos.y); // The position attribute of this sensor
    p = Create<Packet>(MDC_TRAILER_SIZE);
	trailer.SetData (p->GetSize ());
	p->AddHeader (trailer);
    // call to the trace sinks to signal that a trailer is being sent
    // so that tags added to the packet can be traced as well
    m_sendTrace (p);

	m_packetBuffer.push_back(*p);
    // Event data is captured and will be held in a buffer for the MDC to come and pick up
    m_nOutstandingReadings++;
*********************************************************/
}

/*
 * Just pushes an event at the address to the simulator
 */
void 
MdcEventSensor::ScheduleTransmit (Time dt, Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_events.push_front (Simulator::Schedule (dt, &MdcEventSensor::Send, this, address, 0));
}

/*
 * This sends a message to the destination and updates a message sequence number
 *
 * TODO: What if the sensor has more data and the MDC hasn't picked up the packet yet?
 * TODO: Should we implement this with m_data being a Vector of packets?
 */
void 
MdcEventSensor::Send (Address dest, uint32_t seq /* = 0*/)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Packet> p;
  
  // Check if the m_packetBuffer has any saved packets.
  // If any then send them one after another.
  for (unsigned i = 0; i<m_nOutstandingReadings; i++)
    {
	  p = &(m_packetBuffer[i]);

      // some weird bug makes establishing a connection to the same address you did last time not work
      if (dest != m_lastConnection)
        {
          NS_ASSERT (m_tcpSocket->Connect(dest) >= 0);
          m_lastConnection = dest;
        }

      // call to the trace sinks before the packet is actually sent,
      // so that tags added to the packet can be sent as well
      m_sendTrace (p);
      m_tcpSocket->Send (p);
    }
  m_nOutstandingReadings=0;
}

/*
 * A callback function called when the sensor needs to read something
 */
void 
MdcEventSensor::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  Vector mdcCurrPos;
  Vector sensorPos;
  double distanceFromMDC = 0;

  /*
   * This is in case there are multiple packets to handle
   */
  while (packet = socket->RecvFrom (from))
    {
      NS_LOG_LOGIC ("Reading packet from socket. " << m_nOutstandingReadings << " outstanding readings.");

      if (InetSocketAddress::IsMatchingType (from))
        {
          //Ipv4Address source = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

          MdcHeader head;
          packet->PeekHeader (head);

          // Note the MDC Position and see how far the sensor is from MDC
          mdcCurrPos.x = head.GetXPosition();
		  mdcCurrPos.y = head.GetYPosition();
		  mdcCurrPos.z = 0.0;
		  sensorPos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
		  distanceFromMDC = CalculateDistance(mdcCurrPos, sensorPos);

          // If the packet is for us, process the ACK
          if (head.GetDest () == m_address)
            {
              return;
              //ProcessAck (packet, source);
              // TODO: Perhaps check for a mdcDataRequest message send to the sensor directly and simply respond with a sensorDataReply here.
            }

          // Else it was a broadcast from an MDC
          // Make sure that you send only if the MDC is within close proximity of the sensor
          // Should we keep track of the last seen MDC?
          else
            {
              if ((m_nOutstandingReadings > 0) &&
            	  (distanceFromMDC <= m_mdcProximityLimit))
                {
                  for (unsigned i = 0; i<m_nOutstandingReadings; i++)
                    {
                      ScheduleTransmit (Seconds ((i+1)*(m_sensorProcessingDelay + m_sensorTransmissionDelay)), from);
                      //TODO: Stagger the events based on the message sizes... add the transmission delay and processing time.
                      // We may not really need this but think about it.
                      //m_nOutstandingReadings--;
                    }
                }
            }
        }
    }
}

Ipv4Address
MdcEventSensor::GetAddress () const
{
  return m_address;
}


} // Namespace ns3
