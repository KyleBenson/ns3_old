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
//       s0   s1   s2   s3
//       |    |    |    |
//       .    .    .    .     wifi link: 500Kbps, 5ms
//       |    |    |    |
//
//              m0(AP)-------P2P-----------s0
//
// - Flow from s0/1/2/3 to m0 using bundle protocol.
//
//
// - Tracing of queues and packet receptions to file "PC3Test1....tr"
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



using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("PC3SimpleExample");

struct TraceConstData
{
	Ptr<OutputStreamWrapper> outputStream;
	uint32_t nodeId;
};


class PC3Test1 : public Application
{
public:
	PC3Test1();

	/// Configure script parameters, \return true on successful configuration
	bool Configure (int argc, char **argv);
	/// Run simulation
	void Run ();
	/// Report results
	void Report (std::ostream & os);


private:
	// This allows these parameters to be passed and and interpreted on the command line
	uint32_t _nSensors;
	double   _mdcSpeed;
	double   _mdcPause;
	uint32_t _nMdcs;
	uint32_t _nEvents;
	uint32_t _dataSize;
	double   _eventRadius;
	uint32_t _boundaryLength;
	std::string _TraceFile;

	int      _verbose;
	bool     _sendFullData;

	/// Simulation time, seconds
	double _totalSimTime;
	double _simStartTime;
	double _simEndTime;

	/// Write per-device PCAP traces if true
	bool _pcap;

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
	void InstallInternetStack();
	void InstallApplications ();
};

//-----------------------------------------------------------------------------
int main (int argc, char **argv)
{
	PC3Test1 PC3Test1;
	if (!PC3Test1.Configure (argc, argv))
	NS_FATAL_ERROR ("Configuration failed. Aborted.");

	PC3Test1.Run ();
	PC3Test1.Report (std::cout);
	return 0;
}



PC3Test1::PC3Test1 ()
{
	NS_LOG_FUNCTION_NOARGS ();

	_nSensors=4;
	_nMdcs=1;
	_mdcSpeed=3.0;
	_mdcPause=1.0;
	_totalSimTime=100.0;
	_pcap=true;
	_nEvents=1;
	_dataSize=1024;
	_eventRadius=5.0;
	_boundaryLength=100;
	_TraceFile="PC3Test1";
	_verbose=1;
	_sendFullData=false;
	_simStartTime=1.0;
	_simEndTime=10.0;
}



// Trace function for the Sink
static void
SinkPacketRecvTrace (TraceConstData * constData, Ptr<const Packet> packet, const Address & from)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);

	// Ignore MDC's data requests, especially since they'll hit both interfaces and double up...
	// though this shouldn't happen as broadcast should be to a network?
	if (head.GetFlags () == MdcHeader::mdcDataRequest)
		return;

	std::stringstream s;
	Ipv4Address fromAddr = InetSocketAddress::ConvertFrom(from).GetIpv4 ();
	s << "[SINK__TRACE] " << constData->nodeId << " received "  << head.GetPacketType ()
	<< " (" << head.GetData () + head.GetSerializedSize () << "B) from node "
	<< head.GetId () << " via " << fromAddr << " at " << Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(constData->outputStream)->GetStream () << s.str() << std::endl;
}

// Trace function for sensors sending a packet
static void
SensorDataSendTrace (TraceConstData * constData, Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);

	std::stringstream s;
	s << "[SENSORTRACE] " << constData->nodeId << " sent " << head.GetPacketType () << " (" << packet->GetSize () << "B) at " << Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(constData->outputStream)->GetStream () << s.str() << std::endl;
}

// Trace function for MDC forwarding a packet
static void
MdcPacketFwdTrace (TraceConstData * constData, Ptr<const Packet> packet)
{
	NS_LOG_FUNCTION_NOARGS ();

	MdcHeader head;
	packet->PeekHeader (head);

	std::stringstream s;
	s << "[COLL__TRACE] " << constData->nodeId << " forwarding [" << head.GetPacketType () << "] (" << packet->GetSize () << "B) from "
	<< head.GetOrigin () << " to " << head.GetDest () << " at " << Simulator::Now ().GetSeconds ();

	NS_LOG_INFO (s.str ());
	*(constData->outputStream)->GetStream () << s.str() << std::endl;
}


bool
PC3Test1::Configure(int argc, char **argv)
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_LOG_FUNCTION("PC3Test1 Configuration Started.");
	ns3::PacketMetadata::Enable ();
	std::stringstream s;

	mdcConfig = MdcConfig::GetInstance();
	SeedManager::SetSeed (12345);

	_nSensors = mdcConfig->GetIntProperty("mdc.Sensors");
	_nMdcs = mdcConfig->GetIntProperty("mdc.MDCs");
	_mdcSpeed = mdcConfig->GetDoubleProperty("mdc.MdcSpeed");
	_mdcPause = mdcConfig->GetDoubleProperty("mdc.MdcPause");
	_totalSimTime = mdcConfig->GetDoubleProperty("mdc.TotalSimTime");
	_pcap = mdcConfig->GetBoolProperty("mdc.Pcap");
	_nEvents = mdcConfig->GetIntProperty("mdc.Events");
	_dataSize = mdcConfig->GetIntProperty("mdc.DataSize");
	_eventRadius = mdcConfig->GetDoubleProperty("mdc.EventRadius");
	_boundaryLength = mdcConfig->GetIntProperty("mdc.Boundary");
	_TraceFile = mdcConfig->GetStringProperty("mdc.TraceFile");
	_verbose = mdcConfig->GetIntProperty("mdc.Verbose");
	_sendFullData = mdcConfig->GetBoolProperty("mdc.SendFullData");
	_simStartTime = mdcConfig->GetDoubleProperty("mdc.SimStartTime");
	_simEndTime = mdcConfig->GetDoubleProperty("mdc.SimEndTime");



	// The following allows the params to get overridden by setting them up on the commandline
	CommandLine cmd;
	cmd.AddValue ("pcap", "Write PCAP traces.", _pcap);
	cmd.AddValue ("time", "Simulation time, s.", _totalSimTime);
	cmd.AddValue ("nCsma", "Number of \"sensor\" nodes/devices", _nSensors);
	cmd.AddValue ("nWifi", "Number of wifi MDC devices", _nMdcs);
	cmd.AddValue ("mdcSpeed", "Speed of the MDC devices", _mdcSpeed);
	cmd.AddValue ("mdcPause", "Pause duration of the MDC devices", _mdcPause);
	cmd.Parse(argc, argv); // Parse the commandline and see if the parameters have been passed.


	_nSensors = _nSensors == 0 ? 1 : _nSensors; // Making sure you have atleast one node in the sensor network.
	_nMdcs = _nMdcs == 0 ? 1 : _nMdcs; // Making sure you have atleast one node in the sensor network.
	cmd.Parse (argc, argv);


	if (_TraceFile != "")
	{
		AsciiTraceHelper asciiTraceHelper;
		outputStream = asciiTraceHelper.CreateFileStream (_TraceFile);
	}

	double posBound = _boundaryLength/2;
	double negBound = _boundaryLength/-2.0;
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
			_nSensors << "--> mdc.Sensors \n" << "    " <<
			_nMdcs << "--> mdc.MDCs \n" << "    " <<
			_mdcSpeed << "--> mdc.MdcSpeed \n" << "    " <<
			_mdcPause << "--> mdc.MdcPause \n" << "    " <<
			_totalSimTime << "--> mdc.TotalSimTime \n" << "    " <<
			_pcap << "--> mdc.Pcap \n" << "    " <<
			_nEvents << "--> mdc.Events \n" << "    " <<
			_dataSize << "--> mdc.DataSize \n" << "    " <<
			_eventRadius << "--> mdc.EventRadius \n" << "    " <<
			_boundaryLength << "--> mdc.Boundary \n" << "    " <<
			_TraceFile << "--> mdc.TraceFile \n" << "    " <<
			_verbose << "--> mdc.Verbose \n" << "    " <<
			_sendFullData << "--> mdc.SendFullData \n" << "    " <<
			_simStartTime << "--> mdc.SimStartTime \n" << "    " <<
			_simEndTime << "--> mdc.SimEndTime \n"<< "    " <<
			posBound << "--> posBound \n"<< "    " <<
			negBound << "--> negBound \n"
			;

	NS_LOG_INFO(s.str());
	return true;
}

void
PC3Test1::Run()
{
	NS_LOG_FUNCTION_NOARGS ();

	CreateNodes();
	CreateChannels();
	InstallInternetStack();
	SetupMobility();
	InstallApplications();

	//
	// Now, do the actual simulation.
	//
	std::stringstream s;
	s << "\nStarting simulation for " << _totalSimTime << " s ...";

	NS_LOG_INFO (s.str());

	Simulator::Stop (Seconds (_totalSimTime));
	Simulator::Run ();
	Simulator::Destroy ();

	NS_LOG_INFO ("PC3Test1 Run Ended.");
    return;

}

void
PC3Test1::Report (std::ostream &)
{
	NS_LOG_FUNCTION_NOARGS();
	NS_LOG_FUNCTION("PC3Test1 Report.");
}

void
PC3Test1::CreateNodes()
{
	NS_LOG_FUNCTION_NOARGS();
	NS_LOG_FUNCTION ("Create Sensor nodes.");
	sensorNodes.Create (_nSensors);

	//
	// Explicitly create the MDCs.
	// Each of the MDCs are APs. So we are assuming that they will all have the same SSID
	// Somehow the sensor nodes will attach themselves to the closest AP as it comes along
	//
	NS_LOG_FUNCTION ("Create MDC nodes.");
	mdcNodes.Create (_nMdcs);

	//
	// Create the Sink.
	//
	NS_LOG_FUNCTION ("Create Sink nodes.");
	sinkNodes.Create (1);

}

void
PC3Test1::CreateChannels()
{
	NS_LOG_FUNCTION_NOARGS();
	NS_LOG_FUNCTION ("PC3Test1 Create Channels Started.");
	YansWifiChannelHelper ywifiChHlpr;
	YansWifiPhyHelper ywifiPhyHlpr;
	NqosWifiMacHelper nqosWiMacHlpr;
	WifiHelper wifiHlpr;
	Ssid ssid;

	NS_LOG_FUNCTION ("Create Sensor channels.");
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


	NS_LOG_FUNCTION ("Create MDC channels.");
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


	NS_LOG_FUNCTION ("Create Sink channels.");
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


	if (_pcap)
	{
		ywifiPhyHlpr.EnablePcapAll (std::string ("mdcTrc"));
	}


}

void
PC3Test1::InstallInternetStack()
{
	NS_LOG_FUNCTION_NOARGS();

	NS_LOG_FUNCTION ("PC3Test1 Install Internet Stack Started.");

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
	NS_LOG_FUNCTION ("Assign IP Addresses.");
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
PC3Test1::SetupMobility()
{
	NS_LOG_FUNCTION_NOARGS();

	NS_LOG_FUNCTION ("PC3Test1 Setup Mobility Started.");

	// Now apply the mobility model
	// The Sensor nodes do not move in this simulation.
	// Their position is pre-determined on a grid which we can assume
	//   to be the bounds of the mobility of the MDC
	NS_LOG_FUNCTION ("Assign the Mobility Model to the sensors.");
	MobilityHelper mobHlpr;
	// This sets the initial placement of the STA nodes

	Ptr<ListPositionAllocator> sensorListPosAllocator = CreateObject<ListPositionAllocator> ();
	Vector sensorPosV;

	// You may not need to do this one sensor node at a time...
	for (uint32_t i=0; i< _nSensors; i++)
	{
		randomPositionAllocator->SetX (randomPosX);
		randomPositionAllocator->SetY (randomPosY);
		mobHlpr.SetPositionAllocator (randomPositionAllocator);
		mobHlpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobHlpr.Install (sensorNodes.Get(i));

		sensorPosV = Vector3D(randomPosX->GetValue(), randomPosY->GetValue(), 0.0);
		sensorListPosAllocator->Add(sensorPosV);
	}

	// The sensors should be positioned at random now within the x y bounds.
	// The positions are all recorded hopefully in the sensorListPosAllocator



	NS_LOG_FUNCTION ("Assign the Mobility Model to the sink.");
	// Place sink in center of region
	Ptr<ListPositionAllocator> centerPositionAllocator = CreateObject<ListPositionAllocator> ();
	Vector center = Vector3D (0.0, 0.0, 0.0);
	centerPositionAllocator->Add (center);
	mobHlpr.SetPositionAllocator (centerPositionAllocator);
	mobHlpr.Install (sinkNodes);



	NS_LOG_FUNCTION ("Assign the Mobility Model to the MDCs.");
	// What is left is the Mobility model for the MDCs.
	// Place MDCs in center of region
	Ptr<ConstantRandomVariable> constRandomSpeed = CreateObject<ConstantRandomVariable> ();
	constRandomSpeed->SetAttribute ("Constant", DoubleValue (_mdcSpeed));
	Ptr<ConstantRandomVariable> constRandomPause = CreateObject<ConstantRandomVariable> (); //default = 0
	constRandomPause->SetAttribute ("Constant", DoubleValue (_mdcPause));

	mobHlpr.SetPositionAllocator (randomPositionAllocator);
	mobHlpr.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
							 "Pause", PointerValue (constRandomPause),
							 "Speed", PointerValue (constRandomSpeed),
							 "PositionAllocator", PointerValue (sensorListPosAllocator));
	mobHlpr.Install (mdcNodes);

	// Now position all the MDCs in the center of the grid. This is the INITIAL position.
	Vector mdcPosV = Vector3D (0.0, 0.0, 0.0);
	for (NodeContainer::Iterator it = mdcNodes.Begin ();
	it != mdcNodes.End (); it++)
	{
		(*it)->GetObject<MobilityModel>()->SetPosition (mdcPosV);
	}

}

void
PC3Test1::InstallApplications()
{
	NS_LOG_FUNCTION_NOARGS();

	NS_LOG_FUNCTION ("PC3Test1 Install Applications Started.");
	TraceConstData *constData;
	// SINK   ---   TCP sink for data from MDCs, UDP sink for data/notifications directly from sensors

	MdcSinkHelper sinkHelper;
	ApplicationContainer sinkApps = sinkHelper.Install (sinkNodes);
	sinkApps.Start (Seconds (_simStartTime));
	sinkApps.Stop (Seconds (_simEndTime));
	if (_pcap)
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
	mdcApps.Start (Seconds (_simStartTime));
	mdcApps.Stop (Seconds (_simEndTime));


	/*
	* This tracing could be done in the helper itself but we can leave it as it is now
	*/

	if (_pcap)
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
	MdcEventSensorHelper sensorAppHelper (sinkInterfaces.GetAddress (0, 0), _nEvents);
	sensorAppHelper.SetAttribute ("PacketSize", UintegerValue (_dataSize));
	sensorAppHelper.SetAttribute ("SendFullData", BooleanValue (_sendFullData));
	sensorAppHelper.SetEventPositionAllocator (randomPositionAllocator);
	sensorAppHelper.SetRadiusRandomVariable (new ConstantVariable (_eventRadius));

	/*
	* The sensor app here then creates all the events. Again, this might be done in an event helper class but keep this for now.
	*/

	if (_nEvents > 1)
		sensorAppHelper.SetEventTimeRandomVariable (new UniformVariable (_simStartTime, _simEndTime));
	else
		sensorAppHelper.SetEventTimeRandomVariable (new ConstantVariable (_simStartTime*2));

	ApplicationContainer sensorApps = sensorAppHelper.Install (sensorNodes);
	sensorApps.Start (Seconds (_simStartTime));
	sensorApps.Stop (Seconds (_simEndTime));

	/*
	* Verbose tracing like before done for MDCs.
	*/
	for (ApplicationContainer::Iterator itr = sensorApps.Begin ();
	itr != sensorApps.End (); itr++)
	{
		if (_pcap)
		{
			constData = new TraceConstData();
			constData->outputStream = outputStream;
			constData->nodeId = (*itr)->GetNode ()->GetId ();

			(*itr)->TraceConnectWithoutContext ("Send", MakeBoundCallback (&SensorDataSendTrace, constData));
		}

	}

}
