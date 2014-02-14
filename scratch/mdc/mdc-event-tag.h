/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MDC_EVENT_TAG_H
#define MDC_EVENT_TAG_H

/* A Tag to allow access to the event identity as captured on the Mobile Data Collector (MDC) sensors and passed on as a packet payload.
 */
#include "sensed-event.h"
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>

namespace ns3 {


	// define this class in a public tag/header
	class MdcEventTag :
		public Tag
	{
	public:
		MdcEventTag (SensedEvent event);
		explicit MdcEventTag ();
		virtual ~MdcEventTag ();
		MdcEventTag (const MdcEventTag& original);
		MdcEventTag& operator=(const MdcEventTag& original);


		static TypeId GetTypeId (void);
		virtual TypeId GetInstanceTypeId (void) const;
		virtual uint32_t GetSerializedSize (void) const;
		virtual void Serialize (TagBuffer i) const;
		virtual void Deserialize (TagBuffer i);
		virtual void Serialize (Buffer::Iterator i) const;
		virtual uint32_t Deserialize (Buffer::Iterator i);
		virtual void Print (std::ostream &os) const;

		// these are our accessors to our tag structure
		void SetEventId (uint32_t value);
		uint32_t GetEventId (void) const;
//		void SetCenterX (double value);
//		double GetCenterX (void) const;
//		void SetCenterY (double value);
//		double GetCenterY (void) const;
//		void SetCenterZ (double value);
//		double GetCenterZ (void) const;
//		void SetRadius (double value);
//		double GetRadius (void) const;
		void SetTime (double value);
		double GetTime (void) const;
	private:
		uint32_t m_eventId;
//		double m_centerX;
//		double m_centerY;
//		double m_centerZ;
//		double m_radius;
		double m_time;
	};

} //namespace ns3

#endif
