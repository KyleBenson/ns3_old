/*
 * GraphEdgeTuple.cc
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */

#include "GraphEdgeTuple.h"

using namespace ns3;

void GraphEdgeTuple::setWidth(uint32_t w) {
	width = w;
}
void GraphEdgeTuple::setLength(uint32_t l) {
	length = l;
}
void GraphEdgeTuple::setCongestion(uint32_t c) {
	congestion = c;
}
void GraphEdgeTuple::setNumSensors(uint32_t nS) {
	numSensors = nS;
}
void GraphEdgeTuple::setAvgSensorPriority(uint32_t asp) {
	avgSensorPriority = asp;
}
void GraphEdgeTuple::setDeadline(uint32_t d) {
	deadline = d;
}
void GraphEdgeTuple::setMaxSpeed(uint32_t ms) {
	maxSpeed = ms;
}
//Getters
uint32_t GraphEdgeTuple::getNumTuples() {
	return num_tuples;
}
uint32_t GraphEdgeTuple::getWidth() {
	return width;
}

uint32_t GraphEdgeTuple::getLength() {
	return length;
}
uint32_t GraphEdgeTuple::getCongestion() {
	return congestion;
}
uint32_t GraphEdgeTuple::getNumSensors() {
	return numSensors;
}
uint32_t GraphEdgeTuple::getAvgSensorPriority() {
	return avgSensorPriority;
}
uint32_t GraphEdgeTuple::getDeadline() {
	return deadline;
}
uint32_t GraphEdgeTuple::getMaxSpeed() {
	return maxSpeed;
}

GraphEdgeTuple::GraphEdgeTuple() {
	num_tuples = 7;
	width = 0;
	length = 0;
	congestion = 0;
	numSensors = 0;
	avgSensorPriority = 0;
	deadline = 0;
	maxSpeed = 0;

}

GraphEdgeTuple::GraphEdgeTuple(uint32_t w, uint32_t l, uint32_t c, uint32_t n,
		uint32_t a, uint32_t d, uint32_t m) {
	num_tuples = 7;
	width = w;
	length = l;
	congestion = c;
	numSensors = n;
	avgSensorPriority = a;
	deadline = d;
	maxSpeed = m;
}
