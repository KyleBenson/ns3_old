/*
 * GraphEdge.cc
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */

#include "GraphEdge.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("GraphEdge");

GraphEdge::GraphEdge(uint32_t idA, uint32_t idB, float xa, float ya, float xb,
		float yb, GraphEdgeTuple* param) {
	params = param;
	nid1 = idA;
	nid2 = idB;
	x1 = xa;
	y1 = ya;
	x2 = xb;
	y2 = yb;
	isVisitable = true;
}

float GraphEdge::getWeight() {

	float width = (float) params->getWidth();
	float length = (float) params->getLength();
	float congestion = (float) params->getCongestion();
	float numSensors = (float) params->getNumSensors();
	float avgSensorPriority = (float) params->getAvgSensorPriority();
	float deadline = (float) params->getDeadline();
	float maxSpeed = (float) params->getMaxSpeed();

	list<GraphSensors> gSen = getSensors();

	float sensorWeight = 0.0;
	for (list<GraphSensors>::iterator itr = gSen.begin(); itr != gSen.end();
			itr++) {
		sensorWeight += itr->getSensorWeight();
	}

	//Some mathematical operation of weighting
	//Assumption: All input is int type
	//Weights : Relative importance
	float num = length * congestion * deadline;
	float denom = width * numSensors * avgSensorPriority * maxSpeed
			* sensorWeight;
	float weight = num / denom;

	return weight;
}

uint32_t GraphEdge::getFirstNode() {
	return nid1;
}

uint32_t GraphEdge::getSecondNode() {
	return nid2;
}

list<GraphSensors> GraphEdge::getSensors() {
	return closeBySensors;
}

int GraphEdge::getValidSensorCount() {
	int count =0;

	NS_LOG_INFO("hey");

	for (list<GraphSensors>::iterator itrG = getSensors().begin();
			itrG != getSensors().end(); itrG++) {
		if (GraphSensors::FAIL == itrG->getSensorCondition())
			count++;
	}
	NS_LOG_INFO("hey");
	return (getSensors().size() - count);
}

void GraphEdge::setSensors(list<GraphSensors> sensors) {
	closeBySensors = sensors;
}

void GraphEdge::addSensor(GraphSensors sensor) {
	closeBySensors.push_back(sensor);
}

float GraphEdge::getX1() {
	return x1;
}

void GraphEdge::setX1(float x1) {
	this->x1 = x1;
}

float GraphEdge::getX2() {
	return x2;
}

void GraphEdge::setX2(float x2) {
	this->x2 = x2;
}

float GraphEdge::getY1() {
	return y1;
}

void GraphEdge::setY1(float y1) {
	this->y1 = y1;
}

float GraphEdge::getY2() {
	return y2;
}

void GraphEdge::setY2(float y2) {
	this->y2 = y2;
}

bool GraphEdge::isIsVisitable() {
	return isVisitable;
}

void GraphEdge::setIsVisitable(bool isVisitable) {
	this->isVisitable = isVisitable;
}

GraphEdge::~GraphEdge() {
}
