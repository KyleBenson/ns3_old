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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"
#include "ns3/aodv-module.h"

#include "mdc-header.h"
#include "mdc-helper.h"
#include "mdc-event-sensor.h"
#include "mdc-collector.h"
#include "GraphBuilder.h"

#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/animation-interface.h"
#include "ns3/waypoint-mobility-model.h"
#include "ns3/waypoint.h"

/**
 * Creates a simulation environment for a wireless sensor network and event-driven
 * data being collected by a mobile data collector (MDC).
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MdcSimulation");

MdcGraphBuilder mdg;

struct TraceConstData {
	Ptr<OutputStreamWrapper> outputStream;
	uint32_t nodeId;
};

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

/// Trace function for remaining energy at node.
void RemainingEnergySink(TraceConstData * constData, double oldValue,
		double remainingEnergy) {
	//  NS_LOG_UNCOND ("Node " << constData.nodeId << " current remaining energy = " << remainingEnergy
	std::stringstream s;
	s << "Node " << constData->nodeId << " current remaining energy = "
			<< remainingEnergy << "J at time " << Simulator::Now().GetSeconds();

	//NS_LOG_INFO (s.str ());
	*(constData->outputStream)->GetStream() << s.str() << std::endl;
}

/// Trace function for the sink
void SinkPacketReceive(TraceConstData * constData, Ptr<const Packet> packet,
		const Address & from) {
	MdcHeader head;
	packet->PeekHeader(head);

	// Ignore MDC's data requests, especially since they'll hit both interfaces and double up...
	// though this shouldn't happen as broadcast should be to a network?
	if (head.GetFlags() == MdcHeader::mdcDataRequest)
		return;

	std::stringstream s;
	Ipv4Address fromAddr = InetSocketAddress::ConvertFrom(from).GetIpv4();
	s << "Sink " << constData->nodeId << " received " << head.GetPacketType()
			<< " (" << head.GetData() + head.GetSerializedSize()
			<< "B) from node " << head.GetId() << " via " << fromAddr << " at "
			<< Simulator::Now().GetSeconds();

	NS_LOG_INFO(s.str());
	*(constData->outputStream)->GetStream() << s.str() << std::endl;
}

/// Trace function for sensors sending a packet
static void SensorDataSent(TraceConstData * constData,
		Ptr<const Packet> packet) {
	MdcHeader head;
	packet->PeekHeader(head);

	std::stringstream s;
	s << "Sensor " << constData->nodeId << " sent " << head.GetPacketType()
			<< " (" << packet->GetSize() << "B) at "
			<< Simulator::Now().GetSeconds();

	NS_LOG_INFO(s.str());
	*(constData->outputStream)->GetStream() << s.str() << std::endl;
}

/// Trace function for MDC forwarding a packet
static void MdcPacketForward(TraceConstData * constData,
		Ptr<const Packet> packet) {
	MdcHeader head;
	packet->PeekHeader(head);

	std::stringstream s;
	s << "MDC " << constData->nodeId << " forwarding " << head.GetPacketType()
			<< " (" << packet->GetSize() << "B) from " << head.GetOrigin()
			<< " to " << head.GetDest() << " at "
			<< Simulator::Now().GetSeconds();

	NS_LOG_INFO(s.str());
}

void setVerbose(int verbose) {
	if (verbose >= 1) {
		//LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
		LogComponentEnable("MdcSimulation", LOG_LEVEL_INFO);
		//LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
		//LogComponentEnable ("BasicEnergySource", LOG_LEVEL_INFO);
		LogComponentEnable("MdcEventSensorApplication", LOG_LEVEL_INFO);
		LogComponentEnable("MdcHelper", LOG_LEVEL_INFO);
	}

	if (verbose >= 2) {
		LogComponentEnable("MdcCollectorApplication", LOG_LEVEL_INFO);
		LogComponentEnable("MdcSink", LOG_LEVEL_FUNCTION);
		//LogComponentEnable ("PacketSink", LOG_LEVEL_LOGIC);
		//LogComponentEnable ("MdcEventSensorApplication", LOG_LEVEL_LOGIC);
		//LogComponentEnable ("MdcHelper", LOG_LEVEL_LOGIC);
	}

	if (verbose >= 3) {
		LogComponentEnable("Socket", LOG_LEVEL_LOGIC);
		LogComponentEnable("MdcCollectorApplication", LOG_LEVEL_LOGIC);
		LogComponentEnable("TcpSocket", LOG_LEVEL_LOGIC);

		LogComponentEnable("Socket", LOG_LEVEL_INFO);
		LogComponentEnable("TcpSocket", LOG_LEVEL_INFO);
		LogComponentEnable("TcpTahoe", LOG_LEVEL_INFO);
		LogComponentEnable("TcpNewReno", LOG_LEVEL_INFO);
		LogComponentEnable("TcpReno", LOG_LEVEL_INFO);
		LogComponentEnable("PacketSink", LOG_LEVEL_FUNCTION);
		LogComponentEnable("Socket", LOG_LEVEL_FUNCTION);
	}

}

/**
 * Node Setup
 */
void setNodes(uint32_t nSensors, uint32_t nMdcs, NodeContainer &sensors,
		NodeContainer &sink, NodeContainer &mdcs) {
	sensors.Create(nSensors);
	sink.Create(1);
	mdcs.Create(nMdcs);
}

/**
 * WiFi Connection Setup
 */
void setWiFi(NodeContainer &sensors, NodeContainer &sink, NodeContainer &mdcs,
		NetDeviceContainer &sensorDevices, NetDeviceContainer &sinkSensorDevice,
		NetDeviceContainer &mdcSensorDevices,
		NetDeviceContainer &mdcSinkDevices, NetDeviceContainer &sinkMdcDevice) {

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	NqosWifiMacHelper mac = NqosWifiMacHelper::Default();
	YansWifiPhyHelper phyHelper = YansWifiPhyHelper::Default();

	phyHelper.Set("EnergyDetectionThreshold", DoubleValue(-50.0));
	phyHelper.Set("CcaMode1Threshold", DoubleValue(-99.0));
	phyHelper.Set("TxGain", DoubleValue(4.5));
	phyHelper.Set("RxGain", DoubleValue(4.5));
	phyHelper.Set("TxPowerLevels", UintegerValue(1));
	phyHelper.Set("TxPowerEnd", DoubleValue(16));
	phyHelper.Set("TxPowerStart", DoubleValue(16));
	phyHelper.Set("RxNoiseFigure", DoubleValue(4));

	phyHelper.SetChannel(channel.Create());

	/////////////////////////
	YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default();
	NqosWifiMacHelper mac1 = NqosWifiMacHelper::Default();
	YansWifiPhyHelper phyHelper1 = YansWifiPhyHelper::Default();

	phyHelper1.Set("EnergyDetectionThreshold", DoubleValue(-90.0));
	phyHelper1.Set("CcaMode1Threshold", DoubleValue(-99.0));
	phyHelper1.Set("TxGain", DoubleValue(4.5));
	phyHelper1.Set("RxGain", DoubleValue(4.5));
	phyHelper1.Set("TxPowerLevels", UintegerValue(1));
	phyHelper1.Set("TxPowerEnd", DoubleValue(16));
	phyHelper1.Set("TxPowerStart", DoubleValue(16));
	phyHelper1.Set("RxNoiseFigure", DoubleValue(4));
	phyHelper1.SetChannel(channel1.Create());
	//////////////////////////

	WifiHelper wifi = WifiHelper::Default();
	//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
	wifi.SetRemoteStationManager("ns3::AarfcdWifiManager");

	//TODO: tweak rates, thresholds, power levels

	sensorDevices = wifi.Install(phyHelper, mac, sensors);

	sinkSensorDevice = wifi.Install(phyHelper, mac, sink);

	mdcSensorDevices = wifi.Install(phyHelper, mac, mdcs);
	//mdcSensorDevices = wifi.Install(phy, mac, mdcs1);

	// Setup MDCs and associated sink interface
	/*Ssid ssid = Ssid ("ns-3-ssid");
	 mac.SetType ("ns3::StaWifiMac",
	 "Ssid", SsidValue (ssid),
	 "ActiveProbing", BooleanValue (false));*/

	mdcSinkDevices = wifi.Install(phyHelper1, mac1, mdcs);

	/*mac.SetType ("ns3::ApWifiMac",
	 "Ssid", SsidValue (ssid));*/
	sinkMdcDevice = wifi.Install(phyHelper1, mac1, sink);

}

/**
 * Node Position Setup
 */
std::list<GraphSensors> setNodePosition(NodeContainer &sensors,
		MobilityHelper &mobility) {

	Ptr<ListPositionAllocator> posAlloc_sens = CreateObject<
			ListPositionAllocator>();

	for (int i = 0; i < NumberOfSensors; i++) {
		posAlloc_sens->Add(Vector(Sensor_XCoor[i], Sensor_YCoor[i], 0.0));
	}

	std::list<GraphSensors> gListSen;
	mobility.SetPositionAllocator(posAlloc_sens);
	for (uint32_t c = 0; c < sensors.GetN(); c++) {
		Ptr<Node> node = sensors.Get(c);
		GraphSensors *g = new GraphSensors(Sensor_XCoor[c], Sensor_YCoor[c],
				(uint32_t) Sensor_LogicalID[c]);

		//set property
		g->setIsAlive(true);
		g->setLocation(Vector3D(Sensor_XCoor[c], Sensor_YCoor[c], 0.0));
		g->setSensorCondition(GraphSensors::SAFE);
		g->setSensorState(GraphSensors::inWAIT);
		g->setShouldVisit(true);

		gListSen.push_back(*g);
		AnimationInterface::SetNodeColor(node, 0, 255, 0);
		AnimationInterface::SetConstantPosition(node, 0, 0);
	}

	mobility.Install(sensors);

	return gListSen;
}

/**
 * Sink Postion Setup
 */
Vector setSinkPosition(uint32_t boundaryLength, NodeContainer &sink,
		MobilityHelper &mobility,
		Ptr<ListPositionAllocator> &centerPositionAllocator) {
	Vector center = Vector3D(boundaryLength / 2.0, boundaryLength / 2.0, 0.0);

	Ptr<Node> node = sink.Get(0);
	AnimationInterface::SetNodeColor(node, 0, 0, 0);
	AnimationInterface::SetConstantPosition(node, 0, 0);
	centerPositionAllocator->Add(center);
	mobility.SetPositionAllocator(centerPositionAllocator);
	mobility.Install(sink);

	return center;
}

/**
 * MDC Position Setup
 */
///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void setMdcPosition(double mdcSpeed, Ptr<UniformRandomVariable> randomPosition,
		MobilityHelper &mobility,
		Ptr<RandomRectanglePositionAllocator> randomPositionAllocator,
		Ptr<ConstantRandomVariable> constRandomSpeed,
		Ptr<ConstantRandomVariable> constRandomPause, NodeContainer &mdcs,
		uint32_t boundaryLength) {

	RandomWaypointMobilityModel *r = NULL;	//new RandomWaypointMobilityModel();
//	r->SetAttribute("Pause", PointerValue(constRandomPause));
//	r->SetAttribute("Speed", PointerValue(constRandomSpeed));
//	r->SetAttribute("PositionAllocator", PointerValue(positionAlloc));
	mobility.SetMobilityModel("ns3::HierarchicalMobilityModel", "Child",
			PointerValue(r));

	Vector center = Vector3D(boundaryLength / 2.0, boundaryLength / 2.0, 0.0);
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<
			ListPositionAllocator>();
	constRandomSpeed->SetAttribute("Constant", DoubleValue(mdcSpeed));

	if (numOfMDC == 1) {
		positionAlloc->Add(center);

		if (scenario == 1) {
			for (uint32_t i = 0; i < sizeof(OUTPUT_PATH) / sizeof(float); i++) {
				int idx = OUTPUT_PATH[i] - 1;
				positionAlloc->Add(
						Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
			}
		} else {
			for (uint32_t i = 0; i < sizeof(ALT_OUTPUT_PATH) / sizeof(float); i++) {
				int idx = ALT_OUTPUT_PATH[i] - 1;
				positionAlloc->Add(
						Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
			}
		}
		positionAlloc->Add(center);
		mobility.SetPositionAllocator(positionAlloc);
		mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", "Pause",
				PointerValue(constRandomPause), "Speed",
				PointerValue(constRandomSpeed), "PositionAllocator",
				PointerValue(positionAlloc));
		mobility.Install(mdcs);
		mobility.PushReferenceMobilityModel(mdcs.Get(0));
	} else if (numOfMDC == 2) {

		positionAlloc->Add(center);

		if (scenario == 1) {
			for (uint32_t i = 0; i < sizeof(MUL_OUTPUT_PATHA) / sizeof(float);
					i++) {
				int idx = MUL_OUTPUT_PATHA[i] - 1;
				positionAlloc->Add(
						Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
			}
		} else {
			for (uint32_t i = 0; i < sizeof(MUL_OUTPUT_PATHA1) / sizeof(float);
					i++) {
				int idx = MUL_OUTPUT_PATHA1[i] - 1;
				positionAlloc->Add(
						Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
			}
		}

		positionAlloc->Add(center);
		mobility.SetPositionAllocator(positionAlloc);

		mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", "Pause",
				PointerValue(constRandomPause), "Speed",
				PointerValue(constRandomSpeed), "PositionAllocator",
				PointerValue(positionAlloc));
		mobility.Install(mdcs.Get(0));

		Ptr<ListPositionAllocator> positionAlloc1 = CreateObject<
				ListPositionAllocator>();
		positionAlloc1->Add(center);
		for (int i = 0; i < NumberOfOutput; i++) {
			int idx = 0;
			if (scenario == 1) {
				idx = MUL_OUTPUT_PATHB[i] - 1;
			} else {
				idx = MUL_OUTPUT_PATHB1[i] - 1;
			}
			positionAlloc1->Add(Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
		}
		positionAlloc1->Add(center);
		mobility.SetPositionAllocator(positionAlloc1);
		mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", "Pause",
				PointerValue(constRandomPause), "Speed",
				PointerValue(constRandomSpeed), "PositionAllocator",
				PointerValue(positionAlloc1));

		mobility.Install(mdcs.Get(1));

		mobility.PushReferenceMobilityModel(mdcs.Get(1));
		mobility.PushReferenceMobilityModel(mdcs.Get(0));
	}
}

/**
 * MDC Application Setup
 */
ApplicationContainer setMDCApp(MdcCollectorHelper &mdcAppHelper,
		double simStartTime, double simEndTime, NodeContainer &mdcs,
		Ipv4InterfaceContainer &sinkMdcInterface) {
	mdcAppHelper.SetAttribute("RemoteAddress",
			Ipv4AddressValue(
					Ipv4Address::ConvertFrom(sinkMdcInterface.GetAddress(0))));
	mdcAppHelper.SetAttribute("Interval", TimeValue(Seconds(3.0)));
//mdcAppHelper.SetAttribute ("Port", UIntegerValue (9999));

	Ptr<Node> node = mdcs.Get(0);
	AnimationInterface::SetNodeColor(node, 255, 0, 0);
	AnimationInterface::SetConstantPosition(node, 0, 0);
	ApplicationContainer mdcApps = mdcAppHelper.Install(mdcs);
	mdcApps.Start(Seconds(simStartTime));
	mdcApps.Stop(Seconds(simEndTime));
	return mdcApps;
}

/**
 * Sensor Application Setup
 */
MdcEventSensorHelper setSensors(Ipv4InterfaceContainer &sinkSensorInterface,
		uint32_t nEvents,
		Ptr<RandomRectanglePositionAllocator> randomPositionAllocator,
		uint32_t dataSize, bool sendFullData, double eventRadius) {
	MdcEventSensorHelper sensorAppHelper(sinkSensorInterface.GetAddress(0, 0),
			nEvents);
	sensorAppHelper.SetAttribute("PacketSize", UintegerValue(dataSize));
	sensorAppHelper.SetAttribute("SendFullData", BooleanValue(sendFullData));
	sensorAppHelper.SetEventPositionAllocator(randomPositionAllocator);
	sensorAppHelper.SetRadiusRandomVariable(new ConstantVariable(eventRadius));
	return sensorAppHelper;
}

int main(int argc, char *argv[]) {

	int verbose = 1;
	uint32_t nSensors = NumberOfSensors;
	uint32_t nMdcs = numOfMDC;
	uint32_t nEvents = 1;
	uint32_t dataSize = 1024;
	double eventRadius = 5.0;
	double mdcSpeed = 3.0;
	bool sendFullData = true;
	uint32_t boundaryLength = 100;
	std::string traceFile = "mdc-trace.txt";
	double simStartTime = 1.0;
	double simEndTime = simRunTime;

	LogComponentEnable("MdcSimulation", LOG_LEVEL_INFO);

	ns3::PacketMetadata::Enable();

	CommandLine cmd;
	cmd.AddValue("sensors", "Number of sensor nodes", nSensors);
	cmd.AddValue("mdcs", "Number of mobile data collectors (MDCs)", nMdcs);
	cmd.AddValue("events", "Number of events to occur", nEvents);
	cmd.AddValue("data_size", "Size (in bytes) of full sensed event data",
			dataSize);
	cmd.AddValue("event_radius", "Radius of affect of the events", eventRadius);
	cmd.AddValue("mdc_speed", "Speed (in m/s) of the MDCs", mdcSpeed);
	cmd.AddValue("send_full_data",
			"Whether to send the full data upon event detection or simply a notification and then send the full data to the MDCs",
			sendFullData);
	cmd.AddValue("verbose", "Enable verbose logging", verbose);
	cmd.AddValue("boundary",
			"Length of (one side) of the square bounding box for the geographic region under study (in meters)",
			boundaryLength);
	cmd.AddValue("trace_file", "File to write traces to", traceFile);
	cmd.AddValue("time", "End time in seconds", simEndTime);
	cmd.Parse(argc, argv);

	setVerbose(verbose);
	bool tracing = false;
	Ptr<OutputStreamWrapper> outputStream;
	if (traceFile != "") {
		AsciiTraceHelper asciiTraceHelper;
		outputStream = asciiTraceHelper.CreateFileStream(traceFile);
		tracing = true;
	}

/////////////////////////////////////Initialization//////////////////////////////////////
	NodeContainer sensors;
	NodeContainer sink;
	NodeContainer mdcs;
	NetDeviceContainer sensorDevices;
	NetDeviceContainer sinkSensorDevice;
	NetDeviceContainer mdcSensorDevices;
	NetDeviceContainer mdcSinkDevices;
	NetDeviceContainer sinkMdcDevice;

	Ptr<UniformRandomVariable> randomPosition = CreateObject<
			UniformRandomVariable>();
	MobilityHelper mobility;
	Ptr<RandomRectanglePositionAllocator> randomPositionAllocator =
			CreateObject<RandomRectanglePositionAllocator>();
	Ptr<ListPositionAllocator> centerPositionAllocator = CreateObject<
			ListPositionAllocator>();
	Ptr<ConstantRandomVariable> constRandomSpeed = CreateObject<
			ConstantRandomVariable>();
	Ptr<ConstantRandomVariable> constRandomPause = CreateObject<
			ConstantRandomVariable>();	//default = 0

	setNodes(nSensors, nMdcs, sensors, sink, mdcs);
	setWiFi(sensors, sink, mdcs, sensorDevices, sinkSensorDevice,
			mdcSensorDevices, mdcSinkDevices, sinkMdcDevice); /*Function to setup WiFi*/

	list<GraphSensors> gListSen = setNodePosition(sensors, mobility);

	Vector center = setSinkPosition(boundaryLength, sink, mobility,
			centerPositionAllocator);
	setMdcPosition(mdcSpeed, randomPosition, mobility, randomPositionAllocator,
			constRandomSpeed, constRandomPause, mdcs, boundaryLength);

	for (NodeContainer::Iterator itr = mdcs.Begin(); itr != mdcs.End(); itr++) {
		(*itr)->GetObject<MobilityModel>()->SetPosition(center);
	}

/////////////////////////////////////////Network/////////////////////////////////////////
	InternetStackHelper stack;
	AodvHelper aodv;
	aodv.Set("EnableHello", BooleanValue(false));
	aodv.Set("EnableBroadcast", BooleanValue(false));

	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper listRouting;
	listRouting.Add(staticRouting, 5);
	listRouting.Add(aodv, 1);
	stack.SetRoutingHelper(listRouting);

	stack.Install(sensors);
	stack.Install(sink);
	stack.Install(mdcs);

	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");

	Ipv4InterfaceContainer sinkSensorInterface;
	sinkSensorInterface = address.Assign(sinkSensorDevice);
	Ipv4InterfaceContainer sensorInterfaces;
	sensorInterfaces = address.Assign(sensorDevices);
	Ipv4InterfaceContainer mdcSensorInterfaces;
	mdcSensorInterfaces = address.Assign(mdcSensorDevices);

	address.NewNetwork();
	Ipv4InterfaceContainer sinkMdcInterface;
	sinkMdcInterface = address.Assign(sinkMdcDevice);
	Ipv4InterfaceContainer mdcSinkInterfaces;
	mdcSinkInterfaces = address.Assign(mdcSinkDevices);

	TraceConstData * constData = new TraceConstData();
	MdcSinkHelper sinkHelper;
	ApplicationContainer sinkApps = sinkHelper.Install(sink);

	constData->outputStream = outputStream;
	constData->nodeId = sink.Get(0)->GetId();

	sinkApps.Start(Seconds(simStartTime));
	sinkApps.Stop(Seconds(simEndTime));

	if (tracing) {
		sinkApps.Get(0)->TraceConnectWithoutContext("Rx",
				MakeBoundCallback(&SinkPacketReceive, constData));
	}

	MdcCollectorHelper mdcAppHelper;
	ApplicationContainer mdcApps = setMDCApp(mdcAppHelper, simStartTime,
			simEndTime, mdcs, sinkMdcInterface);

	if (verbose) {

		for (uint8_t i = 0; i < mdcApps.GetN(); i++) {
			constData = new TraceConstData();
			constData->nodeId = mdcApps.Get(i)->GetNode()->GetId();
			mdcApps.Get(i)->TraceConnectWithoutContext("Forward",
					MakeBoundCallback(&MdcPacketForward, constData));
		}
	}

	MdcEventSensorHelper sensorAppHelper = setSensors(sinkSensorInterface,
			nEvents, randomPositionAllocator, dataSize, sendFullData,
			eventRadius);

	if (nEvents > 1)
		sensorAppHelper.SetEventTimeRandomVariable(
				new UniformVariable(simStartTime, simEndTime));
	else
		sensorAppHelper.SetEventTimeRandomVariable(
				new ConstantVariable(simStartTime * 2));

	ApplicationContainer sensorApps = sensorAppHelper.Install(sensors);
	sensorApps.Start(Seconds(simStartTime));
	sensorApps.Stop(Seconds(simEndTime));

	for (ApplicationContainer::Iterator itr = sensorApps.Begin();
			itr != sensorApps.End(); itr++) {
		if (tracing) {
			constData = new TraceConstData();
			constData->outputStream = outputStream;
			constData->nodeId = (*itr)->GetNode()->GetId();

			(*itr)->TraceConnectWithoutContext("Send",
					MakeBoundCallback(&SensorDataSent, constData));
		}
	}

	// build the road network

	mdg.createRoadGraph(gListSen, &mdcs, &mobility, &mdcApps);

	Simulator::Stop(Seconds(simEndTime));
	Simulator::Run();
	Simulator::Destroy();

	NS_LOG_INFO("Done!");
	return 0;
}
