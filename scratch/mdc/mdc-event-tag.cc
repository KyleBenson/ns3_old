/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "mdc-event-tag.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("MdcEventTag");
	NS_OBJECT_ENSURE_REGISTERED (MdcEventTag);

	MdcEventTag::MdcEventTag ()
	{
		NS_LOG_FUNCTION_NOARGS ();

		m_eventId = 0;
		/*
		m_centerX = 0;
		m_centerY = 0;
		m_centerZ = 0;
		m_radius = 0;*/
		m_time = 0;
	}

	MdcEventTag::MdcEventTag (SensedEvent event)
	{
		NS_LOG_FUNCTION_NOARGS ();

		m_eventId = event.GetEventId();
		/*
		m_centerX = event.GetCenter().x;
		m_centerY = event.GetCenter().y;
		m_centerZ = event.GetCenter().z;
		m_radius = event.GetRadius(); */
		m_time = event.GetTime().GetSeconds();

	}

	MdcEventTag::MdcEventTag (const MdcEventTag& original)
	{
		NS_LOG_FUNCTION (original.GetEventId());

		m_eventId = original.GetEventId();
		/*
		m_centerX = original.GetCenterX();
		m_centerY = original.GetCenterY();
		m_centerZ = original.GetCenterZ();
		m_radius = original.GetRadius();*/
		m_time = original.GetTime();
	}

	MdcEventTag&
	MdcEventTag::operator=(const MdcEventTag& original)
	{
		NS_LOG_FUNCTION_NOARGS ();

	//	MdcEventTag temp ((const MdcEventTag)original);

		m_eventId = original.GetEventId();
		/*
		m_centerX = original.GetCenterX();
		m_centerY = original.GetCenterY();
		m_centerZ = original.GetCenterZ();
		m_radius = original.GetRadius(); */
		m_time = original.GetTime();
		return *this;
	}

	MdcEventTag::~MdcEventTag ()
	{
		NS_LOG_FUNCTION_NOARGS ();
	}

	TypeId
	MdcEventTag::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::MdcEventTag")
				.SetParent<Tag> ()
				.AddConstructor<MdcEventTag> ()
		;
		return tid;
	}

	TypeId
	MdcEventTag::GetInstanceTypeId (void) const
	{
		return GetTypeId ();
	}

	uint32_t
	MdcEventTag::GetSerializedSize (void) const
	{
//		return (4+(5*sizeof(double))); // =44
		return (4+sizeof(double)); // =12
	}

	void
	MdcEventTag::Serialize (TagBuffer start) const
	{
		NS_LOG_FUNCTION_NOARGS ();

		start.WriteU32(m_eventId);
		/*
		start.WriteDouble(m_centerX);
		start.WriteDouble(m_centerY);
		start.WriteDouble(m_centerZ);
		start.WriteDouble(m_radius); */
		start.WriteDouble(m_time);
	}

	void
	MdcEventTag::Deserialize (TagBuffer start)
	{
		NS_LOG_FUNCTION_NOARGS ();

		m_eventId = start.ReadU32();
		/*
		m_centerX = start.ReadDouble();
		m_centerY = start.ReadDouble();
		m_centerZ = start.ReadDouble();
		m_radius = start.ReadDouble(); */
		m_time = start.ReadDouble();

	}


	void
	MdcEventTag::Serialize (Buffer::Iterator start) const
	{
		NS_LOG_FUNCTION_NOARGS ();

		start.WriteU32(m_eventId);
		/*
		start.WriteU64(m_centerX);
		start.WriteU64(m_centerY);
		start.WriteU64(m_centerZ);
		start.WriteU64(m_radius);*/
		start.WriteU64(m_time);
	}

	uint32_t
	MdcEventTag::Deserialize (Buffer::Iterator start)
	{
		NS_LOG_FUNCTION_NOARGS ();

		m_eventId = start.ReadU32();
		/*
		m_centerX = start.ReadU64();
		m_centerY = start.ReadU64();
		m_centerZ = start.ReadU64();
		m_radius = start.ReadU64(); */
		m_time = start.ReadU64();

		return GetSerializedSize();

	}


	void
	MdcEventTag::Print (std::ostream &os) const
	{
		// This method is invoked by the packet printing
		// routines to print the content of the header.

		os << "{Event Details: Id=" << m_eventId
//			<< " Center=[" << m_centerX << "," << m_centerY << "," << m_centerZ << "]"
//			<< " Radius=" << m_radius
			<< " Time=" << m_time << "}" << std::endl;
	}



	void MdcEventTag::SetEventId (uint32_t value) {	m_eventId = value;}
/*
	void MdcEventTag::SetCenterX (double value) {	m_centerX = value;}

	void MdcEventTag::SetCenterY (double value) {	m_centerY = value;}

	void MdcEventTag::SetCenterZ (double value) {	m_centerZ = value;}

	void MdcEventTag::SetRadius (double value) {	m_radius = value;}
*/
	void MdcEventTag::SetTime (double value) {	m_time = value;}


	uint32_t MdcEventTag::GetEventId (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_eventId;
	}
/*
	double MdcEventTag::GetCenterX (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_centerX;
	}

	double MdcEventTag::GetCenterY (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_centerY;
	}

	double MdcEventTag::GetCenterZ (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_centerZ;
	}


	double MdcEventTag::GetRadius (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_radius;
	}

*/
	double MdcEventTag::GetTime (void) const
	{
		NS_LOG_FUNCTION_NOARGS ();
		return m_time;
	}


} //namespace ns3
