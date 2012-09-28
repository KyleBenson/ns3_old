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

#ifndef MDC_COLLECTOR_H
#define MDC_COLLECTOR_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"

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
 * \brief A Resilient Overlay Network client
 *
 * Attempts to send a packet to the server.  If failed, it will try to send it through
 * one of the overlay nodes.  It will try to forward packets from other overlay nodes
 * to the server.
 */
class MdcCollector : public Application 
{
public:
  static TypeId GetTypeId (void);
  MdcCollector ();
  //virtual ~MdcCollector ();

  /**
   * \param ip destination ipv4 address
   * \param port destination port
   */
  void SetSink (Ipv4Address ip, uint16_t port = 0);

  Ipv4Address GetAddress () const;

  //TODO: api for controlling movement??

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void Send ();
  void ScheduleTransmit (Time dt);
  void CancelEvents (void);

  void ForwardPacket (Ptr<Packet> packet);

  Time m_interval; //for sending request beacons
  Ipv4Address m_sinkAddress;
  Ipv4Address m_address;
  Ptr<Socket> m_sinkSocket;
  Ptr<Socket> m_sensorSocket;
  uint16_t m_port;
  std::list<EventId> m_events;

  /// Callbacks for tracing
  TracedCallback<Ptr<const Packet>, uint32_t> m_requestTrace;
  TracedCallback<Ptr<const Packet>, uint32_t> m_forwardTrace;
};

} // namespace ns3

#endif /* MDC_COLLECTOR_H */

