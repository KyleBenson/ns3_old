/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
 *
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
 *
 * Author: UCI
 */

#ifndef MDC_UTILITIES_H
#define MDC_UTILITIES_H

#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"
#include "ns3/output-stream-wrapper.h"
#include "sensed-event.h"
//#include "mdc-helper.h"
#include "ns3/position-allocator.h"

#include <math.h>
#include <set>
#include <queue>
#include <map>

#include <boost/config.hpp>
#include <iostream>
#include <fstream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>




#define EPSILON 0.0001

namespace ns3 {

	//------------------------
	struct EdgeDataT
	{
		unsigned nodeFrom;
		unsigned nodeTo;
		double weight;
	};

	struct Vertex_infoT
	{
		unsigned nodeId;
		std::string vertexName;
	};

	// These are used in the Shortest Path Computation
	struct WayPointT
	{
		Vector vLoc;
		double dETA;
	};

	struct EventStatsT
	{
		SensedEvent evnt;
		double sinkDataCaptureTime;
		double mdcDataCaptureTime;
		double sensorDataCaptureTime;
	};

	typedef boost::adjacency_list < boost::listS, boost::vecS, boost::undirectedS,
									Vertex_infoT, boost::property < boost::edge_weight_t, int > > GraphT;
	typedef boost::graph_traits < GraphT >::vertex_descriptor VertexDescriptor;
	typedef boost::graph_traits < GraphT >::edge_descriptor EdgeDescriptor;
	typedef boost::graph_traits< GraphT >::vertex_iterator VertexIterator;
	typedef boost::graph_traits< GraphT >::edge_iterator EdgeIterator;

	typedef std::pair<int, int> Edge;

	// These are the property accessors
	typedef boost::property_map<GraphT, std::string Vertex_infoT::*>::type VertexNamePropertyMap;
	typedef boost::property_map<GraphT, unsigned Vertex_infoT::*>::type VertexIdPropertyMap;
	typedef boost::property_map<GraphT, boost::edge_weight_t>::type EdgeWeightPropertyMap;

	// We keep track of the sensor locations here again...
	// Ideally, we shoul dhave just one copy.
	// We use these to compute the mobility values
	static std::vector<Vector> x_sensorLocations;
	static std::vector<EventStatsT> x_eventStats;
	static std::multimap<int,std::string> x_nodeGraphMultimap;
	static std::map<std::string, int> x_depotLocations;

	static std::vector<Ptr<Node> > m_allMDCNodes; // Keeping track of all the MDC Nodes
//	static std::map<uint32_t, SensedEvent> m_allSensedEvents; // Keeps a list of all sensed events for easy translation
	static Ptr<OutputStreamWrapper> m_mdcoutputStream; // output stream for tracing from MDC simulation


	Vector GetClosestVector (std::vector<Vector> posVector, Vector refPoint);
	bool IsSameVector (Vector *aV, Vector *bV);
	Vector CleanPosVector (Vector v);
	std::queue<unsigned> NearestNeighborOrder (std::vector<Vector> * inputVector, Vector refPoint);
	std::vector<Vector> ReSortInputVector (std::vector<Vector> * inputVector, std::queue<unsigned> sortSeq);

	void SetMDCOutputStream (Ptr<OutputStreamWrapper> outputStream);
	Ptr<OutputStreamWrapper> GetMDCOutputStream (void);
	void CreateTSPInput(std::vector<Vector> * inputVector, std::stringstream &s);
	void WriteTSPInputToFile(std::stringstream &s, const char *TSPFileName);
	int ExecuteSystemCommand(const char *TSPfileName);
	std::queue<unsigned> ReadTSPOutput(const char *TSPfileName);
	void PopulateTSPPosAllocator(std::vector<Vector> * inputVector, Ptr<ListPositionAllocator> listPosAllocator);
	void RecomputePosAllocator(Vector vCurrPos, Vector vDepotPos, std::vector<Vector> *inputVector, Ptr<ListPositionAllocator> listPosAllocator);

	bool compare_sensedEvents (const SensedEvent& first, const SensedEvent& second);
	bool RemoveVectorElement (std::vector<Vector> *inputVector, Vector refV);
	void PrintEventTrace(int sourceInd, Ptr<const Packet> packet );

	void AddMDCNodeToVector(Ptr<Node>  node);
	std::vector<Ptr<Node> > GetMDCNodeVector();

	std::vector<Vector> ReadVertexList(const char *vertexFileName);
	GraphT ReadGraphEdgeList(const char *edgeFileName, const char *graphName, std::vector<Vector> vertexList);
	void printTheGraph(GraphT g, const char *graphFileName);

	std::vector<Vector> GetSensorPositions();
	std::vector<EventStatsT> GetEventStats();
	void SetSensorPositions(std::vector<Vector> senPos);
	void StoreEventLocation(SensedEvent se);
	void UpdateEventStats(uint32_t eventId, int statItem, double capTime);
	void PrintEventStats();

	void AddNodeToMultimap(int nodeId, std::string graphName);
	std::vector<std::string> NodeIdToGraph(int nodeId);
	unsigned GetSensorNodeId(Vector pos);
	void PrintNodeGraphMultimap();

} // namespace ns3

#endif /* MDC_UTILITIES_H */
