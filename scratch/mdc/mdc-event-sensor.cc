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
}

MdcEventSensor::~MdcEventSensor()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_udpSocket = 0;
  m_tcpSocket = 0;
  
  delete [] m_data;

  m_data = 0;
  m_dataSize = 0;
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
      //WARNING: remember that its possible for the event to be detected, tx to be queued,
      //and the MDC to request data before expiration, invoking an immediate data reply.
      //
      //It seems realistic enough as the sensor could check for recent data requests and reply
      //to them when done processing anyway...

      ScheduleTransmit (Seconds (m_randomEventDetectionDelay.GetValue ()), InetSocketAddress(m_sinkAddress, m_port));

	  // Typically, on the previous ScheduleTransmit call we expect the sensor to simply send a notification to the sink that
      // an event has occurred and we keep track of the count of such notifications sent.
      if (!m_sendFullData)
        m_nOutstandingReadings++;
    }
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

  NS_LOG_LOGIC ("Sending " << (m_sendFullData ? "full data" : "data notify") << " packet.");

  // if a sequence number is specified, use it.  Otherwise, use the next one.
  if (!seq)
    {
      seq = m_sent;
      m_sent++;
    }

  Ipv4Address destAddr = InetSocketAddress::ConvertFrom (dest).GetIpv4 ();
  MdcHeader::Flags flags;

  /*
   * If the sensor is sending to the sink then a small packet of info is sent to the sink and not the full message
   * Perhaps we will change it to a scenario where the sensor does not send anything anywhere but sits there waiting for the MDC to come along.
   * May not want to remove the send to Sink option completely.
   */
  if (destAddr == m_sinkAddress)
    {
      flags = (m_sendFullData ? MdcHeader::sensorFullData : MdcHeader::sensorDataNotify);
    }
  else
    flags = MdcHeader::sensorDataReply; // Thus means the sensor is sending to a MDC in reply to a mdcDataRequest message

  MdcHeader head (m_sinkAddress, flags); // ultimate destination is the sink anyway
  head.SetSeq (seq);
  head.SetOrigin (m_address); // address of this sensor
  head.SetId (GetNode ()->GetId ()); // The id of this sensor

  Vector pos = GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
  head.SetPosition (pos.x, pos.y); // The position attribute of this sensor

  Ptr<Packet> p;
  //TODO: We may want to identify the conditions better... esp. if the destination address needs to be other than the sink.
  if (!m_sendFullData and destAddr == m_sinkAddress) // Just the notification message and so there is no data
    {
      p = Create<Packet> (0);
      head.SetData (0);
    }
  else // This means that you need to send the fulldata somehow. In any case the destination is the sinkAddress.
    {
      // If only notifying of an event and not sending the full data,
      // don't add fill and set the data size to be 0.
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
      head.SetData (p->GetSize ());
  
      // only schedule timeouts for full data
      //ScheduleTimeout (seq, dest);
    }

  p->AddHeader (head);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_sendTrace (p);

  if (flags == MdcHeader::sensorFullData || flags == MdcHeader::sensorDataReply)
    {
      // some weird bug makes establishing a connection to the same address you did last time not work
      if (dest != m_lastConnection)
        {
          NS_ASSERT (m_tcpSocket->Connect(dest) >= 0);
          m_lastConnection = dest;
        }

      m_tcpSocket->Send (p);
    }
  else
    m_udpSocket->SendTo (p, 0, dest);
}

/*
 * A callback function called when the sensor needs to read something
 */
void 
MdcEventSensor::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;

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

          // If the packet is for us, process the ACK
          if (head.GetDest () == m_address)
            {
              return;
              //ProcessAck (packet, source);
              // TODO: Perhaps check for a mdcDataRequest message send to the sensor directly and simply respond with a sensorDataReply here.
            }

          // Else it was a broadcast from an MDC
          //
          // Should we keep track of the last seen MDC?
          else
            {
              if (m_nOutstandingReadings > 0)
                {
                  for (int i = m_nOutstandingReadings; i > 0; i--)
                    {
                      ScheduleTransmit (Seconds (i*0.1 + 0.1), from);
                      //TODO: Stagger the events based on the message sizes... add the transmission delay and processing time.
                      // We may not really need this but think about it.
                      m_nOutstandingReadings--;
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

/*
 * Kyle may have removed this code... he was trying to implement the reliability aspect.
 */
  /*
void
MdcEventSensor::ProcessAck (Ptr<Packet> packet, Ipv4Address source)
{
  NS_LOG_LOGIC ("ACK received");

  MdcHeader head;
  packet->PeekHeader (head);
  uint32_t seq = head.GetSeq ();
  
  //m_ackTrace (packet, GetNode ()-> GetId ());
  m_outstandingSeqs.erase (seq);
  //TODO: handle an ack from an old seq number
}

void
MdcEventSensor::ScheduleTimeout (uint32_t seq, Address dest)
{
  m_events.push_front (Simulator::Schedule (m_timeout, &MdcEventSensor::CheckTimeout, this, seq, dest));

  // Decrement the number of retries remaining if found
  if (m_outstandingSeqs.find (seq) != m_outstandingSeqs.end ())
    m_outstandingSeqs[seq]--;
  else
    m_outstandingSeqs[seq] = m_retries;
}

void
MdcEventSensor::CheckTimeout (uint32_t seq, Address dest)
{
  std::map<uint32_t, uint8_t>::iterator triesLeft = m_outstandingSeqs.find (seq);

  // If this seq # hasn't been ACKed
  if (triesLeft != m_outstandingSeqs.end ())
    {
      if (triesLeft->second > 0)
        {
          NS_LOG_INFO ("Packet timed out at node " << GetNode ()->GetId ());

          m_events.push_front (Simulator::Schedule (Seconds (m_randomEventDetectionDelay.GetValue ()),
                                                    &MdcEventSensor::Send, this, dest, seq));
        }
      else //give up sending it
        {
          NS_LOG_INFO ("Packet failed to send after " << (uint32_t)m_retries << " attempts at node " << GetNode ()->GetId ());

          m_outstandingSeqs.erase (triesLeft);
        }
    }
    }*/

} // Namespace ns3
