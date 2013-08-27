/*
 * GraphEdgeTuple.h
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */

#ifndef GRAPHEDGETUPLE_H_
#define GRAPHEDGETUPLE_H_

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/core-module.h"

#include <string>
#include <iostream>
#include <fstream>

namespace ns3 {

class GraphEdgeTuple {

public:
	GraphEdgeTuple();
	GraphEdgeTuple(uint32_t w, uint32_t l, uint32_t c, uint32_t n, uint32_t a,
			uint32_t d, uint32_t m);

	void setWidth(uint32_t w);
	void setLength(uint32_t l);
	void setCongestion(uint32_t c);
	void setNumSensors(uint32_t nS);
	void setAvgSensorPriority(uint32_t asp);
	void setDeadline(uint32_t d);
	void setMaxSpeed(uint32_t ms);
	//Getters
	uint32_t getNumTuples();
	uint32_t getWidth();
	uint32_t getLength();
	uint32_t getCongestion();
	uint32_t getNumSensors();
	uint32_t getAvgSensorPriority();
	uint32_t getDeadline();
	uint32_t getMaxSpeed();

private:
	uint32_t num_tuples;
	uint32_t width;
	uint32_t length;
	uint32_t congestion;
	uint32_t numSensors;
	uint32_t avgSensorPriority;
	uint32_t deadline;
	uint32_t maxSpeed;

};
}
#endif /* GRAPHEDGETUPLE_H_ */
