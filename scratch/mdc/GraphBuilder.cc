/*
 * GraphBuilder.cc
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 *
 *      References:
 *      Find a rectangle formed by a line - http://math.stackexchange.com/questions/60336/how-to-find-a-rectangle-which-is-formed-from-the-line
 *      Finding a point in a polygon - http://alienryderflex.com/polygon/
 */

#include "GraphBuilder.h"

using namespace ns3;

AnimationInterface * pAnim = 0;

NS_LOG_COMPONENT_DEFINE("MdcSimulationGraphBuilder");

struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct Sched_Params {
	NodeContainer * mdcs;
	MobilityHelper * mobility;
	ApplicationContainer * mdcApps;

};
#define MaxX 90
#define MaxY 90

void scheduledEventsCallback(list<GraphEdge> graphEdges,
		list<GraphSensors> graphSensors,
		map<int, GraphIntersections> mapGraphIntersections, uint32_t countID,
		Sched_Params * sched_parameters);
void setNewMdcPosition(list<GraphEdge> graphEdges,
		list<GraphSensors> graphSensors,
		map<int, GraphIntersections> mapGraphIntersections, uint32_t countID,
		Sched_Params * sched_parameters);

MdcGraphBuilder::MdcGraphBuilder() {
}

std::list<GraphEdge>& MdcGraphBuilder::getGraphEdges() {
	return graphEdges;
}

void MdcGraphBuilder::setGraphEdges(std::list<GraphEdge>& graphEdges) {
	this->graphEdges = graphEdges;
}

//const std::list<GraphIntersections>& MdcGraphBuilder::getGraphIntersections() {
//	return graphIntersections;
//}

void MdcGraphBuilder::setGraphSensors(std::list<GraphSensors>& graphSensors) {
	this->graphSensors = graphSensors;
}

std::list<GraphSensors>& MdcGraphBuilder::getGraphSensors() {
	return graphSensors;
}

//void MdcGraphBuilder::setGraphIntersections(
//		const std::list<GraphIntersections>& graphIntersections) {
//	this->graphIntersections = graphIntersections;
//}

std::map<int, GraphIntersections>& MdcGraphBuilder::getGraphIntersections() {
	return mapGraphIntersections;
}
void MdcGraphBuilder::setGraphIntersections(
		const std::map<int, GraphIntersections>& graphIntersections) {
	this->mapGraphIntersections = graphIntersections;
}

void MdcGraphBuilder::installPtToPt(Ptr<Node> node1, Ptr<Node> node2) {
	NodeContainer newCont;
	newCont.Add(node1);
	newCont.Add(node2);
	PointToPointHelper pointToPoint;
	pointToPoint.Install(newCont);
}

void MdcGraphBuilder::connectTheDots(NodeContainer nodeCon) {
	for (list<GraphEdge>::iterator itr = graphEdges.begin();
			itr != graphEdges.end(); itr++) {
		uint32_t n1 = itr->getFirstNode();
		uint32_t n2 = itr->getSecondNode();

		installPtToPt(nodeCon.Get(n1 - 1), nodeCon.Get(n2 - 1));
	}
}

bool MdcGraphBuilder::findInVicinity(RectCoOrd r, double x, double y) {

	float x1 = r.x1, y1 = r.y1, x2 = r.x2, y2 = r.y2;

	float x3 = x1 - x2;
	float y3 = y2 - y1;
	float den = sqrt(pow(x3, 2.0) + pow(y3, 2.0));

	float y4 = (RadiusOfMDC * x3) / den;
	float x4 = (RadiusOfMDC * y3) / den;

	double a1 = x1 + round(x4);
	double a2 = x2 + round(x4);
	double a3 = x1 - round(x4);
	double a4 = x2 - round(x4);

	double b1 = y1 + round(y4);
	double b2 = y2 + round(y4);
	double b3 = y1 - round(y4);
	double b4 = y2 - round(y4);

//	float polyX[] = { x1 + x4, x2 + x4, x2 - x4, x1 - x4 };
//	float polyY[] = { y1 + y4, y2 + y4, y2 - y4, y1 - y4 };
	double polyX[] = { a1, a2, a3, a4 };
	double polyY[] = { b1, b2, b3, b4 };

	int i, j = 3;
	bool bVal = false;
//	std::stringstream s;
//	s << x << ", " << y << " offsets " << x3 << "," << y3 << " " << x4 << " "
//			<< y4 << " " << " lies within Rect: " << a1 << ", " << b1
//			<< ", " << a2 << ", " << b2 << ", " << a3 << ", "
//			<< b3 << ", " << a4 << ", " << b4;
//	NS_LOG_INFO(s.str());
	for (i = 0; i < 4; i++) {
		if ((polyY[i] < y && polyY[j] >= y)
				|| (polyY[j] < y && polyY[i] >= y)) {
			if (polyX[i]
					+ (y - polyY[i]) / (polyY[j] - polyY[i])
							* (polyX[j] - polyX[i]) < x) {
				bVal = !bVal;
			}
		}
		j = i;
	}
	return bVal;
}

bool testInVicinity() {

	double x = 44;
	double y = 10;
	float x1 = 42;
	float y1 = 12;
	float x2 = 46;
	float y2 = 2;

	float x3 = x1 - x2;
	float y3 = y2 - y1;

	float den = sqrt(pow(x3, 2.0) + pow(y3, 2.0));

	float y4 = 4 * x3 / den;
	float x4 = 4 * y3 / den;

	float polyX[] = { x1 + x4, x2 + x4, x2 - x4, x1 - x4 };
	float polyY[] = { y1 + y4, y2 + y4, y2 - y4, y1 - y4 };
	int i, j = 3;
	bool bVal = false;

	std::stringstream s;
	s << x << ", " << y << " offsets " << x3 << "," << y3 << " " << x4 << " "
			<< y4 << " " << " lies within Rect: " << x1 + x4 << ", " << y1 + y4
			<< ", " << x2 + x4 << ", " << y2 + y4 << ", " << x2 - x4 << ", "
			<< y2 - y4 << ", " << x1 - x4 << ", " << y1 - y4;
	NS_LOG_INFO(s.str());
//	std::stringstream s;
//	s << x1 + x4 << ", " << y1 + y4 << ", " << x2 + x4 << ", " << y2 + y4
//			<< ", " << x2 - x4 << ", " << y2 - y4 << ", " << x1 - x4 << ", "
//			<< y1 - y4;
//	NS_LOG_INFO(s.str());

	float t1 = (((polyX[3] - polyX[2]) * (polyY[2] - y))
			- ((polyX[2] - x) * (polyY[3] - polyY[2])));
	float t2 = (((polyX[0] - polyX[2]) * (polyY[3] - polyY[2]))
			- ((polyX[3] - polyX[2]) * (polyY[0] - polyY[2])));
	float t3 = (((polyX[0] - polyX[2]) * (polyY[2] - y))
			- ((polyX[2] - x) * (polyY[0] - polyY[2])));
	float t4 = (((polyX[3] - polyX[2]) * (polyY[0] - polyY[2]))
			- ((polyX[0] - polyX[2]) * (polyY[3] - polyY[2])));
	float a1, a2, a3;

	if (t2 != 0) {
		a1 = t1 / t2;
	} else {
		a1 = 0;
	}
	if (t4 != 0) {
		a2 = t3 / t4;
	} else {
		a2 = 0;
	}
	a3 = 1.0f - a1 - a2;

	t1 = (((polyX[1] - polyX[2]) * (polyY[2] - y))
			- ((polyX[2] - x) * (polyY[1] - polyY[2])));
	t2 = (((polyX[0] - polyX[2]) * (polyY[1] - polyY[2]))
			- ((polyX[1] - polyX[2]) * (polyY[0] - polyY[2])));
	t3 = (((polyX[0] - polyX[2]) * (polyY[2] - y))
			- ((polyX[2] - x) * (polyY[0] - polyY[2])));
	t4 = (((polyX[1] - polyX[2]) * (polyY[0] - polyY[2]))
			- ((polyX[0] - polyX[2]) * (polyY[1] - polyY[2])));

	float b1, b2, b3;

	if (t2 != 0) {
		b1 = t1 / t2;
	} else {
		b1 = 0;
	}
	if (t4 != 0) {
		b2 = t3 / t4;
	} else {
		b2 = 0;
	}
	b3 = 1.0f - b1 - b2;

	//NS_LOG_INFO(a1); NS_LOG_INFO(a2); NS_LOG_INFO(a3); NS_LOG_INFO(b1); NS_LOG_INFO(b2); NS_LOG_INFO(b3);

	if ((a1 < 0) || (a2 < 0) || (a3 < 0)) {
		if ((b1 < 0) || (b2 < 0) || (b3 < 0))
			return false;
	}
	return true;
	for (i = 0; i < 3; i++) {
		if ((polyY[i] < y && polyY[j] >= y)
				|| (polyY[j] < y && polyY[i] >= y)) {
			if (polyX[i]
					+ (y - polyY[i]) / (polyY[j] - polyY[i])
							* (polyX[j] - polyX[i]) < x) {
				bVal = !bVal;
			}
		}
		j = i;
	}

	return bVal;
}

void MdcGraphBuilder::setSensorToEdges(list<GraphSensors>::iterator itrG,
		double x, double y) {

	//NS_LOG_INFO(itrG->getSid());

	bool mapped = false;

	//NS_LOG_INFO(graphEdges.size());

	for (list<GraphEdge>::iterator itrE = graphEdges.begin();
			itrE != graphEdges.end(); itrE++) {

		RectCoOrd r;
		r.x1 = itrE->getX1();
		r.y1 = itrE->getY1();
		r.x2 = itrE->getX2();
		r.y2 = itrE->getY2();

//		std::stringstream s;
//		s << r.x1<<" "<<r.y1<<" "<<r.x2<<" "<<r.y2;
//		NS_LOG_INFO(s.str());

		bool b = findInVicinity(r, x, y);
		if (b) {
			mapped = true;
//			std::stringstream s;
//			s << itrG->getSid() << " in Vicinity " << itrE->getFirstNode()
//					<< ", " << itrE->getSecondNode();
//			NS_LOG_INFO(s.str());
			itrE->addSensor(*itrG);
			itrE->params->setNumSensors(itrE->getSensors().size());
		}
	}

	if (!mapped)
		;//NS_LOG_INFO("Error: unable to map sensor to any edge");
}

void MdcGraphBuilder::assignSensorsToEdges() {

	//NS_LOG_INFO(graphSensors.size());

	for (list<GraphSensors>::iterator itrG = graphSensors.begin();
			itrG != graphSensors.end(); itrG++) {

		Vector3D vec = itrG->getLocation();
		setSensorToEdges(itrG, vec.x, vec.y);
	}
}

void MdcGraphBuilder::populateIntersections() {

	int i = 0;
	for (; i < NumberOfNodes; ++i) {
		GraphIntersections * g = new GraphIntersections(Node_LogicalID[i],
				(float) Node_XCoor[i], (float) Node_YCoor[i]);
		//graphIntersections.push_back(*g);
		mapGraphIntersections.insert(std::make_pair(Node_LogicalID[i], *g));
	}
}

void MdcGraphBuilder::populateEdges() {
	int i = 0;
	for (; i < NumberOfEdges; ++i) {

		GraphEdgeTuple * gTuple = new GraphEdgeTuple((uint32_t) all_tup[i][0],
				(uint32_t) all_tup[i][1], (uint32_t) all_tup[i][2],
				(uint32_t) all_tup[i][3], (uint32_t) all_tup[i][4],
				(uint32_t) all_tup[i][5], (uint32_t) all_tup[i][6]);

		GraphEdge * g = new GraphEdge((uint32_t) Edge_Node1[i],
				(uint32_t) Edge_Node2[i], (float) Node_XCoor[Edge_Node1[i] - 1],
				(float) Node_YCoor[Edge_Node1[i] - 1],
				(float) Node_XCoor[Edge_Node2[i] - 1],
				(float) Node_YCoor[Edge_Node2[i] - 1], gTuple);

		graphEdges.push_back(*g);
	}
}

void MdcGraphBuilder::updateAdjacencyEdges() {

	for (std::map<int, GraphIntersections>::iterator itrI =
			mapGraphIntersections.begin(); itrI != mapGraphIntersections.end();
			itrI++) {

		for (list<GraphEdge>::iterator itrE = graphEdges.begin();
				itrE != graphEdges.end(); itrE++) {

			if ((itrE->getFirstNode()
					== ((GraphIntersections) itrI->second).getNodeID())
					|| (itrE->getSecondNode()
							== ((GraphIntersections) itrI->second).getNodeID())) {
				itrI->second.setAdjacency(*itrE);
			}
		}
	}
}

void resetEdgeVisibility(list<GraphEdge> graphEdges,
		map<int, GraphIntersections> mapGraphIntersections, uint32_t sid,
		GraphSensors::SensorMode sMode) {

	std::stringstream s;
	s << "id " << sid << " current level = " << sMode << " at time "
			<< Simulator::Now().GetSeconds();
	NS_LOG_INFO(s.str());

	for (list<GraphEdge>::iterator itr = graphEdges.begin();
			itr != graphEdges.end(); itr++) {

		//NS_LOG_INFO(itr->getSensors().size());

		//Only if Edge has any sensors alive do we check
		if (!itr->isIsVisitable() || (itr->params->getNumSensors() == 0)) {
			continue;
		}

		list<GraphSensors> slist = itr->getSensors();

		//Get the particular sensor object that matches a certain danger and set 'dead'
		for (list<GraphSensors>::iterator itrG = slist.begin();
				itrG != slist.end(); itrG++) {

			if (itrG->getSid() == sid) {

				//bool skip = false;
				if (itrG->getSensorCondition() == GraphSensors::FAIL) {
					break; //skip=true;
				}

				//Matched the sensor in question and setting its Sensor object so it can be got again(usually done after
				//assessing information from Trace file
				itrG->setSensorCondition(sMode);

				//IF SENSOR MODE IS : FAILED
				if (itrG->getSensorCondition() == GraphSensors::FAIL) {

					NS_LOG_INFO(itr->params->getNumSensors());
					if (itr->params->getNumSensors() > 0) {

						itr->params->setNumSensors(
								itr->params->getNumSensors() - 1);

						NS_LOG_INFO(itr->params->getNumSensors());NS_LOG_INFO(itr->getFirstNode());NS_LOG_INFO(itr->getSecondNode());

						//IF LAST SENSOR ON ROAD/EDGE FAILED
						if (itr->params->getNumSensors() == 0) {

							itr->params->setCongestion(10000); //an arbit high value
							itr->params->setAvgSensorPriority(0.01);
							itr->setIsVisitable(false);

							for (std::map<int, GraphIntersections>::iterator itrI =
									mapGraphIntersections.begin();
									itrI != mapGraphIntersections.end();
									itrI++) {

								if (itrI->second.getNodeID()
										== itr->getFirstNode()) {
									pAnim->UpdateNodeColor(
											itrI->second.getActualID(), 40, 40,
											140);
								}

								if (itrI->second.getNodeID()
										== itr->getSecondNode()) {
									pAnim->UpdateNodeColor(
											itrI->second.getActualID(), 40, 40,
											140);
								}
							}
						} else {
							//JUST UPDATE EDGE WEIGHTS ACCOUNTING FOR THE FAILED SENSOR
							itr->params->setCongestion(
									itr->params->getCongestion() + 5); //a higher value set
							uint32_t new_avg =
									(itr->params->getAvgSensorPriority()
											* (itr->params->getNumSensors() + 1)
											- itrG->getSensorWeight())
											/ (itr->params->getNumSensors());
							itr->params->setAvgSensorPriority(new_avg);
						}
					} else {
						NS_LOG_INFO("Can't remove!, Edge has no sensor already!");
					}

				} else if (itrG->getSensorCondition()
						== GraphSensors::WARNING) {
					//TODO:
					//IF WARNING
					//Update weight and colors accordingly

				} else if (itrG->getSensorCondition() == GraphSensors::DANGER) {
					//TODO:
					//IF DANGER
					//Update Weights ACCORDINGLY--nEED TO CONSIDER VISUALISING, BUT MAY BE A LITTLE HEAVY TO VIEW
				}
				break; //No other sensors to check with
			} //MAtch of SID
		} //END OF Sensors
	} //END OF EDGES LIST
}

void setNewMdcPosition(list<GraphEdge> graphEdges,
		list<GraphSensors> graphSensors,
		map<int, GraphIntersections> mapGraphIntersections, uint32_t countID,
		Sched_Params * sched_parameters) {

	uint32_t boundaryLength = 100;
	NodeContainer *mdcs = sched_parameters->mdcs;
	MobilityHelper *mobility = sched_parameters->mobility;
	ApplicationContainer *mdcApps = sched_parameters->mdcApps;
	Ptr<UniformRandomVariable> randomPosition = CreateObject<
			UniformRandomVariable>();
	double mdcSpeed = 3.0;
	Ptr<RandomRectanglePositionAllocator> randomPositionAllocator =
			CreateObject<RandomRectanglePositionAllocator>();
	Ptr<ListPositionAllocator> centerPositionAllocator = CreateObject<
			ListPositionAllocator>();

	Ptr<ListPositionAllocator> positionAlloc = CreateObject<
			ListPositionAllocator>();

	Ptr<ConstantRandomVariable> constRandomSpeed = CreateObject<
			ConstantRandomVariable>();
	Ptr<ConstantRandomVariable> constRandomPause = CreateObject<
			ConstantRandomVariable>(); //default = 0

	Vector center = Vector3D(boundaryLength / 2.0, boundaryLength / 2.0, 0.0);

	mobility->PopReferenceMobilityModel();

	RandomWaypointMobilityModel *r = NULL; //new RandomWaypointMobilityModel();
	mobility->SetMobilityModel("ns3::HierarchicalMobilityModel", "Child",
			PointerValue(r));
	positionAlloc->Add(center);
	for (int i = 0; i < NumberOfOutput; i++) {
		int idx = ALT_OUTPUT_PATH[i] - 1;
		positionAlloc->Add(Vector(Node_XCoor[idx], Node_YCoor[idx], 0.0));
	}
	positionAlloc->Add(center);

	mobility->SetPositionAllocator(positionAlloc);
	constRandomSpeed->SetAttribute("Constant", DoubleValue(mdcSpeed));

	mobility->SetMobilityModel("ns3::RandomWaypointMobilityModel", "Pause",
			PointerValue(constRandomPause), "Speed",
			PointerValue(constRandomSpeed), "PositionAllocator",
			PointerValue(positionAlloc));
	mobility->Install(*mdcs);

	for (NodeContainer::Iterator itr = mdcs->Begin(); itr != mdcs->End();
			itr++) {
		(*itr)->GetObject<MobilityModel>()->SetPosition(center);
	}

	//mobility->PushReferenceMobilityModel(mdcs->Get(0));

	mdcApps->Start(Seconds(EVENT_Time[4]));
	mdcApps->Stop(Seconds(simRunTime));
	//Simulator::Run();

	//Simulator::Stop(Seconds(simRunTime));
//	Simulator::ScheduleNow(scheduledEventsCallback, graphEdges, graphSensors,
//			mapGraphIntersections, countID + 1, sched_parameters);

	//Simulator::Run();

//	if (Simulator::Now().GetSeconds() < EVENT_Time[4] + 1)
//		Simulator::Schedule(Seconds(2), setNewMdcPosition,
//						sched_parameters);
}

void scheduledEventsCallback(list<GraphEdge> graphEdges,
		list<GraphSensors> graphSensors,
		map<int, GraphIntersections> mapGraphIntersections, uint32_t countID,
		Sched_Params * sched_parameters) {

	//NS_LOG_INFO("Graph builder scheduledEventsCallback");
	if (countID >= sizeof(EVENT_ID) / sizeof(uint32_t))
		countID -= sizeof(EVENT_ID) / sizeof(uint32_t);

	GraphSensors::SensorMode sMode;

	if (scenario == 1) {
		if (Simulator::Now().GetSeconds() > EVENT_Time[0]
				&& Simulator::Now().GetSeconds() <= EVENT_Time[1]) {
			sMode = GraphSensors::WARNING;
			pAnim->UpdateNodeColor(EVENT_ID[countID], 255, 140, 0);
			resetEdgeVisibility(graphEdges, mapGraphIntersections,
					EVENT_ID[countID], sMode);
		} else if (Simulator::Now().GetSeconds() > EVENT_Time[1]
				&& Simulator::Now().GetSeconds() <= EVENT_Time[2]) {
			sMode = GraphSensors::DANGER;
			pAnim->UpdateNodeColor(EVENT_ID[countID], 255, 0, 0);
			resetEdgeVisibility(graphEdges, mapGraphIntersections,
					EVENT_ID[countID], sMode);
		}
	} else if (scenario == 2) {
		if (Simulator::Now().GetSeconds() > EVENT_Time[0]
				&& Simulator::Now().GetSeconds() <= EVENT_Time[1]) {
			sMode = GraphSensors::FAIL;
			pAnim->UpdateNodeColor(EVENT_ID[countID], 0, 0, 0);
			resetEdgeVisibility(graphEdges, mapGraphIntersections,
					EVENT_ID[countID], sMode);
		}
	}
//		if (Simulator::Now().GetSeconds() > EVENT_Time[4]
//				&& Simulator::Now().GetSeconds()
//						<= EVENT_Time[4] + PERIOD_BETWEEN_EVENTS) {
//
//			Simulator::Stop(Seconds(EVENT_Time[4]));
//			NS_LOG_INFO("Updating!");
//			Simulator::Schedule(Seconds(PERIOD_BETWEEN_EVENTS),
//					setNewMdcPosition, graphEdges, graphSensors,
//					mapGraphIntersections, countID + 1, sched_parameters);
//		}

// Important: To prevent the simulation run endlessly
	if (Simulator::Now().GetSeconds() < EVENT_Time[4])
		Simulator::Schedule(Seconds(PERIOD_BETWEEN_EVENTS),
				scheduledEventsCallback, graphEdges, graphSensors,
				mapGraphIntersections, countID + 1, sched_parameters);
	else {
		//	Simulator::Stop();
	}

}

void MdcGraphBuilder::createRoadGraph(list<GraphSensors> gListSen,
		NodeContainer *mdcs, MobilityHelper *mobility,
		ApplicationContainer *mdcApps) {

	LogComponentEnable("MdcSimulationGraphBuilder", LOG_LEVEL_INFO);

	NS_LOG_INFO("createRoadGraph");

	NodeContainer nodeCon;
	nodeCon.Create(NumberOfNodes);

//LogComponentEnableAll(LOG_LEVEL_WARN);

	populateIntersections();
	populateEdges();

	Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator>();

	for (std::map<int, GraphIntersections>::iterator itrI =
			mapGraphIntersections.begin(); itrI != mapGraphIntersections.end();
			itrI++) {

		GraphIntersections gInt = (GraphIntersections) itrI->second;
		Vector2D v = gInt.getPoint();
		posAlloc->Add(Vector(v.x, v.y, 0.0));
	}

	mobility->SetPositionAllocator(posAlloc);

	for (uint32_t c = 0; c < nodeCon.GetN(); c++) {
		Ptr<Node> node = nodeCon.Get(c);

		for (std::map<int, GraphIntersections>::iterator itrI =
				mapGraphIntersections.begin();
				itrI != mapGraphIntersections.end(); itrI++) {

			if (itrI->second.getNodeID() == c + 1) {
				itrI->second.setActualID(node->GetId());
				break;
//				int temp = itrI->first;
//				GraphIntersections val = itrI->second;
//				mapGraphIntersections.erase(itrI);
//				mapGraphIntersections.insert(std::make_pair(temp, val));
			}
		}

		AnimationInterface::SetNodeColor(node, 192, 192, 192);
		AnimationInterface::SetConstantPosition(node, 0, 0);
		//AnimationInterface::SetNodeDescription(node, "Sensor");
	}

	connectTheDots(nodeCon);
	updateAdjacencyEdges();
	setGraphSensors(gListSen);
	assignSensorsToEdges();

	pAnim = new AnimationInterface("anim.xml");

	mobility->Install(nodeCon);

	mobility->AssignStreams(nodeCon, 0);
	AsciiTraceHelper ascii;
	MobilityHelper::EnableAsciiAll(
			ascii.CreateFileStream("mobility-trace-example.mob"));

	uint32_t countID = 0;
	Sched_Params sch;
	sch.mdcApps = mdcApps;
	sch.mdcs = mdcs;
	sch.mobility = mobility;

	Simulator::Schedule(Seconds(START_EVENTS), scheduledEventsCallback,
			graphEdges, graphSensors, mapGraphIntersections, countID, &sch);

//	Simulator::Stop(Seconds(EVENT_Time[4]));
//	Simulator::Run();
//	Simulator::Destroy();

//	Simulator::Schedule(Seconds(EVENT_Time[4] + 1), setNewMdcPosition,
//			graphEdges, graphSensors, mapGraphIntersections, countID + 1, &sch);
	Simulator::Stop(Seconds(simRunTime));
	Simulator::Run();
	Simulator::Destroy();

//	bool b = testInVicinity();
//	if (b)
//		NS_LOG_INFO("true");

	NS_LOG_INFO("Graph build simulation Done!");
}
