/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

/* This program implements a simple network of sensors on a wi-fi network.
 * We also have a MDC traveling in the same space as these sensors.
 * When the MDC comes in proximity to the sensors, the sensor simply transfers the message to the MDC.
 * When the MDC reaches the base station finally, it will offload all the messages it has reached.
 * The simulation ends there.
 */
// Network topology
//
//       s0   s1   s2   sN
//       |    |    |    |
//       .    .    .    .
//       |____|____|____|
//               |          10.1.1.0/255.255.255.0
//       |---------------|
//       |     |     |   |
//       m0    m1   m2  mM (AP)
//       |_____|_____|___|
//                |
//                |         10.1.2.0/255.255.255.0
//                S - Sink
//
//
//
// - Tracing of queues and packet receptions to file "MdcMain....tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/packet-sink.h"
#include "ns3/aodv-module.h"
#include "ns3/energy-module.h"

//#include "ns3/bp-endpoint-id.h"
//#include "ns3/bundle-protocol.h"
//#include "ns3/bp-static-routing-protocol.h"
//#include "ns3/bundle-protocol-helper.h"
//#include "ns3/bundle-protocol-container.h"
#include <iostream>
#include <cmath>

#include "mdc-helper.h"
#include "mdc-event-sensor.h"
#include "mdc-collector.h"
#include "mdc-config.h"
#include "mdc-header.h"
#include "mdc-utilities.h"

#include "ns3/netanim-module.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("MdcMainSimulation");

struct TraceConstData
{
	Ptr<OutputStreamWrapper> outputStream;
	uint32_t nodeId;
};


class MdcMain : public Application
{
public:
	MdcMain();

	/// Configure script parameters, \return true on successful configuration
	bool Configure (int argc, char **argv);
	/// Run simulation
	void Run ();
	/// Report results
	void Report (std::ostream & os);


private:
	// This allows these parameters to be passed and and interpreted on the command line
	uint32_t m_nSensors;
	double   m_mdcSpeed;
	double   m_mdcPause;
	uint32_t m_nMdcs;
	uint32_t m_nEvents;
	uint32_t m_dataSize;
	double   m_eventRadius;
	uint32_t m_boundaryLength;
	std::string m_TraceFile;

	int      m_verbose;
	bool     m_sendFullData;

	/// Simulation time, seconds
	double m_totalSimTime;
	double m_simStartTime;
	double m_simEndTime;

	/// Write per-device PCAP traces if true
	bool m_pcap;

	/// Write application traces if true
	bool m_traces;

	// The singleton class MdcConfig and get its reference
	MdcConfig *mdcConfig;

	Ptr<OutputStreamWrapper> outputStream; // output stream for tracing

	// Collections in the simulation
	NodeContainer sensorNodes;
	NodeContainer mdcNodes;
	NodeContainer sinkNodes;

	NetDeviceContainer wifiSensorDevices;
	NetDeviceContainer wifiMDCDevices;
	NetDeviceContainer sinkNetDevices;
	NetDeviceContainer sinkToMdcDevices; // dedicated devices on sink for sink --> MDC
	NetDeviceContainer mdcToSinkDevices; // dedicated devices on MDC for MDC --> sink

	Ipv4InterfaceContainer wifiSensorInterfaces;
	Ipv4InterfaceContainer wifiMDCInterfaces;
	Ipv4InterfaceContainer sinkInterfaces;
	Ipv4InterfaceContainer sinkToMdcInterfaces; // dedicated interfaces on sink for sink --> MDC
	Ipv4InterfaceContainer mdcToSinkInterfaces; // dedicated interfaces on MDC for MDC --> sink

	Ptr<RandomRectanglePositionAllocator> randomPositionAllocator;
	Ptr<UniformRandomVariable> randomPosX;
	Ptr<UniformRandomVariable> randomPosY;

private:
	void CreateNodes();
	void CreateChannels();
	void SetupMobility();
	void InstallEnergyModel();
	void InstallInternetStack();
	void InstallApplications ();
        void SetupTraces ();
};

//-----------------------------------------------------------------------------
int main (int argc, char **argv)
{
	MdcMain mdcMain;
	if (!mdcMain.Configure (argc, argv))
	NS_FATAL_ERROR ("Configuration failed. Aborted.");

	mdcMain.Run ();
	mdcMain.Report (std::cout);
	return 0;
}



MdcMain::MdcMain ()
{
	NS_LOG_FUNCTION_NOARGS ();

	m_nSensors=4;
	m_nMdcs=1;
	m_mdcSpeed=3.0;
	m_mdcPause=1.0;
	m_totalSimTime=100.0;
	m_pcap=true;
	m_traces=true;
	m_nEvents=1;
	m_dataSize=1024;
	m_eventRadius=5.0;
	m_boundaryLength=100;
	m_TraceFile="MdcMain";
	m_verbose=1;
	m_sendFullData=false;
	m_simStartTime=1.0;
	m_simEndTime=10.0;
}



// Trace function for the Sink
static void
SinkPacketRecvTrace (TraceConstData * constData, Ptr<const Packet> packet, const Address & from)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);
	std::stringstream s;
	Ipv4Address fromAddr = InetSocketAddress::ConvertFrom(from).GetIpv4 ();

	// Ignore MDC's data requests, especially since they'll hit both interfaces and double up...
	// though this shouldn't happen as broadcast should be to a network?
	if (   (head.GetFlags () == MdcHeader::mdcDataForward)
		|| (head.GetFlags () == MdcHeader::mdcDataRequest)
		|| (head.GetFlags () == MdcHeader::sensorDataNotify)
		)
	{
		s << "[SINK__TRACE] IGNORED....."
				<< constData->nodeId
				<< " received ["
				<< head.GetPacketType ()
				<< "] (" << head.GetData () + head.GetSerializedSize ()
				<< "B) from node "
				<< head.GetId ()
				<< " via "
				<< fromAddr
				<< " at "
				<< Simulator::Now ().GetSeconds ();

//		NS_LOG_LOGIC (s.str ());
	}

	if (	(head.GetFlags () == MdcHeader::sensorDataReply)
		)
	{
		// Complete pkt received. Send an indication
		uint32_t expectedPktSize = head.GetData() + head.GetSerializedSize ();
		uint32_t recdPktSize = packet->GetSize();
		std::stringstream s, csv;

		if (recdPktSize < expectedPktSize)
		{
			s.str("");
			csv.str("");
			s		<< "[SINK__TRACE] Sink Node#"
					<< constData->nodeId
					<< " FIRST_SEGMENT ["
					<< head.GetPacketType ()
					<< "] SegmentSize=" << recdPktSize
					<< "B ExpectedFullPacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " via "
					<< fromAddr
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "SINK__TRACE,"
					<< constData->nodeId
					<< ",FIRST_SEGMENT,"
					<< head.GetPacketType ()
					<< "," << recdPktSize
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< fromAddr
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();
			NS_LOG_INFO (s.str ());
			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;
		}
		else if (recdPktSize > expectedPktSize)
		{
			s.str("");
			csv.str("");
			s		<< "[SINK__TRACE] Sink Node#"
					<< constData->nodeId
					<< " FULL_PACKET ["
					<< head.GetPacketType ()
					<< "] CompletePacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " via "
					<< fromAddr
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "SINK__TRACE,"
					<< constData->nodeId
					<< ",FULL_PACKET,"
					<< head.GetPacketType ()
					<< "," << expectedPktSize
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< fromAddr
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();
			NS_LOG_INFO (s.str ());

			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;

			// Reset s here for the next message for the partial pkt recd...
			s.str("");
			csv.str("");
			s		<< "[SINK__TRACE] Sink Node#"
					<< constData->nodeId
					<< " FIRST_SEGMENT ["
					<< head.GetPacketType ()
					<< "] SegmentSize=" << (recdPktSize - expectedPktSize)
					<< "B ExpectedFullPacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " via "
					<< fromAddr
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "SINK__TRACE,"
					<< constData->nodeId
					<< ",FIRST_SEGMENT,"
					<< head.GetPacketType ()
					<< "," << (recdPktSize - expectedPktSize)
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< fromAddr
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();


			NS_LOG_INFO (s.str ());
			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;
		}
		else // the recdPktsize and expectedpacketsize are equal
		{
			s.str("");
			csv.str("");
			s		<< "[SINK__TRACE] Sink Node#"
					<< constData->nodeId
					<< " FULL_PACKET ["
					<< head.GetPacketType ()
					<< "] CompletePacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " via "
					<< fromAddr
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "SINK__TRACE,"
					<< constData->nodeId
					<< ",FULL_PACKET,"
					<< head.GetPacketType ()
					<< "," << expectedPktSize
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< fromAddr
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();
			NS_LOG_INFO (s.str ());
			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;
		}
	}

	return;
}

// Trace function for sensors sending a packet
static void
SensorDataSendTrace (TraceConstData * constData, Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);

	std::stringstream s,csv;
	csv.str("");
	s.str("");
	s << "[SENSORTRACE] "
			<< constData->nodeId
			<< " sent ["
			<< head.GetPacketType ()
			<< "] (" << head.GetData()
			<< "B) at "
			<< Simulator::Now ().GetSeconds ();

	csv << "SENSORTRACE,"
			<< constData->nodeId
			<< ","
			<< head.GetPacketType ()
			<< "," << head.GetData()
			<< ","
			<< Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//	*(constData->outputStream)->GetStream () << s.str() << std::endl;
}

// Trace function for MDC forwarding a packet
static void
MdcPacketFwdTrace (TraceConstData * constData, Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);
	std::stringstream s, csv;


	// Trace only DataReply messages and no others

	if (	(head.GetFlags () == MdcHeader::sensorDataReply)
		)
	{
		// Complete pkt received. Send an indication
		uint32_t expectedPktSize = head.GetData() + head.GetSerializedSize ();
		uint32_t recdPktSize = packet->GetSize();

		if (recdPktSize < expectedPktSize)
		{
			s.str("");
			csv.str("");
			s		<< "[COLL__TRACE] MDC Node#"
					<< constData->nodeId
					<< " FIRST_SEGMENT ["
					<< head.GetPacketType ()
					<< "] SegmentSize=" << recdPktSize
					<< "B ExpectedFullPacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "COLL__TRACE,"
					<< constData->nodeId
					<< ",FIRST_SEGMENT,"
					<< head.GetPacketType ()
					<< "," << recdPktSize
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();
			NS_LOG_INFO (s.str ());
			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;
		}
		else if (recdPktSize > expectedPktSize)
		{
			{
				s.str("");
				csv.str("");
				s		<< "[COLL__TRACE] MDC Node#"
						<< constData->nodeId
						<< " FULL PACKET ["
						<< head.GetPacketType ()
						<< "] CompletePacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "COLL__TRACE,"
						<< constData->nodeId
						<< ",FULL_PACKET,"
						<< head.GetPacketType ()
						<< "," << expectedPktSize
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();
				NS_LOG_INFO (s.str ());
				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//				*(constData->outputStream)->GetStream () << s.str() << std::endl;
			}
			// Reset s here for the next message for the partial pkt recd...
			{
				s.str("");
				csv.str("");
				s		<< "[COLL__TRACE] MDC Node#"
						<< constData->nodeId
						<< " FIRST SEGMENT ["
						<< head.GetPacketType ()
						<< "] SegmentSize=" << (recdPktSize - expectedPktSize)
						<< "B ExpectedFullPacketSize=" << expectedPktSize
						<< "B from "
						<< head.GetOrigin ()
						<< " to "
						<< head.GetDest ()
						<< " at "
						<< Simulator::Now ().GetSeconds ();
				csv		<< "COLL__TRACE,"
						<< constData->nodeId
						<< ",FIRST_SEGMENT,"
						<< head.GetPacketType ()
						<< "," << (recdPktSize-expectedPktSize)
						<< "," << expectedPktSize
						<< ","
						<< head.GetOrigin ()
						<< ","
						<< head.GetDest ()
						<< ","
						<< Simulator::Now ().GetSeconds ();

				NS_LOG_INFO (s.str ());
				*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
	//				*(constData->outputStream)->GetStream () << s.str() << std::endl;
			}
		}
		else // the recdPktsize and expectedpacketsize are equal
		{
			s.str("");
			csv.str("");
			s		<< "[COLL__TRACE] MDC Node#"
					<< constData->nodeId
					<< " FULL PACKET ["
					<< head.GetPacketType ()
					<< "] CompletePacketSize=" << expectedPktSize
					<< "B from "
					<< head.GetOrigin ()
					<< " to "
					<< head.GetDest ()
					<< " at "
					<< Simulator::Now ().GetSeconds ();
			csv		<< "COLL__TRACE,"
					<< constData->nodeId
					<< ",FULL_PACKET,"
					<< head.GetPacketType ()
					<< "," << expectedPktSize
					<< "," << expectedPktSize
					<< ","
					<< head.GetOrigin ()
					<< ","
					<< head.GetDest ()
					<< ","
					<< Simulator::Now ().GetSeconds ();

			NS_LOG_INFO (s.str ());
			*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
//			*(constData->outputStream)->GetStream () << s.str() << std::endl;
		}
	}
}

/// Trace function for remaining energy at node.
static void
RemainingEnergyAtNodeTrace (TraceConstData * constData, double oldValue, double remainingEnergy)
{
  //  NS_LOG_UNCOND ("Node " << constData.nodeId << " current remaining energy = " << remainingEnergy
  std::stringstream s;
  s << "Node " << constData->nodeId << " current remaining energy = " << remainingEnergy
    << "J at time " << Simulator::Now ().GetSeconds ();

    NS_LOG_INFO (s.str ());
	*(GetMDCOutputStream())->GetStream () << s.str() << std::endl;
//  *(constData->outputStream)->GetStream () << s.str() << std::endl;
}

static void
MacTxDrop(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::stringstream s, csv;
	s 		<< "[PACKET_DROP] [MAC/TX]"
			<< " at "
			<< Simulator::Now ().GetSeconds ();
	csv 	<< "PACKET_DROP,MAC/TX,"
			<< Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
////  NS_LOG_INFO("Packet Drop MacTx");
}

static void
PhyTxDrop(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::stringstream s, csv;
	s 		<< "[PACKET_DROP] [PHY/TX]"
			<< " at "
			<< Simulator::Now ().GetSeconds ();
	csv 	<< "PACKET_DROP,PHY/TX,"
			<< Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(GetMDCOutputStream())->GetStream () <<  csv.str() << std::endl;
////  NS_LOG_INFO("Packet Drop PhyTx");
}

static void
PhyRxDrop(Ptr<const Packet> p)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::stringstream s, csv;
	s 		<< "[PACKET_DROP] [PHY/RX]"
			<< " at "
			<< Simulator::Now ().GetSeconds ();
	csv 	<< "PACKET_DROP,PHY/RX,"
			<< Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(GetMDCOutputStream())->GetStream () << csv.str() << std::endl;
////  NS_LOG_INFO("Packet Drop PhyRx");
}

bool
MdcMain::Configure(int argc, char **argv)
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_LOG_FUNCTION("MdcMain Configuration Started.");
	ns3::PacketMetadata::Enable ();
	std::stringstream s;

	/* initialize random seed: */
	srand (time(NULL));

	/* generate a random seed number between 1 and 1000 */
	int randomSeed = rand() % 1000 + 1;
	/* generate a random run number between 1 and 1000 */
	int randomRun = rand() % 1000 + 1;


//	UniformVariable randomSeed(0,999);
//	UniformVariable randomRun(0,999);

	mdcConfig = MdcConfig::GetInstance();
//	SeedManager::SetSeed (randomSeed.GetInteger(0,999));
//	SeedManager::SetRun (randomRun.GetInteger(0,999));
	SeedManager::SetSeed (randomSeed);
	SeedManager::SetRun (randomRun);


	m_nSensors = mdcConfig->GetIntProperty("mdc.Sensors");
	m_nMdcs = mdcConfig->GetIntProperty("mdc.MDCs");
	m_mdcSpeed = mdcConfig->GetDoubleProperty("mdc.MdcSpeed");
	m_mdcPause = mdcConfig->GetDoubleProperty("mdc.MdcPause");
	m_totalSimTime = mdcConfig->GetDoubleProperty("mdc.TotalSimTime");
	m_pcap = mdcConfig->GetBoolProperty("mdc.Pcap");
	m_nEvents = mdcConfig->GetIntProperty("mdc.Events");
	m_dataSize = mdcConfig->GetIntProperty("mdc.DataSize");
	m_eventRadius = mdcConfig->GetDoubleProperty("mdc.EventRadius");
	m_boundaryLength = mdcConfig->GetIntProperty("mdc.Boundary");
	m_traces = mdcConfig->GetBoolProperty("mdc.Traces");
	m_TraceFile = mdcConfig->GetStringProperty("mdc.TraceFile");
	m_verbose = mdcConfig->GetIntProperty("mdc.Verbose");
	m_sendFullData = mdcConfig->GetBoolProperty("mdc.SendFullData");
	m_simStartTime = mdcConfig->GetDoubleProperty("mdc.SimStartTime");
	m_simEndTime = mdcConfig->GetDoubleProperty("mdc.SimEndTime");



	// The following allows the params to get overridden by setting them up on the commandline
	CommandLine cmd;
	cmd.AddValue ("pcap", "Write PCAP trace output.", m_pcap);
	cmd.AddValue ("traces", "Write Sensor traces.", m_traces);
	cmd.AddValue ("time", "Simulation time, s.", m_totalSimTime);
	cmd.AddValue ("nCsma", "Number of \"sensor\" nodes/devices", m_nSensors);
	cmd.AddValue ("nWifi", "Number of wifi MDC devices", m_nMdcs);
	cmd.AddValue ("mdcSpeed", "Speed of the MDC devices", m_mdcSpeed);
	cmd.AddValue ("mdcPause", "Pause duration of the MDC devices", m_mdcPause);
	cmd.Parse(argc, argv); // Parse the commandline and see if the parameters have been passed.


	m_nSensors = m_nSensors == 0 ? 1 : m_nSensors; // Making sure you have atleast one node in the sensor network.
	m_nMdcs = m_nMdcs == 0 ? 1 : m_nMdcs; // Making sure you have atleast one node in the sensor network.
	cmd.Parse (argc, argv);


	if (m_TraceFile == "")
		m_TraceFile = "traceOutput.txt";

	if (m_pcap) m_traces = true; // You may want to set this parameter in MdcConfig.xml

	AsciiTraceHelper asciiTraceHelper;
	outputStream = asciiTraceHelper.CreateFileStream (m_TraceFile);
	SetMDCOutputStream(outputStream);

	double posBound = m_boundaryLength;
	double negBound = 0;
	randomPosX = CreateObject<UniformRandomVariable> ();
	randomPosX->SetAttribute ("Max", DoubleValue (posBound));
	randomPosX->SetAttribute ("Min", DoubleValue (negBound));

	randomPosY = CreateObject<UniformRandomVariable> ();
	randomPosY->SetAttribute ("Max", DoubleValue (posBound));
	randomPosY->SetAttribute ("Min", DoubleValue (negBound));

	randomPositionAllocator = CreateObject<RandomRectanglePositionAllocator> ();
	randomPositionAllocator->SetX (randomPosX);
	randomPositionAllocator->SetY (randomPosY);



	s << "Configuration Parameters:- \n" << "    " <<
			m_nSensors << "--> mdc.Sensors \n" << "    " <<
			m_nMdcs << "--> mdc.MDCs \n" << "    " <<
			m_mdcSpeed << "--> mdc.MdcSpeed \n" << "    " <<
			m_mdcPause << "--> mdc.MdcPause \n" << "    " <<
			m_totalSimTime << "--> mdc.TotalSimTime \n" << "    " <<
			m_pcap << "--> mdc.Pcap \n" << "    " <<
			m_traces << "--> mdc.Traces \n" << "    " <<
			m_nEvents << "--> mdc.Events \n" << "    " <<
			m_dataSize << "--> mdc.DataSize \n" << "    " <<
			m_eventRadius << "--> mdc.EventRadius \n" << "    " <<
			m_boundaryLength << "--> mdc.Boundary \n" << "    " <<
			m_TraceFile << "--> mdc.TraceFile \n" << "    " <<
			m_verbose << "--> mdc.Verbose \n" << "    " <<
			m_sendFullData << "--> mdc.SendFullData \n" << "    " <<
			m_simStartTime << "--> mdc.SimStartTime \n" << "    " <<
			m_simEndTime << "--> mdc.SimEndTime \n"<< "    " <<
			posBound << "--> posBound \n"<< "    " <<
			negBound << "--> negBound \n"
			;

	NS_LOG_INFO(s.str());
	return true;
}

void
MdcMain::Run()
{
	NS_LOG_FUNCTION_NOARGS ();

	CreateNodes();
	CreateChannels();
	InstallInternetStack();
	SetupMobility();
	// InstallEnergyModel(); TODO: This aspect has not been enabled for our tests
	InstallApplications();
        SetupTraces();

	//
	// Now, do the actual simulation.
	//
	std::stringstream s;
	s << "\nStarting simulation for " << m_totalSimTime << " s ...";

	NS_LOG_INFO (s.str());

	Simulator::Stop (Seconds (m_totalSimTime));

	AnimationInterface anim ("pc3Test1.xml");
	anim.SetMobilityPollInterval (Seconds (1));
	anim.EnablePacketMetadata (true);

	anim.SetConstantPosition (mdcNodes.Get(0), m_boundaryLength/2.0, m_boundaryLength/2.0);

	Simulator::Run ();
	Simulator::Destroy ();

	NS_LOG_INFO ("MdcMain Run Ended.");
    return;

}

void
MdcMain::Report (std::ostream &)
{
	NS_LOG_FUNCTION("MdcMain Report.");
}

void
MdcMain::CreateNodes()
{
	NS_LOG_FUNCTION_NOARGS();
	NS_LOG_LOGIC ("Create Sensor nodes.");
	sensorNodes.Create (m_nSensors);

	//
	// Explicitly create the MDCs.
	// Each of the MDCs are APs. So we are assuming that they will all have the same SSID
	// Somehow the sensor nodes will attach themselves to the closest AP as it comes along
	//
	NS_LOG_LOGIC ("Create MDC nodes.");
	mdcNodes.Create (m_nMdcs);

	//
	// Create the Sink.
	//
	NS_LOG_LOGIC ("Create Sink nodes.");
	sinkNodes.Create (1);

}

void
MdcMain::CreateChannels()
{
	NS_LOG_FUNCTION ("MdcMain Create Channels Started.");
	YansWifiChannelHelper ywifiChHlpr;
	YansWifiPhyHelper ywifiPhyHlpr;
	NqosWifiMacHelper nqosWiMacHlpr;
	WifiHelper wifiHlpr;
	Ssid ssid;

	NS_LOG_LOGIC ("Create Sensor channels.");
	// Setup the helper objects to create the Wifi connection
	ywifiChHlpr = YansWifiChannelHelper::Default();
	ywifiPhyHlpr = YansWifiPhyHelper::Default();
	ywifiPhyHlpr.SetChannel(ywifiChHlpr.Create()); // This sets up the PHY Layer with the channel
	nqosWiMacHlpr = NqosWifiMacHelper::Default();

	// setup the Station Manager software on the AP first
	wifiHlpr = WifiHelper::Default ();
	wifiHlpr.SetRemoteStationManager ("ns3::AarfcdWifiManager");
	/// wifiHlpr.SetRemoteStationManager ("ns3::AarfWifiManager");
	/// // Let us complete the MAC layer of the SENSOR nodes... these are typical STA nodes in a Wifi n/w.
	/// ssid = Ssid ("MDC-SSID"); // This is the SSID of the AP that the MACs will be configured with
	/// nqosWiMacHlpr.SetType("ns3::StaWifiMac",   //<-- This is where you tell the MAC to create a STA
	/// 		"Ssid", SsidValue (ssid),
	/// 		"ActiveProbing", BooleanValue (false));
	// Install the MAC and PHY layer on the STA nodes. These nodes do not send any beacons
	wifiSensorDevices = wifiHlpr.Install(ywifiPhyHlpr, nqosWiMacHlpr, sensorNodes);


	NS_LOG_LOGIC ("Create MDC channels.");
	// You are left with the AP node now.
	// Set the MAC helper object to now create a ApWifiMac
	/// nqosWiMacHlpr.SetType ("ns3::ApWifiMac", //<-- Here you tell it to create an AP.
	/// 		"Ssid", SsidValue (ssid),
	/// 		"BeaconGeneration", BooleanValue (true),
	/// 		"BeaconInterval", TimeValue (Seconds (2.5)));
	// Install the MAC and PHY layer on the AP nodes...
	wifiMDCDevices = wifiHlpr.Install(ywifiPhyHlpr, nqosWiMacHlpr, mdcNodes);
	// Install the same MAC and PHY on the backend network
	mdcToSinkDevices = wifiHlpr.Install(ywifiPhyHlpr, nqosWiMacHlpr, mdcNodes);


	NS_LOG_LOGIC ("Create Sink channels.");
	// You are left with the Sink node now.
	// TODO: Ideally, we want the MDC and the Sink to communicate via a different channel.
	// For now... Set the MAC helper object to now create a StaWifiMac
	/// nqosWiMacHlpr.SetType("ns3::StaWifiMac",   //<-- This is where you tell the MAC to create a STA
	/// 		"Ssid", SsidValue (ssid),
	/// 		"ActiveProbing", BooleanValue (false));
	// Install the MAC and PHY layer on the Sink nodes... in this case just 1
	sinkNetDevices = wifiHlpr.Install(ywifiPhyHlpr, nqosWiMacHlpr, sinkNodes);
	// Install the same MAC and PHY on the backend network
	sinkToMdcDevices = wifiHlpr.Install(ywifiPhyHlpr, nqosWiMacHlpr, sinkNodes);


	if (m_pcap)
	{
		ywifiPhyHlpr.EnablePcapAll (std::string ("mdcTrc"));
	}


}

void
MdcMain::InstallInternetStack()
{
	NS_LOG_FUNCTION("MdcMain Install Internet Stack Started.");

	// At this point we are worried about applying the Internet stack on all the nodes
	// Apply a stack on the nodes so that they use the IP to communicate with the devices.

	// TODO: Check with Kyle... we do not have separate network...
	// all are on the same Wifi in the MDC-simulation code
	InternetStackHelper istkHlpr;
	AodvHelper aodvHlpr;
	aodvHlpr.Set ("EnableHello", BooleanValue (false));
	aodvHlpr.Set ("EnableBroadcast", BooleanValue (false));
	// TODO: Discuss this with Kyle
	// For some reason, static routing is necessary for the MDCs to talk to the sink.
	// Something to do with them having 2 interfaces, as the issue goes away if one is
	// removed, even though the sink still has 2...
	Ipv4StaticRoutingHelper staticRoutingHlpr;
	Ipv4ListRoutingHelper listRoutingHlpr;
	listRoutingHlpr.Add (staticRoutingHlpr, 5);
	listRoutingHlpr.Add (aodvHlpr, 1);
	istkHlpr.SetRoutingHelper (listRoutingHlpr);


	istkHlpr.Install(sensorNodes);
	istkHlpr.Install(mdcNodes);
	istkHlpr.Install(sinkNodes);

	//
	// We've got the "hardware" in place.  Now we need to add IP addresses.
	//
	NS_LOG_LOGIC ("Assign IP Addresses.");
	Ipv4AddressHelper ipv4Hlpr;
	ipv4Hlpr.SetBase ("10.1.1.0", "255.255.255.0");
	wifiSensorInterfaces = ipv4Hlpr.Assign (wifiSensorDevices);
	wifiMDCInterfaces = ipv4Hlpr.Assign (wifiMDCDevices);
	sinkInterfaces = ipv4Hlpr.Assign (sinkNetDevices);

	// The long-range wifi between sink and MDCs uses a different network
	ipv4Hlpr.NewNetwork ();
	sinkToMdcInterfaces = ipv4Hlpr.Assign (sinkToMdcDevices);
	mdcToSinkInterfaces = ipv4Hlpr.Assign (mdcToSinkDevices);

}

void
MdcMain::SetupMobility()
{
	NS_LOG_FUNCTION ("MdcMain Setup Mobility Started.");
	// Now apply the mobility model
	// The Sensor nodes do not move in this simulation.
	// Their position is pre-determined on a grid which we can assume
	//   to be the bounds of the mobility of the MDC
	MobilityHelper mobHlpr;

	Vector center = Vector3D (m_boundaryLength/2, m_boundaryLength/2, 0.0);
	Vector sinkPos = Vector3D (m_boundaryLength, m_boundaryLength, 0.0);

	NS_LOG_LOGIC ("Assign the Mobility Model to the sink.");
	// Place sink in center of region
	Ptr<ListPositionAllocator> centerPositionAllocator = CreateObject<ListPositionAllocator> ();
	centerPositionAllocator->Add (sinkPos);
	mobHlpr.SetPositionAllocator (centerPositionAllocator);
	mobHlpr.Install (sinkNodes);






	NS_LOG_LOGIC ("Assign the Mobility Model to the sensors.");
	Vector ns3SenPos;
	std::vector<Vector> posVector;
	for (uint32_t i=0; i< m_nSensors; i++)
	{
		randomPositionAllocator->SetX (randomPosX);
		randomPositionAllocator->SetY (randomPosY);
		ns3SenPos = Vector3D(randomPosX->GetValue(), randomPosY->GetValue(), 0.0);
		std::cout << "Random Sensor Position " << i << " = [" << ns3SenPos.x << "," << ns3SenPos.y << "," << ns3SenPos.z << "].\n";
		posVector.push_back(ns3SenPos);
	}


	// std::cout << "PosVector has " << posVector.size() << "entries.\n";
	// Now Sort the posVector starting from the center
	std::cout << "Center = [" << center.x << "," << center.y << "," << center.z << "].\n";
	// Get the closest sensor position
	ns3SenPos = GetClosestVector(posVector, center);
	std::cout << "Closest sensor to center = [" << ns3SenPos.x << "," << ns3SenPos.y << "," << ns3SenPos.z << "].\n";
	std::queue<unsigned> tempOrder = NearestNeighborOrder (&posVector, ns3SenPos);
	//std::cout << " - tempOrder has " << tempOrder.size() << "entries.\n";
	std::vector<Vector> posVectorSorted = ReSortInputVector (&posVector, tempOrder);
	//std::cout << "PosVectorSorted has " << posVectorSorted.size() << "entries.\n";

	Ptr<ListPositionAllocator> sensorListPosAllocator = CreateObject<ListPositionAllocator> ();
	for (uint32_t i=0; i< m_nSensors; i++)
	{
		sensorListPosAllocator->Add(posVectorSorted[i]);
	}

	//for (uint32_t i=0; i< m_nSensors; i++)
	//{
	//	std::cout << "Sorted Sensor Position " << i << " = [" << posVectorSorted[i].x << "," << posVectorSorted[i].y << "," << posVectorSorted[i].z << "].\n";
	//}

	// sensorListPosAllocator should have the node positions
	mobHlpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobHlpr.SetPositionAllocator (sensorListPosAllocator);
	mobHlpr.Install (sensorNodes);


	// The sensors should be positioned at random now within the x y bounds.
	// The positions are all recorded hopefully in the sensorListPosAllocator


	NS_LOG_LOGIC ("Assign the Mobility Model to the MDCs.");
	// What is left is the Mobility model for the MDCs.
	// Place MDCs in center of region
	Ptr<ConstantRandomVariable> constRandomSpeed = CreateObject<ConstantRandomVariable> ();
	constRandomSpeed->SetAttribute ("Constant", DoubleValue (m_mdcSpeed));
	Ptr<ConstantRandomVariable> constRandomPause = CreateObject<ConstantRandomVariable> (); //default = 0
	constRandomPause->SetAttribute ("Constant", DoubleValue (m_mdcPause));

	mobHlpr.SetPositionAllocator (randomPositionAllocator);
	mobHlpr.SetMobilityModel ("ns3::RandomWaypointMobilityModel"
							 ,"Pause", PointerValue (constRandomPause)
							 ,"Speed", PointerValue (constRandomSpeed)
							 ,"PositionAllocator", PointerValue (sensorListPosAllocator) // This uses NearestNeighbor
//							 ,"PositionAllocator", PointerValue (randomPositionAllocator) // This is random
							 );
	mobHlpr.Install (mdcNodes);

	// Now position all the MDCs in the center of the grid. This is the INITIAL position.
	for (NodeContainer::Iterator it = mdcNodes.Begin ();
	it != mdcNodes.End (); it++)
	{
		(*it)->GetObject<MobilityModel>()->SetPosition (center);
	}

}


void
MdcMain::InstallEnergyModel()
{
	// InstallEnergyModel(); TODO: This aspect has not been enabled for our tests
	//////////////////////////////////////////////////////////////////////
	////////////////////      Energy Model      //////////////////////////
	//////////////////////////////////////////////////////////////////////


	//BasicEnergySourceHelper basicSourceHelper;
	//basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initEnergy));
	//EnergySourceContainer energySources = basicSourceHelper.Install (sensors);

	LiIonEnergySourceHelper energySourceHelper;
	//energySourceHelper.Set ("LiIonEnergySourceInitialEnergyJ", DoubleValue (initEnergy));
	//energySourceHelper.Set ("TypCurrent", DoubleValue ());
	energySourceHelper.Set ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (10.)));
	EnergySourceContainer energySources = energySourceHelper.Install (sensorNodes);

	// if not using a helper (such as for the SimpleDeviceEnergyModel that can be used to mimic a processor/sensor), you must do
	// Ptr<DeviceEnergyModel> energyModel = CreateObject<SimpleDeviceEnergyModel> ();
	// energyModel->SetCurrentA (WHATEVER);
	// energyModel->SetEnergySource (energySource);
	// and energySource->AppendDeviceEnergyModel (energyModel);

	WifiRadioEnergyModelHelper radioEnergyHelper;
	radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
	DeviceEnergyModelContainer deviceEnergyModels = radioEnergyHelper.Install (wifiSensorDevices, energySources);

	if (m_traces)
	{
		for (EnergySourceContainer::Iterator itr = energySources.Begin ();
		itr != energySources.End (); itr++)
		{
			TraceConstData * constData = new TraceConstData();
			constData->outputStream = outputStream;
			constData->nodeId = (*itr)->GetNode ()->GetId ();

			Ptr<LiIonEnergySource> energySource = DynamicCast<LiIonEnergySource> (*itr);
			NS_ASSERT (energySource != NULL);

			energySource->TraceConnectWithoutContext ("RemainingEnergy", MakeBoundCallback (&RemainingEnergyAtNodeTrace, constData));
		}
	}
}

void
MdcMain::InstallApplications()
{
	NS_LOG_FUNCTION("MdcMain Install Applications Started.");
	TraceConstData *constData;
	// SINK   ---   TCP sink for data from MDCs, UDP sink for data/notifications directly from sensors

	MdcSinkHelper sinkHelper;
	ApplicationContainer sinkApps = sinkHelper.Install (sinkNodes);
	sinkApps.Start (Seconds (m_simStartTime));
	sinkApps.Stop (Seconds (m_simEndTime));
	if (m_traces)
	{
		constData = new TraceConstData();
		constData->outputStream = outputStream;
		constData->nodeId = sinkNodes.Get (0)->GetId (); // There will be just 1 sink node
		sinkApps.Get (0)->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&SinkPacketRecvTrace, constData));
	}

	// MOBILE DATA COLLECTORS
	//BulkSendHelper mdcAppHelper ("ns3::TcpSocketFactory", sinkDestAddress);
	MdcCollectorHelper mdcAppHelper;
	mdcAppHelper.SetAttribute ("RemoteAddress", Ipv4AddressValue (Ipv4Address::ConvertFrom (sinkToMdcInterfaces.GetAddress (0))));
	mdcAppHelper.SetAttribute ("Interval", TimeValue (Seconds (3.0)));
	//mdcAppHelper.SetAttribute ("Port", UIntegerValue (9999));
	ApplicationContainer mdcApps = mdcAppHelper.Install (mdcNodes);
	mdcApps.Start (Seconds (m_simStartTime));
	mdcApps.Stop (Seconds (m_simEndTime));


	/*
	* This tracing could be done in the helper itself but we can leave it as it is now
	*/

	if (m_traces)
	{
		for (uint8_t i = 0; i < mdcApps.GetN (); i++)
		{
			constData = new TraceConstData();
			constData->outputStream = outputStream;
			constData->nodeId = mdcApps.Get (i)->GetNode ()->GetId ();
			mdcApps.Get (i)->TraceConnectWithoutContext ("Forward", MakeBoundCallback (&MdcPacketFwdTrace, constData));
		}
	}

	// SENSORS
	MdcEventSensorHelper sensorAppHelper (sinkInterfaces.GetAddress (0, 0), m_nEvents);
	sensorAppHelper.SetAttribute ("PacketSize", UintegerValue (m_dataSize));
	sensorAppHelper.SetAttribute ("SendFullData", BooleanValue (m_sendFullData));
	sensorAppHelper.SetEventPositionAllocator (randomPositionAllocator);
	sensorAppHelper.SetRadiusRandomVariable (new ConstantVariable (m_eventRadius));

	/*
	* The sensor app here then creates all the events. Again, this might be done in an event helper class but keep this for now.
	*/

	if (m_nEvents > 1)
		sensorAppHelper.SetEventTimeRandomVariable (new UniformVariable (m_simStartTime, m_simEndTime));
	else
		sensorAppHelper.SetEventTimeRandomVariable (new ConstantVariable (m_simStartTime*2));

	ApplicationContainer sensorApps = sensorAppHelper.Install (sensorNodes);
	sensorApps.Start (Seconds (m_simStartTime));
	sensorApps.Stop (Seconds (m_simEndTime));

	/*
	* Verbose tracing like before done for MDCs.
	*/
	for (ApplicationContainer::Iterator itr = sensorApps.Begin ();
	itr != sensorApps.End (); itr++)
	{
		if (m_traces)
		{
			constData = new TraceConstData();
			constData->outputStream = outputStream;
			constData->nodeId = (*itr)->GetNode ()->GetId ();

			(*itr)->TraceConnectWithoutContext ("Send", MakeBoundCallback (&SensorDataSendTrace, constData));
		}

	}

}

void MdcMain::SetupTraces () {
  // Trace Collisions
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));
}
