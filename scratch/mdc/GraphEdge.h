/*
 * GraphEdge.h
 *
 *  Created on: Jun 1, 2013
 *  Author: pithya, apoorva, ppr
 */

#ifndef GRAPHEDGE_H_
#define GRAPHEDGE_H_

#include "GraphEdgeTuple.h"
#include "GraphSensors.h"

namespace ns3 {

class GraphEdge {
private:
	uint32_t nid1;
	uint32_t nid2;

	float x1, y1;
	float x2, y2;
	list<GraphSensors> closeBySensors;
	bool isVisitable;

public:
	~GraphEdge();
	GraphEdgeTuple *params;
	float getWeight();
	GraphEdge(uint32_t idA, uint32_t idB, float xa, float ya, float xb,
			float yb, GraphEdgeTuple* param);
	uint32_t getFirstNode();
	uint32_t getSecondNode();
	list<GraphSensors> getSensors();
	void setSensors(list<GraphSensors> sensors);
	void addSensor(GraphSensors sensor);
	float getX1();
	void setX1(float x1);
	float getX2();
	void setX2(float x2);
	float getY1();
	void setY1(float y1);
	float getY2();
	void setY2(float y2);

	bool isIsVisitable();
	void setIsVisitable(bool isVisitable);

	int getValidSensorCount();
};
}
#endif /* GRAPHEDGE_H_ */
