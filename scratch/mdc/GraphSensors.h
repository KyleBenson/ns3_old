/*
 * GraphSensors.h
 *
 *  Created on: Jun 4, 2013
 *      Author: pithya
 */

#ifndef GRAPHSENSORS_H_
#define GRAPHSENSORS_H_

using namespace std;
//#include "GraphEdge.h"
#include "ns3/core-module.h"
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

class GraphSensors {

public:

	enum SensorEnergyLevel {
		LOW = 1, MEDIUM = 2, HIGH = 3
	};
	enum SensorMode {
		FAIL, SAFE, WARNING, DANGER
	};
	//Safe is low DR, Danger means
	enum SensorState {
		inWAIT, inVISIT, MISSED
	};
	//State is useful for the Sensor application

	//setters
	void setBufferSize(uint32_t bufferSize);
	//void setClosestEdges(Ptr <list<GraphEdge>> closestEdges);
	SensorEnergyLevel getEnergy() const;
	float getEstNextVisitTime() const;
	bool isAlive() const;
	void setIsAlive(bool isAlive);
	void setNearestSensor(int sensorD);
	//void setSensor(Ptr<Node> sensor);
	const string& getSensorData() const;
	void setSensorData(Ptr<string> sensorData);
	void setSensorDataRate(float sensorDataRate);
	bool shouldVisit() const;
	void setShouldVisit(bool shouldVisit);
	void setSid(uint32_t sid);
	void setSensorCondition(SensorMode condition);
	void setSensorState(SensorState state);

	uint32_t getSid();
	uint32_t getSensorWeight();

	//Node getSensor();
	void setLocation(Vector3D loc);
	int getNearestSensor();
	//list<GraphEdge> getClosestEdges();
	Vector3D getLocation();

	SensorMode getSensorCondition();

	GraphSensors(double x_pos, double y_pos, uint32_t sensorID,
			Ptr<Node> sensorNode);
	GraphSensors(double x_pos, double y_pos, uint32_t sensorID);
	//virtual ~GraphSensors();

private:
	uint32_t sid; //ns3 node id value
	//Node sensor; //ns3 node object
	int nearestSensorID;
	SensorEnergyLevel energy; //enum for current sensor energy level
	bool alive; //if sensor is currently alive or not(logically)
	bool visit; //if sensor needs to be visited or not
	//list<GraphEdge> closestEdges; //matching edges close to sensors
	Vector3D location;
	float estNextVisitTime;
	SensorMode sensorMode;
	float sensorDataRate;
	uint32_t bufferSize;
	string sensorData;
	SensorState sensorState;
};
}
#endif /* GRAPHSENSORS_H_ */
