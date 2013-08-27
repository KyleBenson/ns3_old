/*
 * GraphSensors.cpp
 *
 *  Created on: Jun 4, 2013
 *      Author: pithya
 */

#include "GraphSensors.h"
using namespace ns3;

GraphSensors::GraphSensors(double x_pos, double y_pos, uint32_t sensorID) {
	Vector3D v;
	v.x = x_pos;
	v.y = y_pos;
	v.z = 0.0;
	setLocation(v);
	sid = sensorID;
}

void GraphSensors::setBufferSize(uint32_t bufferSize) {
	this->bufferSize = bufferSize;
}

//void GraphSensors::setClosestEdges(Ptr<list<GraphEdge>> closestEdges) {
//	this->closestEdges = closestEdges;
//}

bool GraphSensors::isAlive() const {
	return alive;
}

void GraphSensors::setIsAlive(bool alive) {
	this->alive = alive;
}

void GraphSensors::setSensorData(Ptr<string> sensorData) {
	//this->sensorData. = sensorData;
}

void GraphSensors::setSensorDataRate(float sensorDataRate) {
	this->sensorDataRate = sensorDataRate;
}

bool GraphSensors::shouldVisit() const {
	return this->visit;
}

void GraphSensors::setShouldVisit(bool visit) {
	this->visit = visit;
}

void GraphSensors::setSid(uint32_t sid) {
	this->sid = sid;
}

GraphSensors::SensorMode GraphSensors::getSensorCondition(){
	return sensorMode;
}

void GraphSensors::setSensorCondition(SensorMode condition) {
	sensorMode = condition;
}
void GraphSensors::setSensorState(SensorState state) {
	sensorState = state;
}

//Getters
uint32_t GraphSensors::getSensorWeight() {
	return (uint32_t) (sensorMode * energy * sensorDataRate);
	//higher the weight more it contributes to the importance (lower edge-weight)
}

uint32_t GraphSensors::getSid(){
	return sid;
}
GraphSensors::SensorEnergyLevel GraphSensors::getEnergy() const {
	return energy;
}

float GraphSensors::getEstNextVisitTime() const {
	return estNextVisitTime;
}
const string& GraphSensors::getSensorData() const {
	return sensorData;
}

void GraphSensors::setLocation(Vector3D loc) {
	location = loc;
}

void GraphSensors::setNearestSensor(int sensorID){
	nearestSensorID = sensorID;
}

int GraphSensors::getNearestSensor() {
	return nearestSensorID;
}
//list<GraphEdge> GraphSensors::getClosestEdges(){
// 	 return closestEdges;
//}
Vector3D GraphSensors::getLocation() {
	return location;
}
