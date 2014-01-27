/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MDC_HEADER_H
#define MDC_HEADER_H

#include "ns3/ptr.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/address.h"
#include <iostream>

namespace ns3 {

/* A Header for the Mobile Data Collector (MDC) sensors, collectors, and sinks.
 */
class MdcHeader : public Header 
{
public:
  enum Flags// : uint16_t
    {
	  // TODO: Remove the sensorDataNotify and sensorFullData...
	  // or maybe enhance the sensorDataNotify atleast to somehow indicate that it does have some data.
	  // currently, we are assuming that the sink somehow knows about all global events.
	  // We expect the sensors to just respond to mdcDataRequest if it is within proximity
      sensorDataNotify, // Sensor telling Sink(in the implementation) that it has data to send... maybe we change this to Sensor sed=nding this message to MDC
      sensorFullData, // Sensor sends all the data to the Sink directly. This gets done only if sendFullData bool is true
      //-------------------------------------------------------
      sensorDataReply, // Sensor responds to MDC with data
      mdcDataRequest, // MDC Broadcasts the beacon
      mdcDataForward, // MDC to Sink
      sinkRouteUpdate // **** Not implemented
    };

  explicit MdcHeader ();
  MdcHeader (Ipv4Address dest, Flags flags);
  virtual ~MdcHeader ();
  MdcHeader (const MdcHeader& original);
  MdcHeader& operator=(const MdcHeader& original);

  void SetDest (Ipv4Address dest);
  void SetOrigin (Ipv4Address origin);
  Ipv4Address GetDest (void) const;
  Ipv4Address GetOrigin (void) const;

  uint32_t GetData (void) const; // Returns the size of the message
  void SetData (uint32_t size); // Sets the size of the message
  uint32_t GetSeq () const;
  void SetSeq (uint32_t newSeq);
  uint32_t GetXPosition () const;
  uint32_t GetYPosition () const;
  void SetPosition (uint32_t x, uint32_t y);
  uint16_t GetId () const;
  void SetId (uint16_t id);
  std::string GetPacketType () const;
  void SetFlags (Flags newFlags);
  Flags GetFlags () const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  Flags m_flags;		// Indicates the type of the message
  uint16_t m_data; 		// Size of the data packet TODO: Call it dataSize
  uint16_t m_id;		// Unique identifier for the node sending the message
  uint32_t m_xPos;		// ?? X-Position value that can be used as appropriate
  uint32_t m_yPos;		// ?? Y-Position value that can be used as appropriate
  uint32_t m_dest;		// Host Order Destination ipv4 address
  uint32_t m_origin;	// Host Order Source ipv4 address
  uint32_t m_seq;		// Sequence number of the message sent by the node
};

#define MDC_HEADER_SIZE 26

} //namespace ns3

#endif
