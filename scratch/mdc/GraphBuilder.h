/*
 * GraphBuilder.h
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */
#ifndef MDC_GRAPH_BUILDER_H
#define MDC_GRAPH_BUILDER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
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
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"
#include "ns3/brite-module.h"
#include "ns3/ipv4-nix-vector-helper.h"

#include "InsteadOfFile.h"
#include "mdc-event-sensor.h"
#include "mdc-collector.h"
#include "GraphIntersections.h"
#include "GraphEdge.h"
#include "GraphEdgeTuple.h"

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <stdlib.h>

struct RectCoOrd {
	float x1, x2, y1, y2;
};

namespace ns3 {

class MdcGraphBuilder {
public:
	MdcGraphBuilder();

	void SetAttribute(std::string name, const AttributeValue &value);
	void createRoadGraph(list<GraphSensors> gListSen, NodeContainer *mdcs,
			MobilityHelper *mobility, ApplicationContainer *mdcApps);

	void createGridGraph();

	std::list<GraphEdge>& getGraphEdges();
	void setGraphEdges(std::list<GraphEdge>& graphEdges);
//	const std::list<GraphIntersections>& getGraphIntersections();
//	void setGraphIntersections(
//			const std::list<GraphIntersections>& graphIntersections);

	std::map<int, GraphIntersections>& getGraphIntersections();
	void setGraphIntersections(
			const std::map<int, GraphIntersections>& graphIntersections);

	std::list<GraphSensors>& getGraphSensors();
	void setGraphSensors(std::list<GraphSensors> &graphSensors);
	void assignSensorsToEdges();
	void setSensorToEdges(list<GraphSensors>::iterator itrG, double x,
			double y);
	bool findInVicinity(RectCoOrd r, double x, double y);

	std::list<GraphEdge> graphEdges;
	std::list<GraphSensors> graphSensors;
	std::list<GraphIntersections> graphIntersections;
	std::map<int, GraphIntersections> mapGraphIntersections;

private:
	void populateIntersections();
	void populateEdges();
	void updateAdjacencyEdges();
	void connectTheDots(NodeContainer nodeCon);
	void installPtToPt(Ptr<Node> node1, Ptr<Node> node2);
	void resetEdgeVisibility(list<GraphEdge> graphEdges, uint32_t sid,
			GraphSensors::SensorMode sMode);
};

} // namespace ns3

#endif /* MDC_GRAPH_BUILDER_H */
