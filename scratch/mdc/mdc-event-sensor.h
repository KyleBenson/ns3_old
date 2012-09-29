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

#ifndef MDC_EVENT_SENSOR_H
#define MDC_EVENT_SENSOR_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable.h"

#include "sensed-event.h"
#include "mdc-header.h"

#include <list>
#include <set>
#include <vector>

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup mdc Mdc
 */

/**
 * \ingroup mdc
 * \brief A Mobile Data Collector sensor client
 *
 * Reports data about sensed events, which may be simply notifying the sink about data availability
 * and then sending it to a MDC for forwarding to the sink to save power.
 */
class MdcEventSensor : public Application 
{
public:
  static TypeId GetTypeId (void);
  MdcEventSensor ();
  virtual ~MdcEventSensor ();

  /**
   * \param ip destination ipv4 address
   * \param port destination port
   */
  void SetSink (Ipv4Address ip, uint16_t port = 0);

  /**
   * Set the data size of the packet (the number of bytes that are sent as data
   * to the server).  The contents of the data are set to unspecified (don't
   * care) by this call.
   *
   * \warning If you have set the fill data for the echo client using one of the
   * SetFill calls, this will undo those effects.
   *
   * \param dataSize The size of the echo data you want to sent.
   */
  void SetDataSize (uint32_t dataSize);

  /**
   * Get the number of data bytes that will be sent to the server.
   *
   * \warning The number of bytes may be modified by calling any one of the 
   * SetFill methods.  If you have called SetFill, then the number of 
   * data bytes will correspond to the size of an initialized data buffer.
   * If you have not called a SetFill method, the number of data bytes will
   * correspond to the number of don't care bytes that will be sent.
   *
   * \returns The number of data bytes.
   */
  uint32_t GetDataSize (void) const;

  /**
   * Set the data fill of the packet (what is sent as data to the server) to 
   * the zero-terminated contents of the fill string string.
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the size of the fill string -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param fill The string to use as the actual echo data bytes.
   */
  void SetFill (std::string fill);

  /**
   * Set the data fill of the packet (what is sent as data to the server) to 
   * the repeated contents of the fill byte.  i.e., the fill byte will be 
   * used to initialize the contents of the data packet.
   * 
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the dataSize parameter -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param fill The byte to be repeated in constructing the packet data..
   * \param dataSize The desired size of the resulting echo packet data.
   */
  void SetFill (uint8_t fill, uint32_t dataSize);

  /**
   * Set the data fill of the packet (what is sent as data to the server) to
   * the contents of the fill buffer, repeated as many times as is required.
   *
   * Initializing the packet to the contents of a provided single buffer is 
   * accomplished by setting the fillSize set to your desired dataSize
   * (and providing an appropriate buffer).
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the dataSize parameter -- this means that the PacketSize
   * attribute of the Application may be changed as a result of this call.
   *
   * \param fill The fill pattern to use when constructing packets.
   * \param fillSize The number of bytes in the provided fill pattern.
   * \param dataSize The desired size of the final echo data.
   */
  void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

  /**
   * Get the IP Address of the wifi interface in spot 0 of this node.
   */
  Ipv4Address GetAddress () const;

  /**
   * Schedules an event to occur at the specified time and whether the sensor should directly
   * transmit to the sink or if it should notify the sink of availability and then transmit
   * it to the MDC when available.
   */
  void ScheduleEventDetection (Time t, SensedEvent event, bool noData = false);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void SetTimeout (Time t);
  void Send (bool noData);
  void CancelEvents (void);
  void ScheduleTransmit (Time dt, bool noData = false);
  void CheckEventDetection (SensedEvent event, bool noData = false);

  void ForwardPacket (Ptr<Packet> packet, Ipv4Address source);
  void ProcessAck (Ptr<Packet> packet, Ipv4Address source);
  void CheckTimeout (uint32_t seq);
  void ScheduleTimeout (uint32_t seq);

  uint32_t m_size; //used if dataSize == 0
  uint32_t m_dataSize;
  uint8_t *m_data;

  RandomVariable m_randomEventDetectionDelay;

  Ipv4Address m_servAddress;
  Ipv4Address m_address;
  Ptr<Socket> m_socket;
  uint16_t m_port;

  Time m_timeout;
  uint8_t m_retries;
  uint32_t m_sent; //# sent
  std::list<EventId> m_events;
  std::set<uint32_t> m_outstandingSeqs;

  /// Callbacks for tracing
  TracedCallback<Ptr<const Packet> > m_sendTrace;
  TracedCallback<Ptr<const Packet> > m_rcvTrace;
};

} // namespace ns3

#endif /* MDC_EVENT_SENSOR_H */

