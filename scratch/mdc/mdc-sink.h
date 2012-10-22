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

#ifndef MDC_SINK_H
#define MDC_SINK_H

#include "ns3/packet-sink.h"
#include <map>
#include "ns3/waypoint-mobility-model.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

/**
 * \ingroup mdc 
 * \defgroup mdc MdcSink
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a MdcSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates 
 * ICMP Port Unreachable errors, receiving applications will be needed.
 */

/**
 * \ingroup mdc
 *
 * \brief Receive and consume traffic generated to an IP address and port
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a MdcSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates 
 * ICMP Port Unreachable errors, receiving applications will be needed.
 *
 * The constructor specifies the Address (IP address and port) and the 
 * transport protocol to use.   A virtual Receive () method is installed 
 * as a callback on the receiving socket.  By default, when logging is
 * enabled, it prints out the size of packets and their address, but
 * we intend to also add a tracing source to Receive() at a later date.
 */
class MdcSink : public Application 
{
public:
  static TypeId GetTypeId (void);
  MdcSink ();

  virtual ~MdcSink ();

  /**
   * \return the total bytes received in this sink app
   */
  uint32_t GetTotalRx () const;

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  void HandleRead (Ptr<Socket>);
  void HandleAccept (Ptr<Socket>, const Address& from);
  void HandlePeerClose (Ptr<Socket>);
  void HandlePeerError (Ptr<Socket>);

  // In the case of TCP, each socket accept returns a new socket, so the 
  // listening socket is stored seperately from the accepted sockets
  Ptr<Socket>     m_mdcSocket;
  Ptr<Socket>     m_sensorSocket;
  std::map<uint32_t, Ptr<Socket> > m_acceptedSockets; //the accepted MDC sockets
  std::map<uint32_t, Ptr<MobilityModel> > m_mobilityModels;

  // Need to keep track of the # expected bytes left in the next packet for each stream
  std::map<Ptr<Socket>, uint32_t> m_expectedBytes;

  Address         m_mdcLocal;        // Local address to bind to for talking with MDCs
  Address         m_sensorLocal;        // Local address to bind to for talking with sensors
  uint32_t        m_totalRx;      // Total bytes received
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

  bool m_waypointRouting;
};

} // namespace ns3

#endif /* MDC_SINK_H */

