/*
 * GraphIntersections.h
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */

#ifndef GRAPHINTERSECTIONS_H_
#define GRAPHINTERSECTIONS_H_

using namespace std;

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/core-module.h"
#include "GraphEdge.h"
#include "GraphEdgeTuple.h"

#include <string>
#include <iostream>
#include <fstream>

namespace ns3 {
	class GraphIntersections {
		float x;
		float y;
		uint32_t id;
		uint32_t Actual_id;
		Vector2D c;
		list<GraphEdge> adjacency;

	public:
		GraphIntersections(uint32_t nid, float x_pos, float y_pos);

		void setNodeID(uint32_t nid);
		uint32_t getNodeID();

		Vector2D getPoint();
		list<GraphEdge> getEdges();

		uint32_t getActualID();
		void setActualID(uint32_t nid);

		void setAdjacency(GraphEdge e);
		virtual ~GraphIntersections();
		GraphIntersections();
	};
}
#endif /* GRAPHINTERSECTIONS_H_ */
