/*
 * InsteadOfFile.h
 *
 *  Created on: Jun 1, 2013
 *      Author: pithya, apoorva, ppr
 */
#include <iostream>
#include <list>

#define NumberOfNodes 14
#define NumberOfEdges 19
#define NumberOfParam 7
#define NumberOfSensors 38
#define Offset 2
#define RadiusOfMDC Offset+3

//parameters for single or multiple mdc
#define NumberOfSingleOutput 24
#define NumberOfMultipleOutput 13
#define NumberOfOutput NumberOfMultipleOutput

static const uint32_t numOfMDC = 1;
static const uint32_t scenario = 1;

static const float simRunTime = 500.0;

//events for simulation
static const uint32_t PERIOD_BETWEEN_EVENTS = 4;
static const uint32_t START_EVENTS = 5;
static const uint32_t END_OF_ROUND = 200;
static const uint32_t NUM_EVENTS = 4;
static const uint32_t EVENT_ID[] = { 9, 8, 33, 26 };
static const uint32_t EVENT_Time[] = { START_EVENTS, START_EVENTS
		+ PERIOD_BETWEEN_EVENTS * NUM_EVENTS + 1, START_EVENTS
		+ 2 * PERIOD_BETWEEN_EVENTS * NUM_EVENTS + 1, START_EVENTS
		+ 3 * PERIOD_BETWEEN_EVENTS * NUM_EVENTS + 1, END_OF_ROUND,
		(uint32_t) simRunTime };

//intersection, sensor and edge positions
static const uint32_t Node_LogicalID[NumberOfNodes] = { 1, 2, 3, 4, 5, 6, 7, 8,
		9, 10, 11, 12, 13, 14 };

static const float Node_XCoor[NumberOfNodes] = { 0, 0, 0, 0, 30, 30, 30, 30, 60,
		60, 90, 90, 90, 90 };
static const float Node_YCoor[NumberOfNodes] = { 0, 30, 60, 90, 0, 30, 60, 90,
		0, 30, 0, 30, 60, 90 };

static const uint32_t Edge_Node1[NumberOfEdges] = { 1, 1, 2, 2, 3, 3, 4, 5, 5,
		6, 6, 7, 8, 9, 9, 10, 11, 12, 13 };
static const uint32_t Edge_Node2[NumberOfEdges] = { 2, 5, 3, 6, 4, 7, 8, 6, 9,
		7, 10, 8, 14, 10, 11, 12, 12, 13, 14 };

static const float OUTPUT_PATH[NumberOfSingleOutput] = { 7, 8, 4, 3, 7, 6, 2, 3, 2, 1,
		5, 6, 10, 9, 5, 9, 11, 12, 10, 12, 13, 14, 8, 7 };

static const float ALT_OUTPUT_PATH[NumberOfSingleOutput] = { 7, 8, 4, 3, 7, 3, 2, 6,
		2, 1, 5, 6, 10, 9, 5, 9, 11, 12, 10, 12, 13, 14, 8, 7 };

static const float MUL_OUTPUT_PATHA[NumberOfMultipleOutput] = { 7, 8, 4, 3, 7, 6, 2, 3,
		2, 1, 5, 6, 7 };

static const float MUL_OUTPUT_PATHB[NumberOfMultipleOutput] = { 6, 10, 9, 5, 9, 11, 12,
		10, 12, 13, 14, 8, 7 };

static const float MUL_OUTPUT_PATHA1[] = { 7, 8, 4, 3, 7, 3, 2, 6,
		2, 1, 5, 6 };

static const float MUL_OUTPUT_PATHB1[NumberOfMultipleOutput] = { 6, 10, 9, 5, 9, 11, 12,
		10, 12, 13, 14, 8, 7 };

static const uint32_t all_tup[NumberOfEdges][NumberOfParam] = { { 7, 1, 1, 1, 1,
		1, 1 }, // row 0
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 1
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 2
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 3
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 4
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 5
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 6
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 7
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 8
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 9
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 10
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 11
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 12
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 13
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 14
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 15
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 16
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 17
		{ 7, 1, 1, 1, 1, 1, 1 }, // row 18
		};

static const int Sensor_LogicalID[NumberOfSensors] = { 0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37 };

static const float Sensor_XCoor[NumberOfSensors] = { 0 + Offset, 0 + Offset, 0
		+ Offset, 0 + Offset, 0 + Offset, 0 + Offset, 30 + Offset, 30 - Offset,
		30 + Offset, 30 - Offset, 30 + Offset, 30 - Offset, 60 + Offset, 60
				- Offset, 90 - Offset, 90 - Offset, 90 - Offset, 90 - Offset, 90
				- Offset, 90 - Offset, 10, 20, 40, 50, 70, 80, 10, 20, 40, 50,
		70, 80, 10, 20, 10, 20, 50, 80 };

static const float Sensor_YCoor[NumberOfSensors] = { 10, 20, 40, 50, 70, 80, 10,
		20, 40, 50, 70, 80, 10, 20, 10, 20, 40, 50, 70, 80, 0 + Offset, 0
				+ Offset, 0 + Offset, 0 + Offset, 0 + Offset, 0 + Offset, 30
				+ Offset, 30 - Offset, 30 + Offset, 30 - Offset, 30 + Offset, 30
				- Offset, 60 + Offset, 60 - Offset, 90 - Offset, 90 - Offset, 90
				- Offset, 90 - Offset };
