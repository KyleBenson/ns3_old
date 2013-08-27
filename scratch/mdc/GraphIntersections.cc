/*
 * GraphIntersections.cc
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */

#include "GraphIntersections.h"

using namespace ns3;

GraphIntersections::GraphIntersections(){
	id = 0;
	x = 0;
	y = 0;
	c.x = 0;
	c.y = 0;
	Actual_id = 0;
}
GraphIntersections::GraphIntersections(uint32_t nid, float x_pos, float y_pos) {
	id = nid;
	x = x_pos;
	y = y_pos;
	c.x = x;
	c.y = y;
	Actual_id = 0;
}
void GraphIntersections::setAdjacency(GraphEdge e) {
	//Update for every edge from edge list, that has first node matching logical id
	adjacency.push_back(e);
}

void GraphIntersections::setActualID(uint32_t nid) {
	Actual_id = nid;
}

uint32_t GraphIntersections::getActualID() {
	return Actual_id;
}


Vector2D GraphIntersections::getPoint() {
	return c;
}
list<GraphEdge> GraphIntersections::getEdges() {
	return adjacency;
}

void GraphIntersections::setNodeID(uint32_t nid) {
	id = nid;
}

uint32_t GraphIntersections::getNodeID() {
	return id;
}

GraphIntersections::~GraphIntersections() {

}
