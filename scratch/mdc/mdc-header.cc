/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "mdc-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MdcHeader");
NS_OBJECT_ENSURE_REGISTERED (MdcHeader);

MdcHeader::MdcHeader ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_data = 0;
  m_xPos = 0;
  m_yPos = 0;
  m_dest = 0;
}

MdcHeader::MdcHeader (Ipv4Address dest)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_data = 0;
  m_xPos = 0;
  m_yPos = 0;
  m_dest = dest.Get ();
}

MdcHeader::MdcHeader (const MdcHeader& original)
{
  NS_LOG_FUNCTION (original.GetDest ());

  m_data = original.m_data;
  m_xPos = original.m_xPos;
  m_yPos = original.m_yPos;
  m_dest = original.m_dest;
  m_origin = original.m_origin;
}

MdcHeader&
MdcHeader::operator=(const MdcHeader& original)
{
  NS_LOG_FUNCTION_NOARGS ();

  MdcHeader temp ((const MdcHeader)original);

  std::swap(m_data, temp.m_data);
  std::swap(m_xPos, temp.m_xPos);
  std::swap(m_yPos, temp.m_yPos);
  std::swap(m_dest, temp.m_dest);
  std::swap(m_origin, temp.m_origin);

  return *this;
}

MdcHeader::~MdcHeader ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId
MdcHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MdcHeader")
    .SetParent<Header> ()
    .AddConstructor<MdcHeader> ()
  ;
  return tid;
}

TypeId
MdcHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

Ipv4Address
MdcHeader::GetDest (void) const
{
  NS_LOG_FUNCTION_NOARGS ();

  return Ipv4Address (m_dest);
}

Ipv4Address
MdcHeader::GetOrigin (void) const
{
  return Ipv4Address (m_origin);
}

void
MdcHeader::SetDestination (Ipv4Address dest)
{
  m_dest = dest.Get ();
}

void
MdcHeader::SetOrigin (Ipv4Address origin)
{
  m_origin = origin.Get ();
}

uint32_t
MdcHeader::GetData (void) const
{
  return m_data;
}

void
MdcHeader::SetData (uint32_t size)
{
  m_data = size;
}

uint32_t
MdcHeader::GetXPosition () const
{
  return m_xPos;
}

uint32_t
MdcHeader::GetYPosition () const
{
  return m_yPos;
}

void
MdcHeader::SetPosition (uint32_t x, uint32_t y)
{
  m_xPos = x;
  m_yPos = y;
}

void
MdcHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of the header.
  os << "Packet from " << Ipv4Address (m_origin) << " to " << Ipv4Address (m_dest);
  if (m_data)
    {
      os << " containing " << m_data << " bytes of sensed data.";
    }
}

uint32_t
MdcHeader::GetSerializedSize (void) const
{
  return MDC_HEADER_SIZE;
}

void
MdcHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION_NOARGS ();

  start.WriteU16 (m_data);
  start.WriteU32 (m_xPos);
  start.WriteU32 (m_yPos);
  start.WriteU32 (m_dest);
  start.WriteU32 (m_origin);
}

uint32_t
MdcHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_data = start.ReadU16 ();
  m_xPos = start.ReadU32 ();
  m_yPos = start.ReadU32 ();
  m_dest = start.ReadU32 ();
  m_origin = start.ReadU32 ();

  // we return the number of bytes effectively read.
  return MDC_HEADER_SIZE;
}

} //namespace ns3
