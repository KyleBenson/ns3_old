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
  explicit MdcHeader ();
  MdcHeader (Ipv4Address dest);
  virtual ~MdcHeader ();
  MdcHeader (const MdcHeader& original);
  MdcHeader& operator=(const MdcHeader& original);

  void SetDest (Ipv4Address dest);
  void SetOrigin (Ipv4Address origin);
  Ipv4Address GetDest (void) const;
  Ipv4Address GetOrigin (void) const;

  uint32_t GetData (void) const;
  void SetData (uint32_t size);
  uint32_t GetSeq () const;
  void SetSeq (uint32_t newSeq);
  uint32_t GetXPosition () const;
  uint32_t GetYPosition () const;
  void SetPosition (uint32_t x, uint32_t y);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  uint16_t m_data;
  uint32_t m_xPos;
  uint32_t m_yPos;
  uint32_t m_dest;
  uint32_t m_origin;
  uint32_t m_seq;
};

#define MDC_HEADER_SIZE 22

} //namespace ns3

#endif
