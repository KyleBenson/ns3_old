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
		else
			return false;

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
		if (fabs(v.x) < EPSILON) w.x = 0.0;
		if (fabs(v.y) < EPSILON) w.y = 0.0;
		if (fabs(v.z) < EPSILON) w.z = 0.0;
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
			sinkTime << Simulator::Now ().GetSeconds ();
		else if (sourceInd == 1) // print collectTime
			collectTime << Simulator::Now ().GetSeconds ();
		else if (sourceInd == 2) // print sensedTime
			sensedTime << Simulator::Now ().GetSeconds ();

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



} // namespace ns3 
