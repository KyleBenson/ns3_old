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
#include "mdc-utilities.h"
#include "ns3/log.h"
#include "ns3/trace-helper.h"
#include "mdc-event-tag.h"

NS_LOG_COMPONENT_DEFINE ("MDCUtilities");

namespace ns3 {


	// Returns the position vector closest to the refPoint provided.
	Vector GetClosestVector (std::vector<Vector> posVector, Vector refPoint)
	{
		Vector retV;
		std::vector<Vector>::iterator it;
		double d = INFINITY;
		double temp = 0;
		for (it = posVector.begin(); it != posVector.end();)
		{
			temp = CalculateDistance(*it, refPoint);
			if (temp < d)
			{
				d = temp;
				retV.x = it->x;
				retV.y = it->y;
				retV.z = it->z;
			}
			it++;
		}
		return retV;
	}

	bool IsSameVector (Vector *aV, Vector *bV)
	{
		if (aV == bV)
			return true;
//		else
//			return false;

		if ((fabs((aV->x - bV->x)) < EPSILON) &&
			(fabs((aV->y - bV->y)) < EPSILON) &&
			(fabs((aV->z - bV->z)) < EPSILON) )
			return true;
		else
			return false;
	}

	Vector CleanPosVector (Vector v)
	{
		Vector w;
		if (fabs(v.x) < EPSILON) w.x = 0.0; else w.x=v.x;
		if (fabs(v.y) < EPSILON) w.y = 0.0; else w.y=v.y;
		if (fabs(v.z) < EPSILON) w.z = 0.0; else w.z=v.z;
		w.z = 0.0; // we are in a 2D world here.
		return w;
	}

	std::queue<unsigned> NearestNeighborOrder (std::vector<Vector> * inputVector, Vector refPoint)
	{
		std::queue<unsigned> orderSeq;
		std::set<unsigned> visitedSet;
		unsigned vectorSize = (*inputVector).size();

		double minDistance;
		Vector newRefPos; // Keeping track of the current ref point
		unsigned currMinIndex; // Keeping track of min node in the curr iteration

		newRefPos = refPoint;
//		std::vector<Vector>::iterator it;
		std::set<unsigned>::iterator it;

		for (unsigned j=0; j<vectorSize; j++)
		{
			minDistance = INFINITY;
			currMinIndex = 0;
			for (unsigned i=0; i<vectorSize; i++)
			{
				if (visitedSet.find(i) != visitedSet.end() )
				{
					// Node is already visited.
					;
				}
				else
				{
					double temp = CalculateDistance((*inputVector)[i], newRefPos);
					if (temp < minDistance)
					{
						currMinIndex = i;
						minDistance = temp;
					}
				}
			}
			orderSeq.push(currMinIndex);
			visitedSet.insert(currMinIndex);
			newRefPos = (*inputVector)[currMinIndex];
//			std::cout << "Next Nearest Sensor = " << newRefPos.x << "," << newRefPos.y << "," << newRefPos.z << "].\n";
		}
		return orderSeq;
	}

	std::vector<Vector> ReSortInputVector (std::vector<Vector> * inputVector, std::queue<unsigned> sortSeq)
	{
		std::vector<Vector> outputVector;
		unsigned u;
		while (!sortSeq.empty())
		{
			u = sortSeq.front();
			outputVector.push_back((*inputVector)[u]);
 			sortSeq.pop();
//			std::cout << "Sorted Sensor Position = [" << (*inputVector)[u].x << "," << (*inputVector)[u].y << "," << (*inputVector)[u].z << "].\n";
		}

		return outputVector;
	}

	void SetMDCOutputStream (Ptr<OutputStreamWrapper> outputStream)
	{
		m_mdcoutputStream = outputStream;
	}

	Ptr<OutputStreamWrapper> GetMDCOutputStream (void)
	{
		return m_mdcoutputStream;
	}

	void SetMDCVelocity (double vel) {} //TODO:
	double GetMDCVelocity () { return 11.0;} //TODO:
	double GetMDCVelocity (std::string graphName)
	{
		if (graphName.compare("H"))
			return 11.11;
		if (graphName.compare("T"))
			return 6.96;
		else
			return 1;

	} //TODO: Better accept this as a parameter


	void CreateTSPInput(std::vector<Vector> * inputVector, std::stringstream &s)
	{
		s << "NAME : MDC SIMULATION - TSP INPUT " << std::endl
				<< "COMMENT : Sensor Node Placement " << std::endl
				<< "TYPE : TSP "  << std::endl
				<< "DIMENSION: " << inputVector->size() << std::endl
				<< "EDGE_WEIGHT_TYPE : EUC_2D " << std::endl
				<< "NODE_COORD_SECTION" << std::endl;
		std::vector<Vector>::iterator it = inputVector->begin();
		for (int i=0 ; it != inputVector->end(); i++)
		{
			s << i << " " << it->x << " " << it->y << " " << std::endl;
			++it;
		}
		s << "EOF" << std::endl;
		return;
	}

	void WriteTSPInputToFile(std::stringstream &s, const char *TSPfileName)
	{
		/*
		std::ofstream tspFile(TSPfileName, std::ios::trunc);
		tspFile << s;
		tspFile.close();
		*/
		////
		AsciiTraceHelper asciiTraceHelper;
		Ptr<OutputStreamWrapper> outputStream = asciiTraceHelper.CreateFileStream (TSPfileName);
		*outputStream->GetStream () << s.str() << std::endl;
		////

	}

	int ExecuteSystemCommand(const char *cmd)
	{
		int i = std::system(cmd);
		return i;
	}

	std::queue<unsigned> ReadTSPOutput(const char *TSPSolfileName)
	{
		std::queue<unsigned> orderSeq;
		std::string s;
		unsigned count;
		unsigned nodeId;
		std::ifstream tspSolFile(TSPSolfileName);
		tspSolFile >> s;
		sscanf(s.c_str(),"%u", &count);
		std::string s1;
		for (unsigned i=0; i<count; i++)
		{
			tspSolFile >> s1;
			sscanf(s1.c_str(),"%u", &nodeId);
			orderSeq.push(nodeId);
			s1.clear();
		}
		std::cout << "TSP Solution Loaded in queue length..." << orderSeq.size() << std::endl;
		return orderSeq;
	}

	void PopulateTSPPosAllocator(
			std::vector<Vector> *inputVector,
			Ptr<ListPositionAllocator> listPosAllocator
			)
	{

		// 		Write Sensor positions into TSPLIB format
		// 		Generate TSP tour
		//		Get Closest sensor
		//		Populate ListPositionAllocator in TSP order
		std::stringstream tspInputStr;
		std::queue<unsigned> tempOrder;
		std::vector<Vector> posVectorSorted;

		CreateTSPInput(inputVector, tspInputStr);
		WriteTSPInputToFile(tspInputStr, "mdcInput.tsp");
		ExecuteSystemCommand("/u01/ns3/workspace/concorde/concorde_build/TSP/concorde mdcInput.tsp");
		tempOrder = ReadTSPOutput("mdcInput.sol");
		posVectorSorted = ReSortInputVector (inputVector, tempOrder);
		listPosAllocator->Dispose();  // We will be populating this Allocator
		for (uint32_t i=0; i< posVectorSorted.size(); i++)
		{
			listPosAllocator->Add(posVectorSorted[i]);
		}

	}

	void RecomputePosAllocator(
			Vector vCurrPos,
			Vector vDepotPos,
			std::vector<Vector> *inputVector,
			Ptr<ListPositionAllocator> listPosAllocator
			)
	{
		/**** Ideally this is what we need
		listPosAllocator->Add(vCurrPos);
		for (std::vector<Vector>::iterator it = inputVector->begin() ; it != inputVector->end(); ++it)
			listPosAllocator->Add(*it);
		listPosAllocator->Add(vDepotPos);
		***************/

		// Now copy the event locations yet to be visited as a tour into the return param...
		// Add the currPos and DepotPos at either end
		uint32_t i=0;
		//listPosAllocator->Add(vCurrPos); This is becoming very inaccurate somehow
		for (i=0; i<inputVector->size(); i++)
			listPosAllocator->Add(inputVector->at(i));
		listPosAllocator->Add(vDepotPos);

		// We need to optimize this route if possible.

		std::cout << "New Vector is:- \n";
		for (uint32_t j=0; j<i+1; j++)
			std::cout << "  " << listPosAllocator->GetNext() << std::endl;
		std::cout << "---- End of New Vector \n";


	}

	bool RemoveVectorElement (std::vector<Vector> *inputVector, Vector refV)
	{
		bool ret = false;
		std::vector<Vector>::iterator it;
		it = inputVector->begin();
		for (uint32_t i=0; i < inputVector->size(); i++)
		{
			if (fabs(((*it).x - refV.x)) < EPSILON)
				if (fabs(((*it).y - refV.y)) < EPSILON)
					if (fabs(((*it).z - refV.z)) < EPSILON)
					{
						inputVector->erase(inputVector->begin() + i);
						ret = true;
					}
			it++;
		}
		return ret;
	}

	bool compare_sensedEvents (const SensedEvent& first, const SensedEvent& second)
	{
	  return ( first.GetTime().GetSeconds() < second.GetTime().GetSeconds() );
	}


	// Trace function for the Events themselves
	void
	PrintEventTrace(int sourceInd, Ptr<const Packet> packet )
	{
		// 0-SinkTime, 1-CollectTime, 2-SensedTime

		MdcEventTag eventTag;

	//	packet->PrintByteTags(*(GetMDCOutputStream())->GetStream());
	//	*(GetMDCOutputStream())->GetStream() << std::endl;


		std::string s1, s2;

		ByteTagIterator bti = packet->GetByteTagIterator();
		while (bti.HasNext())
		{
			ns3::ByteTagIterator::Item bt = bti.Next();
			s1 = (bt.GetTypeId()).GetName();
			s2 = (eventTag.GetTypeId()).GetName();
			//std::cout << "Comparing " << s1 << " and " << s2 << std::endl;

			if (s1.compare(s2)==0)
			{
				//std::cout << "Found a match for " << s2 << std::endl;
				bt.GetTag(eventTag);
				break;
			}
		}

		std::stringstream s, csv, sensedTime, collectTime, sinkTime;

		sensedTime.clear();
		collectTime.clear();
		sinkTime.clear();
		sensedTime << "0";
		collectTime << "0";
		sinkTime << "0";

		if (sourceInd == 0) // print sinkTime
		{
			sinkTime << Simulator::Now ().GetSeconds ();
			UpdateEventStats(eventTag.GetEventId(), sourceInd, Simulator::Now ().GetSeconds());
		}
		else if (sourceInd == 1) // print collectTime
		{
			collectTime << Simulator::Now ().GetSeconds ();
			UpdateEventStats(eventTag.GetEventId(), sourceInd, Simulator::Now ().GetSeconds());
		}
		else if (sourceInd == 2) // print sensedTime
		{
			sensedTime << Simulator::Now ().GetSeconds ();
			UpdateEventStats(eventTag.GetEventId(), sourceInd, Simulator::Now ().GetSeconds());
		}

		s << "[EVENT_DELAY] Event Id=<" << eventTag.GetEventId() << "> SchedTime=<" << eventTag.GetTime()
				<< "> SensedTime=<" << sensedTime.str() << "> collectTime=<" << collectTime.str()
				<< "> SinkTime=<" << sinkTime.str() << ">" << std::endl;
	    csv << "EVENT_DELAY," << eventTag.GetEventId() << "," << eventTag.GetTime() << "," << sensedTime.str() << ","
	    		<< collectTime.str() << "," << sinkTime.str() << "," << 'N' << std::endl;
	    *(GetMDCOutputStream())->GetStream() << csv.str();
	    NS_LOG_INFO(s.str());
	}

	void AddMDCNodeToVector(Ptr<Node>  n)
	{
		m_allMDCNodes.push_back(n);
	}

	std::vector<Ptr<Node> > GetMDCNodeVector()
	{
		return m_allMDCNodes;
	}

	std::vector<Node_infoT> ReadVertexList(const char *vertexFileName)
	{
		std::string s;
		unsigned count;
		Node_infoT v;
		unsigned nodeId;
		std::ifstream vertexFile(vertexFileName);

		// The first few lines will be:
		// NAME ...
		// COMMENT ...
		// DIMENSION ...
		// NODE_COORD_SECTION

		// Clear the x_sensorPositions vector first.
		x_sensorLocations.clear();
		x_nodeLocations.clear();

		while (!vertexFile.eof())
		{
			getline(vertexFile,s);
			if (s.length() == 0)
				continue;

			if (	(s.compare(0, 4, "NAME") == 0) ||
					(s.compare(0, 7, "COMMENT") == 0) ||
					(s.compare(0, 18, "NODE_COORD_SECTION") == 0) ||
					(s.compare(0, 3, "EOF") == 0) )
				std::cout << "Reading..." << s << std::endl;
			else if ((s.compare(0, 9, "DIMENSION") == 0))
			{
				if (sscanf(s.c_str(),"DIMENSION %u", &count) == 1)
					std::cout << "Expectng..." << count << " nodes." << std::endl;
				else
					std::cout << "... UNKNOWN DIMENSION..." << s << std::endl;

				count = 0;
			}
			else
			{
				/*
					NAME MDC SIMULATION - SHANTYTOWN NODELIST INPUT
					COMMENT Sensor Node Relative Placement provided by IIST
					DIMENSION 11868
					NODE_COORD_SECTION
					1	30518346	-686.5472222223	-202.6833333332
					2	30518455	-68.7583333333	216.0444444446
					3	30518460	601.6888888889	681.7750000002
					4	30518464	817.713888889	858.8138888891
					5	213033769	757.5083333334	794.6361111114
				 *
				 */
				char nodeNameStr[20];
				v.nodePos.x=0; v.nodePos.y=0; v.nodePos.z=0;
				if (sscanf(s.c_str(),"%u %s %lf %lf", &nodeId, nodeNameStr, &v.nodePos.x, &v.nodePos.y) == 4)
				{
					v.nodeId = nodeId;
					v.nodeName = nodeNameStr;
					x_nodeLocations.push_back(v);
					x_sensorLocations.push_back(v.nodePos);

					// Add the NodeInfoT to the x_nodeMap also
					x_nodeMap.insert(std::pair<int, Node_infoT>(nodeId, v));

					std::cout << "... Adding Vertex [" << v.nodeId << "] NodeName [" << v.nodeName << "] at [" << v.nodePos << "]." << std::endl;
				}
				else
					std::cout << "... No Vertex info found in.." << s << std::endl;

				count++;
			}


		}
		std::cout << "Recorded..." << count << " nodes." << std::endl;

		return GetNodePositions();
	}

	GraphT ReadGraphEdgeList(const char *edgeFileName, const char *graphName, std::vector<Node_infoT> vertexList)
	{
		std::vector<EdgeDataT> edgeList;
		std::string s;
		unsigned count;
		EdgeDataT eDat;

		std::ifstream edgeFile(edgeFileName);

		/* The first few lines will be:
			NAME MDC SIMULATION - SHANTYTOWN NODELIST INPUT
			COMMENT Sensor Node Relative Placement provided by IIST
			DIMENSION 12315
			DEPOT	g1	24
			EDGE_LIST_SECTION
			24    22    g1    86.9941483975268
			22    231    g1    58.5001204787593
			11864    11865    g1    11.6996078566915
			11865    11866    g1    6.54295068641896
			11866    11868    g1    32.0056339697657
		 */
		while (!edgeFile.eof())
		{
			getline(edgeFile,s);
			if (s.length() == 0)
				continue;

			if (	(s.compare(0, 4, "NAME") == 0) ||
					(s.compare(0, 7, "COMMENT") == 0) ||
					(s.compare(0, 17, "EDGE_LIST_SECTION") == 0) ||
					(s.compare(0, 3, "EOF") == 0) )
				std::cout << "Reading..." << s << std::endl;
			else if ((s.compare(0, 5, "DEPOT") == 0))
			{
				char s1[10], s2[10];

				if (sscanf(s.c_str(),"DEPOT %s %s", s1, s2) == 2)
				{
					if (strcmp(s1,graphName) == 0)
					{
						int nodeId = std::atoi(s2);
						std::cout << "Depot Location for Graph " << s1 << " set to Node " << nodeId << ".\n";

						// Store the depot location of this graph
						x_depotLocations.insert(std::pair<std::string, int>(s1, nodeId));
					}
					else
						std::cout << "... Skipping entry [" << s << "]." << std::endl;
				}
				else
					std::cout << "... UNKNOWN DEPOT CONFIGURATION..." << s << std::endl;
			}
			else if ((s.compare(0, 9, "DIMENSION") == 0))
			{
				if (sscanf(s.c_str(),"DIMENSION %u", &count) == 1)
					std::cout << "Expectng..." << count << " edges." << std::endl;
				else
					std::cout << "... UNKNOWN DIMENSION..." << s << std::endl;

				count = 0;
			}
			else
			{
				/* Each edge is represented as:
				24    22    g1    86.9941483975268
				22    231    g1    58.5001204787593
				11864    11865    g1    11.6996078566915
				11865    11866    g1    6.54295068641896
				11866    11868    g1    32.0056339697657
				 */

				char s1[10], s2[10], s3[10];
				double wt;

				if (sscanf(s.c_str(),"%s %s %s %lf", s1, s2, s3, &wt) == 4)
				{
					if (strcmp(s3,graphName) ==0)
					{
						eDat.nodeFrom = std::atoi(s1);
						eDat.nodeTo = std::atoi(s2);
						eDat.weight = wt;
						edgeList.push_back(eDat);
						std::cout << "Adding edge " << eDat.nodeFrom << "<===>" << eDat.nodeTo << " with weight =" << eDat.weight << ".\n";
						count++;

						// Store the node <==> graph association
						AddNodeToMultimap(eDat.nodeFrom, s3);
						AddNodeToMultimap(eDat.nodeTo, s3);
					}
					else
					{
						//std::cout << "... Skipping entry [" << s << "]." << std::endl;
					}
				}
				else
				{
					//std::cout << "... Edge Info Not found in.." << s << std::endl;
				}
			}
		}
		std::cout << "Recorded..." << count << " edges." << std::endl;



		//	Edge edge_array[count];
		//	double weights[count];
		//	for (unsigned i=0; i<count; i++)
		//	{
		//		edge_array[i] = Edge(edgeList.at(i).nodeFrom, edgeList.at(i).nodeTo);
		//		weights[i] = edgeList.at(i).weight;
		//	}


		// At this point, you have an edgeList and a vertexList fully populated.
		// This is enough to create a graph and return it.

		// Create an empty graph
		unsigned num_nodes = vertexList.size();
		GraphT g(num_nodes);

		// Now add the vertices...
		VertexDescriptor vd;
		VertexNamePropertyMap vertexNameMap = boost::get(&Vertex_infoT::vertexName, g);
		VertexIdPropertyMap vertexIdMap = boost::get(&Vertex_infoT::nodeId, g);
		for (unsigned i=0; i<num_nodes; i++)
		{
			// for each vertex add the NodeId and Vertex Name info... hopefully it will be useful later.
			vd = vertex(i,g);
			vertexIdMap[vd] = vertexList[i].nodeId;
			vertexNameMap[vd] = vertexList[i].nodeName;
		}

		// Then add the edges to that graph
		unsigned num_edges = edgeList.size();
		VertexDescriptor u, v;
		for (unsigned i=0; i<num_edges; i++)
		{
			u = edgeList.at(i).nodeFrom;
			v = edgeList.at(i).nodeTo;
			add_edge(u, v, edgeList.at(i).weight, g);
		}

		/***** Only if you like to print all edges...
		EdgeDescriptor e;
		EdgeIterator ei, ei_end;
		EdgeWeightPropertyMap weightMap = boost::get(boost::edge_weight, g);
		for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
		{
			e = *ei;
			u = source(e, g), v = target(e, g);
			std::cout << u << " -> " <<  v  << " Weight =" << get(weightMap, e) << std::endl;
		}
		*****************/


		/******* An alternate way of creating the graph... might work too!
		GraphT g(edge_array, edge_array + num_arcs, weights, num_nodes);

		VertexNamePropertyMap vertexName = boost::get(&Vertex_infoT::vertexName, g);
		VertexIdPropertyMap vertexId = boost::get(&Vertex_infoT::nodeId, g);
		for (int i=0; i<num_nodes; i++)
		{
			vertexId[i] = vertexList[i].nodeId;
			vertexName[i] = vertexList[i].nodeName;
		}
		************/

		return g;

	}

	void printTheGraph(GraphT g, const char *graphFileName)
	{

		// This code snippet simply creates a visualization of the shortest path.
		EdgeWeightPropertyMap weightMap = boost::get(boost::edge_weight, g);
		//VertexNamePropertyMap vertexNameMap = boost::get(&Vertex_infoT::vertexName, g);
		//VertexIdPropertyMap nodeIdMap = boost::get(&Vertex_infoT::nodeId, g);
		//std::ofstream dot_file("figs/dijkstra-eg.dot");
		std::ofstream dot_file(graphFileName, std::ofstream::out);

		dot_file << "digraph D {\n"
		<< "  rankdir=LR\n"
		<< "  size=\"4,3\"\n"
		<< "  ratio=\"fill\"\n"
		<< "  edge[style=\"bold\"]\n" << "  node[shape=\"circle\"]\n";

		EdgeDescriptor e;
		VertexDescriptor u, v;
		EdgeIterator ei, ei_end;
		for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
		{
			e = *ei;
			u = source(e, g), v = target(e, g);
			//dot_file
			dot_file << u << " -> " <<  v  << "[label=\"" << get(weightMap, e) << "\"" ;
//			std::cout << u << " -> " <<  v  << " Weight=" << get(weightMap, e) << std::endl;

			// Use this snippet below to show the shortest path... comment the hard code to black below.
			dot_file << ", color=\"black\"";
			//		if (p[v] == u)
			//			dot_file << ", color=\"black\"";
			//		else
			//			dot_file << ", color=\"grey\"";
			dot_file << "]" << std::endl;
		}
		dot_file << "}" << std::endl;

	}

	std::vector<Node_infoT> GetNodePositions()
	{
		return x_nodeLocations;
	}

	void SetNodePositions(std::vector<Node_infoT> allNodePos)
	{
		x_nodeLocations = allNodePos;
	}

	std::vector<Vector> GetSensorPositions()
	{
		return x_sensorLocations;
	}

	void SetSensorPositions(std::vector<Vector> senPos)
	{
		x_sensorLocations = senPos;
	}

	std::vector<EventStatsT> GetEventStats()
	{
		return x_eventStats;
	}

	void StoreEventLocation(SensedEvent se)
	{
		EventStatsT es;
		es.evnt = se;
		es.sinkDataCaptureTime=0;
		es.mdcDataCaptureTime=0;
		es.sensorDataCaptureTime=0;
		x_eventStats.push_back(es);
	}

	void UpdateEventStats(uint32_t eventId, int statItem, double capTime)
	{
		// See the values sent by PrintEventTrace
		// Note that eventId starts from 1 for readability...
		if ((x_eventStats[eventId-1].sinkDataCaptureTime == 0) && (statItem == 0))
			x_eventStats[eventId-1].sinkDataCaptureTime = capTime;
		else if ((x_eventStats[eventId-1].mdcDataCaptureTime == 0) && (statItem == 1))
			x_eventStats[eventId-1].mdcDataCaptureTime = capTime;
		else if ((x_eventStats[eventId-1].sensorDataCaptureTime == 0) && (statItem == 2))
			x_eventStats[eventId-1].sensorDataCaptureTime = capTime;
	}

	void PrintEventStats()
	{
		std::cout << "Event Data Capture Statistics....Begin" << std::endl;
		for(size_t i=0; i<x_eventStats.size(); i++)
		{
			std::cout << "Id: " << x_eventStats[i].evnt.GetEventId() <<
					" Location: " << x_eventStats[i].evnt.GetCenter() <<
					" EventTime: " << x_eventStats[i].evnt.GetTime().GetSeconds() <<
					" SensedTime: " << x_eventStats[i].sensorDataCaptureTime <<
					" CollectedTime: " << x_eventStats[i].mdcDataCaptureTime <<
					" SinkTime: " << x_eventStats[i].sinkDataCaptureTime <<
					std::endl;
		}
		std::cout << "Event Data Capture Statistics....End" << std::endl;

	}

	void AddNodeToMultimap(int nodeId, std::string graphName)
	{
		// As a node is added to the graph, this method is called so that we keep track of it in a separate structure outside the graph itself.
		// In reality, i am sure there may be many different ways to achieve this but this is one naive way.
		bool found = false;

	    std::pair <std::multimap<int,std::string>::iterator, std::multimap<int,std::string>::iterator> rangeKeys;
	    rangeKeys = x_nodeGraphMultimap.equal_range(nodeId);
	    for (std::multimap<int,std::string>::iterator it=rangeKeys.first; it!=rangeKeys.second; ++it)
	    {
	    	if (graphName.compare(it->second)==0)
	    	{
	    		found = true;
	    		break;
	    	}
	    }
	    if (!found)
	    	x_nodeGraphMultimap.insert(std::pair<int,std::string>(nodeId, graphName));
	}

	std::vector<std::string> NodeIdToGraph(int nodeId)
	{
		// This translates the NodeId to a graph or set of graphs
		// Useful if you want to change the route of multiple MDCs running on different trajectories.
		std::vector<std::string> gNames;
	    std::pair <std::multimap<int,std::string>::iterator, std::multimap<int,std::string>::iterator> rangeKeys;
	    rangeKeys = x_nodeGraphMultimap.equal_range(nodeId);

	    for (std::multimap<int,std::string>::iterator it=rangeKeys.first; it!=rangeKeys.second; ++it)
	    	gNames.push_back(it->second);

	    return gNames;
	}

	int GetSensorNodeId(Vector pos)
	{
		// TODO: Obviously not very efficient... Will take O(n) time
		for (unsigned i=0; i< x_nodeLocations.size(); i++)
		{
			if (IsSameVector(&x_nodeLocations[i].nodePos,&pos))
				return x_nodeLocations[i].nodeId;
		}
		std::cout << "No Sensor found at position: " << pos << std::endl;
		return -1;
	}

	void PrintNodeGraphMultimap()
	{
		// Use for debugging purposes only.
		std::cout << "Multimap has " << x_nodeGraphMultimap.size() << " entries.\n";
	    for (std::multimap<int,std::string>::iterator it=x_nodeGraphMultimap.begin(); it!=x_nodeGraphMultimap.end(); ++it)
		       std::cout << (*it).first << " => " << (*it).second << '\n';
		std::cout << "End of multimap entries.\n";
	}

	// This is a utility function that will be needed
	Vector GetDepotPosition(std::string graphName)
	{
		Vector v;
		v.x=0; v.y=0; v.z=0;
		std::map<std::string, int>::iterator it = x_depotLocations.find(graphName);
		if (it == x_depotLocations.end())
		{
			std::cout << "ERROR... Depot Location Not available... Assuming [0,0,0]\n";
			return v;
		}
		else
		{
			// it-> second points to the nodeId of the Depot node.
			// Now you need to translate that to a NodeLocation.
			std::map<int, Node_infoT>::iterator itNI = x_nodeMap.find(it->second);
			if (itNI == x_nodeMap.end())
			{
				std::cout << "ERROR... Depot Location Not found for...NodeId= " << it->second << std::endl;
				return v;
			}
			else
			{
				v.x = itNI->second.nodePos.x;
				v.y = itNI->second.nodePos.y;
				v.z = itNI->second.nodePos.z;
				return v;
			}
		}

	}

	std::vector<WayPointT> GetWaypointVector(std::string graphName)
	{
		std::vector<WayPointT> v;
		std::map<std::string, std::vector<WayPointT> >::iterator it = x_WPVectorMap.find(graphName);
		if (it == x_WPVectorMap.end())
		{
			std::cout << "ERROR... Waypoint Vector Not available... NULL returned\n";
			return (v);
		}
		else
		{
			// Make sure that the WP Vector has atleast the depot. The starting place for the MDC.
			if (it->second.size()==0)
			{
				// This will get executed only once per initialization/graph
				WayPointT depotPoint;
				depotPoint.dDistance = 0.0;
				depotPoint.dETA = 0.0;
				depotPoint.dEventTime = 0.0;
				depotPoint.vLoc = GetDepotPosition(graphName);
				it->second.push_back(depotPoint);
			}

			//std::cout << "Waypoint Vector contains ..." << it->second.size() << " entries\n";
			return (it->second);
		}

	}

	void PrintWaypointVector(std::string graphName)
	{
		std::vector<WayPointT> v;
		std::map<std::string, std::vector<WayPointT> >::iterator it = x_WPVectorMap.find(graphName);

		GraphT g = GetGraph(graphName);
		std::vector<VertexDescriptor> predVector(num_vertices(g));
		std::vector<int> distVector(num_vertices(g));
		VertexDescriptor src;
		VertexDescriptor dest;
		std::vector<VertexDescriptor> segmentPath;

		if (it == x_WPVectorMap.end())
		{
			std::cout << "ERROR... Waypoint Vector Not available... NULL returned\n";
		}
		else
		{
			std::cout << "\nPRINTING Waypoint Vector : " << graphName << std::endl;
			std::vector<WayPointT> wpVec = it->second;
			for (int i=0; i<(int)wpVec.size(); i++)
			{
				std::cout << " EventTime=" << wpVec[i].dEventTime
						<< " Location=" << wpVec[i].vLoc
						<< " Distance=" << wpVec[i].dDistance
						<< " ETA=" << wpVec[i].dETA
						<< std::endl;

				if (i>0)
				{
					src = vertex(GetSensorNodeId(wpVec[i-1].vLoc),g);
					dest = vertex(GetSensorNodeId(wpVec[i].vLoc),g);
					distVector = GetShortestPathsFromSource(g, src, &predVector);
					segmentPath = GetShortestPathBetweenVertices(g, predVector, src, dest);
				}

			}
			std::cout << "Waypoint Vector for " << graphName << " has "<< wpVec.size() << " entries." << std::endl;

		}
	}


	void PrintGraphRoute(std::string graphName, const char *graphFileName)
	{

		// This code snippet simply creates a visualization of the shortest path.

		std::vector<WayPointT> v;
		std::map<std::string, std::vector<WayPointT> >::iterator it = x_WPVectorMap.find(graphName);

		GraphT g = GetGraph(graphName);
		std::vector<VertexDescriptor> predVector(num_vertices(g));
		std::vector<int> distVector(num_vertices(g));
		VertexDescriptor src;
		VertexDescriptor dest;
		std::vector<VertexDescriptor> segmentPath;

		if (it == x_WPVectorMap.end())
		{
			std::cout << "ERROR... Waypoint Vector Not available... NULL returned\n";
		}
		else
		{
			std::ofstream dot_file(graphFileName, std::ofstream::out);
			dot_file << "digraph G { \nnode[pin=true]; "; // \nrankdir=LR";

			std::vector<WayPointT> wpVec = it->second;

			for (int i=0; i<(int)wpVec.size(); i++)
			{
				// Use this as a prefix for Node Information

				std::string wpTripPrefix;
				char chStr[15];
				sprintf(chStr, "%d.", i);
				wpTripPrefix = chStr;

				if (i>0)
				{
					src = vertex(GetSensorNodeId(wpVec[i-1].vLoc),g);
					dest = vertex(GetSensorNodeId(wpVec[i].vLoc),g);
					distVector = GetShortestPathsFromSource(g, src, &predVector);
					segmentPath = GetShortestPathBetweenVertices(g, predVector, src, dest);

					for ( size_t j = 0; (j< segmentPath.size()); j++)
					{
						dot_file << std::endl << wpTripPrefix << segmentPath.at(j) << " "
								<< "[style=\"rounded,filled\", shape=box ";

						if (j==0)
						{
							dot_file << ", fillcolor=green ,label=\""
									<< segmentPath.at(j)
									<< " &#92;nEventTime=" << wpVec[i].dEventTime
									<< " \" ";
						}
						else if (j==segmentPath.size()-1)
						{
							dot_file << ", fillcolor=red ,label=\""
								<< segmentPath.at(j)
								<< " &#92;nEventTime=" << wpVec[i].dEventTime
								<< " &#92;nDistance=" << wpVec[i].dDistance
								<< " &#92;nETA=" << wpVec[i].dETA
								<< " \" ";
						}
						else
						{
							dot_file << ", fillcolor=yellow ,label=\""
									<< segmentPath.at(j)
									<< " \" ";
						}
						dot_file << "]";

					}

					for ( size_t j = 1; (j< segmentPath.size()); j++)
					{
						dot_file << std::endl
								<< wpTripPrefix << segmentPath.at(j-1)
								<< " -> "
								<< wpTripPrefix << segmentPath.at(j);
					}
				}
			}
			dot_file << "\n}" << std::endl;
		}

	}



	void CreateNS2TraceFromWaypointVector(uint32_t mdcNodeId, std::string graphName, const char *ns2TraceFileName, const std::ofstream::openmode openmode)
	{
		std::vector<WayPointT> v;
		std::map<std::string, std::vector<WayPointT> >::iterator it = x_WPVectorMap.find(graphName);

		GraphT g = GetGraph(graphName);
		std::vector<VertexDescriptor> predVector(num_vertices(g));
		std::vector<int> distVector(num_vertices(g));
//		VertexDescriptor src;
//		VertexDescriptor dest;
		std::vector<VertexDescriptor> segmentPath;

		std::ofstream ns2Trcfile(ns2TraceFileName, openmode);

		if (it == x_WPVectorMap.end())
		{
			std::cout << "ERROR... Waypoint Vector Not available... NULL returned\n";
		}
		else
		{
			/*
			 * The ns2 trace file format goes something like this...
			 *  $node_(0) set X_ 329.82427591159615
				$node_(0) set Y_ 66.06016140869389
				$ns_ at 91.87745989691848 "$node_(0) setdest 378.37542668840655 45.5928630482057 0.0"
				$ns_ at 219.47118355124258 "$node_(0) setdest 286.6872580249029 142.51631507750932 0.0"
				$ns_ at 352.52885253886916 "$node_(0) setdest 246.3202938897401 107.57197005511536 0.0"
				$ns_ at 579.1889287677668 "$node_(0) setdest 27.380316338943658 186.9421090132031 0.0"
				$ns_ at 874.8806114578338 "$node_(0) setdest 241.0193442850251 42.45159418309071 0.0"

				$ns at $time $node setdest \<x2\> \<y2\> \<speed\>
				At $time sec, the node would start moving from its initial position of (x1,y1) towards a destination (x2,y2) at the defined speed.

			 *
			 */
/*
*/
			Vector mdcLoc = GetDepotPosition(graphName);
			ns2Trcfile << "$node_(" << mdcNodeId << ") set X_ " << mdcLoc.x << std::endl;
			ns2Trcfile << "$node_(" << mdcNodeId << ") set Y_ " << mdcLoc.y << std::endl;

			std::vector<WayPointT> wpVec = it->second;
			for (int i=0; i<(int)wpVec.size(); i++)
			{
				ns2Trcfile << "$ns_ at " << wpVec[i].dETA << " \"$node_(" <<
						mdcNodeId << ") setdest "
						<< wpVec[i].vLoc.x << " "
						<< wpVec[i].vLoc.y << " "
						<< " 1.0"    // This is the speed
						<< "\""
						<< std::endl;

				/* This somehow failed to move the nodes properly
				ns2Trcfile << "$ns at " << wpVec[i].dETA << " " <<
						mdcNodeId << " setdest "
						<< wpVec[i].vLoc.x << " "
						<< wpVec[i].vLoc.y << " "
						<< " 5.0"    // This is the speed
						<< " "
						<< std::endl;
						*/



//				if (i>0)
//				{
//					src = vertex(GetSensorNodeId(wpVec[i-1].vLoc),g);
//					dest = vertex(GetSensorNodeId(wpVec[i].vLoc),g);
//					distVector = GetShortestPathsFromSource(g, src, &predVector);
//					segmentPath = GetShortestPathBetweenVertices(g, predVector, src, dest);
//				}

			}
//			std::cout << "Waypoint Vector for " << graphName << " has "<< wpVec.size() << " entries." << std::endl;

		}
	}


	void SetWaypointVector(std::string graphName,std::vector<WayPointT> newWPVec)
	{
		std::map<std::string, std::vector<WayPointT> >::iterator it = x_WPVectorMap.find(graphName);
		if (it == x_WPVectorMap.end())
		{
			std::cout << "ERROR... Waypoint Vector Not available... NULL returned\n";
			x_WPVectorMap.insert(std::pair<std::string, std::vector<WayPointT> >(graphName, newWPVec) );
		}
		else
		{
			it->second = newWPVec;
			//std::cout << "Waypoint Vector contains ..." << it->second.size() << " entries\n";
		}
	}

	void AddGraph(std::string graphName, GraphT g)
	{
    	x_GraphMap.insert(std::pair<std::string, GraphT>(graphName, g));

    	// If you are adding a new graph entry then make sure you add a new entry for the Waypoint vector for the graph
    	std::vector<WayPointT> newWPVec;
		x_WPVectorMap.insert(std::pair<std::string, std::vector<WayPointT> >(graphName, newWPVec) );
	}

	GraphT GetGraph(std::string graphName)
	{
		GraphT g;
		std::map<std::string, GraphT >::iterator it = x_GraphMap.find(graphName);
		if (it == x_GraphMap.end())
		{
			std::cout << "ERROR... Graph Object Not available... NULL returned\n";
			return (g);
		}
		else
		{
			return (it->second);
		}
	}

	// newPoint will be added 'after' insertLoc position (0-based)
	std::vector<WayPointT> CreateNewWaypointVector(std::string graphName, int insertLoc, WayPointT newPoint)
	{
		// Start with the Waypoint vector saved already.
		std::vector<WayPointT> prevWPVector = GetWaypointVector(graphName);
		std::vector<WayPointT> newWPVector;

		// We use these variables to navigate thru the WPVectors...
		int origWPIndex = insertLoc; // Current point on Orig WP Vector
		int newWPIndex = insertLoc+1;  // Current entry on New WP Vector
		// Hopefully you won't need to do arithmetic a lot on insertLoc after this...

		// Fetch the graph object where this new waypoint should be evaluated
		GraphT g = GetGraph(graphName);

		// First copy all the waypoints until and incl the insertLoc into a new WP Vector.
		for (int i=0; i<(int)prevWPVector.size(); i++)
		{
			newWPVector.push_back(prevWPVector[i]);
			if (i==insertLoc) break;
		}

		int newNodeId = GetSensorNodeId(newPoint.vLoc);
		if (newNodeId == -1)
		{
			std::cout << "ERROR... Unable to insert waypoint... " << newPoint.vLoc << std::endl;
		}
		else
		{
			WayPointT wp;
			// Add the node into the New WP Vector and evaluate the route from insertLoc --> newNodeLoc

			// This is the NodeId of the node after which the new Node will be added
			int insertLocNodeId = GetSensorNodeId(prevWPVector[origWPIndex].vLoc);

			VertexDescriptor src = vertex(insertLocNodeId, g);
			VertexDescriptor dest = vertex(newNodeId, g);
			Vector vDep = GetDepotPosition(graphName);

			// This will be a collection of the predecessors on the shortest path in the graph.
			// Note that this needs to be passed as ref so that the shortest path algo can write to it
			std::vector<VertexDescriptor> predVector(num_vertices(g));

			// This is a collecton that gets populated to get the distances
			// from the source identified to each of the other vertices on the shortest path
			std::vector<int> distVector(num_vertices(g));
			// First Run the Dijkstra's Shortest path algorithm to populate the distance and predecessor vectors.
			distVector = GetShortestPathsFromSource(g, src, &predVector);

			// You may not need this but this gives you the actual route of the best path between src and dest
			// getShortestPathBetweenVertices(g, predVector, src, dest);

			double newLocDistance1 = GetBestCostBetweenVertices(g, distVector, src, dest);
			// Compute the ETA of the new Waypoint
			wp.dDistance = prevWPVector[origWPIndex].dDistance + newLocDistance1;
			wp.dEventTime = newPoint.dEventTime;
			// The ETA will be the max(ETA of the Previous Location, CurrEventTime) + Time to travel to the new location
			wp.dETA = ((wp.dEventTime > prevWPVector[origWPIndex].dETA) ? wp.dEventTime :
					prevWPVector[origWPIndex].dETA
						)
							+ newLocDistance1/GetMDCVelocity(graphName); // Really assuming the velocity is the same all along the route
			wp.vLoc = newPoint.vLoc;
			newWPVector.push_back(wp); //
			newWPIndex++; // Get ready to be at the next entry
			std::cout << "Added New Step <" << GetSensorNodeId(wp.vLoc) << ">.\n";

			// If the insertLoc is after the last way point, then you are done.
			// If not, compute the route from the newLoc to that waypoint.
			if (origWPIndex == (int)prevWPVector.size()-1)
			{
				// There should no more WPs to consider...
				// Simply send the MDC to Depot and that is the end.
			}
			else
			{
				origWPIndex++; // Now you are on the next loc after the insertLoc
				// Now the next step is to analyze the next leg... newLoc --> insertLoc+1

				// Even if the next waypoint is the Depot, then you do not want to skip that stop here...
				// This is because there may be a long gap between events and the MDC is resting at the Depot
				// vDep is the Depot position
				insertLocNodeId = GetSensorNodeId(prevWPVector[origWPIndex].vLoc);

				src = vertex(newNodeId, g);
				dest = vertex(insertLocNodeId, g);

				// Run the Dijkstra's Shortest path algorithm again and populate the distance and predecessor vectors.
				distVector = GetShortestPathsFromSource(g, src, &predVector);

				// You may not need this but this gives you the actual route of the best path between src and dest
				// getShortestPathBetweenVertices(g, predVector, src, dest);

				double newLocDistance2 = GetBestCostBetweenVertices(g, distVector, src, dest);
				// Compute the ETA on the next stop in terms of the previous stop
				wp.dDistance = newWPVector[newWPIndex-1].dDistance + newLocDistance2;
				wp.vLoc = prevWPVector[origWPIndex].vLoc;
				// TODO: EventTime must be the set properly ...
				if (IsSameVector(&wp.vLoc, &vDep))
					wp.dEventTime = newWPVector[newWPIndex-1].dEventTime;
				else
					wp.dEventTime = prevWPVector[origWPIndex].dEventTime;

				// ETA is based on ETA on the previous node
				wp.dETA = ((wp.dEventTime > newWPVector[newWPIndex-1].dETA) ?
						wp.dEventTime :
						newWPVector[newWPIndex-1].dETA
						)
							+ newLocDistance2/GetMDCVelocity(graphName); // Again assuming uniform velocity
				newWPVector.push_back(wp);
				newWPIndex++; // Position on the next insertion point
				std::cout << "Added Next Step <" << GetSensorNodeId(wp.vLoc) << ">.\n";


				// At this point, you have handled... prevWPVector[origWPIndex]
				//     and newWPVector[newWPIndex] <-- the two may be off by 1 because we added the NewWPLoc.
				origWPIndex++; // Now this is should keep you on insertLoc +2
				std::cout << "Orig WP Pointer=" << origWPIndex << " New WP Pointer=" << newWPIndex << std::endl;


				// If there are any more waypoints in the PrevWPVector, you need to handle them one by one...
				// ... starting with prevWPVector[origWPIndex] in the newWPVector[newWPIndex]
				// We will need to compute the shortest path from the previous to the next
				// Note that the overall distance and ETA will increase as we have added a waypoint.
				for (int i=origWPIndex; i<(int)prevWPVector.size(); i++) // essentially perform this for all remaining vertices...
				{
					// If the last step in the prevWPVector was the Depot run...
					// ... let us not add it as we will be calculating that step anyway.
					if (i==(int)prevWPVector.size()-1)
					{
						if (IsSameVector(&(prevWPVector[i].vLoc), &vDep))
						{
							std::cout << "Skipped Step <" << GetSensorNodeId(prevWPVector[i].vLoc) << ">.\n";
							// Do nothing
							break;
						}
					}

					// This is a non-Depot Node and must be placed on the route based on the last node inserted...
					src = vertex(GetSensorNodeId(newWPVector[newWPIndex-1].vLoc), g);
					dest = vertex(GetSensorNodeId(prevWPVector[i].vLoc), g);
					// Run the Dijkstra's Shortest path algorithm again and populate the distance and predecessor vectors.
					distVector = GetShortestPathsFromSource(g, src, &predVector);
					double newLocDistance = GetBestCostBetweenVertices(g, distVector, src, dest);
					// Compute the ETA
					wp.dDistance = newWPVector[newWPIndex-1].dDistance + newLocDistance;
					wp.vLoc = prevWPVector[i].vLoc;
					wp.dEventTime = prevWPVector[i].dEventTime;
					wp.dETA = ((wp.dEventTime > newWPVector[newWPIndex-1].dETA) ?
							wp.dEventTime :
							newWPVector[newWPIndex-1].dETA
								)
								+ newLocDistance/GetMDCVelocity(graphName); // Again assuming uniform velocity
					newWPVector.push_back(wp);
					newWPIndex++; // Now be at the next insertion point
					std::cout << "Added a Remaining Step <" << GetSensorNodeId(wp.vLoc) << ">.\n";
				}
			} // now you handled all locations after the insertLoc and considered other maypoints after that

			// If the last waypoint is not the Depot, then you want to add that step as the MDCs, when idle are at the depot
			// vDep is the Depot position
			if (IsSameVector(&(newWPVector[newWPIndex-1].vLoc), &vDep))
			{
				std::cout << "Skipped Step <" << GetSensorNodeId(newWPVector[newWPIndex-1].vLoc) << "> was already at the Depot. \n";
				// Do nothing
			}
			else
			{
				uint32_t lst = newWPVector.size()-1;
				src = vertex(GetSensorNodeId(newWPVector[lst].vLoc), g);
				dest = vertex(GetSensorNodeId(vDep), g);
				// Run the Dijkstra's Shortest path algorithm again and populate the distance and predecessor vectors.
				distVector = GetShortestPathsFromSource(g, src, &predVector);
				double newDepotDistance = GetBestCostBetweenVertices(g, distVector, src, dest);
				// Compute the ETA
				wp.dDistance = newWPVector[lst].dDistance + newDepotDistance;
				wp.vLoc = vDep;
				wp.dEventTime = newWPVector[lst].dEventTime;
				wp.dETA = ((wp.dEventTime > newWPVector[lst].dETA) ?
						wp.dEventTime :
						newWPVector[lst].dETA
						)
							+ newDepotDistance/GetMDCVelocity(graphName); // Again assuming uniform velocity
				newWPVector.push_back(wp); // This will add the final depot location
			}
		}
		return (newWPVector);
	}


	std::vector<int> GetShortestPathsFromSource(GraphT g, VertexDescriptor src, std::vector<VertexDescriptor>* predVectorPtr)
	{
		std::vector<int> distVector(num_vertices(g));
		dijkstra_shortest_paths(g, src,
							  predecessor_map(boost::make_iterator_property_map(predVectorPtr->begin(), get(boost::vertex_index, g))).
							  distance_map(boost::make_iterator_property_map(distVector.begin(), get(boost::vertex_index, g))));

		return distVector;

	}

	double GetBestCostBetweenVertices(GraphT g, std::vector<int> distVector, VertexDescriptor src, VertexDescriptor dest)
	{
		//VertexNamePropertyMap vertexNameMap = boost::get(&Vertex_infoT::vertexName, g);
		//std::cout << "Best Cost to " << vertexNameMap[dest]  << " is " << distVector[dest] << std::endl ;
		return distVector[dest];
	}


	std::vector<VertexDescriptor> GetShortestPathBetweenVertices(GraphT g, std::vector<VertexDescriptor> predVector, VertexDescriptor src, VertexDescriptor dest)
	{
		std::vector<VertexDescriptor> segmentPath;
		std::vector<VertexDescriptor>::iterator spi, spEnd;

//		VertexNamePropertyMap vertexNameMap = boost::get(&Vertex_infoT::vertexName, g);

		// You will be inserting items from the bottom here. Start with the dest. Then work your way up to the source.
		segmentPath.insert(segmentPath.begin(), dest);

		VertexDescriptor curr = dest;
		while (curr != src)
		{
			curr = predVector[curr]; // Fetch the next predecessor
			segmentPath.insert(segmentPath.begin(), curr);
		}

		// Print the shortest segment
		std::cout << "  Shortest Path requested = ";
		for ( size_t i = 0; i< segmentPath.size(); )
		{
//			std::cout << vertexNameMap[segmentPath.at(i)] ;
			std::cout << segmentPath.at(i) ;
			if (++i< segmentPath.size())
				std::cout << " --> ";
			else
				std::cout << std::endl;
		}

		return segmentPath;
	}

	void StoreEventList(std::list<SensedEvent> m_events)
	{
		int i = 0;
		for (std::list<SensedEvent>::iterator it = m_events.begin(); it!=m_events.end(); ++it)
		{
			m_allEvents.insert(std::pair<uint32_t, SensedEvent> (i++, *it) );
		}
	}

	// Ideally, you want to do this logic each time you get a new event.
	// However, ns3 does not provide a way to dynamically change the path on a graph like we want.
	// That is why we are computing the entire path upfront and running the simulation
	void ComputeAllGraphWayPoints()
	{
		// Start with the m_allEvents and build the waypoint vectors one by one
		std::map<uint32_t, SensedEvent>::iterator evIt;
		SensedEvent currEvent;
		for (uint32_t i=0; i<m_allEvents.size(); i++)
		{
			currEvent = m_allEvents[i];
			WayPointT newWayPoint;
			newWayPoint.dDistance = 0.0;
			newWayPoint.dETA = 0.0;
			newWayPoint.dEventTime = currEvent.GetTime().GetSeconds();
			newWayPoint.vLoc = currEvent.GetCenter();
			std::cout << "PROCESSING EVENT AT TIME= " << newWayPoint.dEventTime << " LOCATION=" << newWayPoint.vLoc << " NODE ID=[" <<  GetSensorNodeId(newWayPoint.vLoc) << "]" << std::endl;


			// Get the list of graphs that the node would be reachable from
			// There can be nodes that can be on more than one graph
			std::vector<std::string> graphStrVec = NodeIdToGraph(GetSensorNodeId(currEvent.GetCenter()));

			// Repeat for each graph found...
			// Add the newWayPoint at the best possible location in each graph
			// Update the WayPointVector with distance and ETAs.
			for (unsigned j=0; j<graphStrVec.size(); j++)
			{
				std::string graphStr = graphStrVec.at(j);

				// Obtain the graph object to work with
				GraphT g = GetGraph(graphStr);

				// Get the WPVector is one exists already. If not, build one with the MDC at the Depot
				std::vector<WayPointT> currWPVec = GetWaypointVector(graphStr);
				int insertLoc = 0;
				// Let us hold a reference to the best WP vector
				std::vector<WayPointT> bestWPVec = currWPVec; // Any new vector has to be better than existing
				// This will be the new WP vector that will be returned.
				std::vector<WayPointT> newWPVec;
				// Compute the cost of this route and determine if it is the best. If so, keep a record and try the next insertLocation.
				double bestDistance = INFINITY;
				double newBestDistance = 0;


				// Now you need to figure out the best position to insert the newWayPoint into the currWPVec.
				// The insertPosition should be after the WayPoint for which the ETA is less than currEvent's sensedTime
				// If the last Waypoint in the vector has an ETA less than the currEvent time then the Waypoint is simply added at the end of the vector.
				insertLoc = GetInsertLocation(currWPVec, currEvent.GetTime().GetSeconds());

				// Repeat these steps for each valid insertLocation within this graph.
				// A valid insertLocation is anywhere from this point to the end after all others found so far.
				for (;insertLoc < (int)currWPVec.size(); insertLoc++)
				{
					// Next obtain the New WayPoint vector with the waypoint added at a specific insert location
					newWPVec = CreateNewWaypointVector(graphStr, insertLoc, newWayPoint);

					// Compute the cost of this route and determine if it is the best. If so, keep a copy and try the next insertLocation.
					newBestDistance = CompareWaypointVectorDistance(newWPVec, bestDistance);

					std::cout << "--->Printing Candidate WaypointVector... for Graph " << graphStr << std::endl;
					for (int i=0; i<(int)newWPVec.size(); i++)
					{
						std::cout << "  --->EventTime=[" << newWPVec[i].dEventTime
								<< "] Location=[" << newWPVec[i].vLoc << "] <" << GetSensorNodeId(newWPVec[i].vLoc) << ">"
								<< "] Distance=[" << newWPVec[i].dDistance
								<< "] ETA=[" << newWPVec[i].dETA << "]"
								<< std::endl;
						if (newWPVec[i].dDistance > 999999)
						{
							std::cout << "  --->Node [" << newWPVec[i].vLoc << "] seems to be unreachable on Graph " << graphStr << std::endl;
							newBestDistance = INFINITY; // <-- Force it to be ignored
							break; // No point in going further on this path.
						}
					}
					std::cout << " BestDistance Found So far on Graph " << graphStr << " = " << newBestDistance << std::endl;

					if (newBestDistance < bestDistance)
					{
						bestDistance = newBestDistance;
						bestWPVec = newWPVec;
					}

					// Now go and try the next InsertLocation.
				}

				// At this point, you should have tried all possible insertion points of the new node.
				// The bestWPVec should be pointing to the WPVector that has the best possible sequence of traversing this graph.

				// Replace the WPVector for this graph with the best option seen.
				SetWaypointVector(graphStr, bestWPVec);
				std::cout << "***Printing BEST WaypointVector... on Graph " << graphStr << std::endl;
				for (int i=0; i<(int)bestWPVec.size(); i++)
				{
					std::cout << " **EventTime=[" << bestWPVec[i].dEventTime
							<< "] Location=[" << bestWPVec[i].vLoc << "] <" << GetSensorNodeId(bestWPVec[i].vLoc) << ">"
							<< "] Distance=[" << bestWPVec[i].dDistance
							<< "] ETA=[" << bestWPVec[i].dETA << "]"
							<< std::endl;
				}
			}
			// Go back now and add this waypoint for all the graphs where the waypoint is reachable.

		}
		// Go back and add the next event location

	}


	int GetInsertLocation(std::vector<WayPointT> WPVec, double eventTime)
	{
		/****************
		std::cout << "Printing Current WaypointVector... " << std::endl;
		for (int i=0; i<(int)WPVec.size(); i++)
		{
			std::cout << " EventTime=[" << WPVec[i].dEventTime
					<< "] Location=[" << WPVec[i].vLoc << "] <" << GetSensorNodeId(WPVec[i].vLoc) << ">"
					<< "] Distance=[" << WPVec[i].dDistance
					<< "] ETA=[" << WPVec[i].dETA << "]"
					<< std::endl;
		}
		******************/

		// This will be the ideal... to set a course if we knew the occurence of all the events upfront.
//		for (int i=WPVec.size()-1; i>=0; i--)
//		{
//			if (WPVec[i].dETA < eventTime)
//			{
//				std::cout << "Insert Location for {" << eventTime << "} is after Entry: " << i << std::endl;
//				return i;
//			}
//		}

		// This assumes we can change the course only after the current waypoint an MDC has left for has been reached.
		for (int i=0; i<(int)WPVec.size()-1; i++)
		{
			if (WPVec[i].dETA > eventTime)
			{
				std::cout << "Insert Location for {" << eventTime << "} is after Entry: " << i << std::endl;
				return i;
			}
		}


		return 0;
	}

	double CompareWaypointVectorDistance(std::vector<WayPointT> WPVec, double currLowestDistance)
	{
		double newLowestDistance = WPVec[WPVec.size()-1].dDistance;
		std::cout << " Comparing " << newLowestDistance << " with BestDistance " << currLowestDistance << std::endl;
		if (newLowestDistance < currLowestDistance)
			return newLowestDistance;
		else
			return currLowestDistance;
	}


} // namespace ns3
