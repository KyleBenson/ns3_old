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

#ifndef MDC_UTILITIES_H
#define MDC_UTILITIES_H

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"
#include "ns3/output-stream-wrapper.h"
//#include "mdc-helper.h"

#include <math.h>
#include <set>
#include <queue>


#define EPSILON 0.0001

namespace ns3 {

	static Ptr<OutputStreamWrapper> m_mdcoutputStream; // output stream for tracing from MDC simulation
	Vector GetClosestVector (std::vector<Vector> posVector, Vector refPoint);
	bool IsSameVector (Vector *aV, Vector *bV);
	std::queue<unsigned> NearestNeighborOrder (std::vector<Vector> *inputVector, Vector refPoint);
	std::vector<Vector> ReSortInputVector (std::vector<Vector> *inputVector, std::queue<unsigned> sortSeq);

	void SetMDCOutputStream (Ptr<OutputStreamWrapper> outputStream);
	Ptr<OutputStreamWrapper> GetMDCOutputStream (void);
	std::stringstream CreateTSPInput(std::vector<Vector> *inputVector);
	void WriteTSPInputToFile(std::stringstream &s, const char *TSPFileName);
	int ExecuteSystemCommand(const char *TSPfileName);
	std::queue<unsigned> ReadTSPOutput(const char *TSPfileName);

} // namespace ns3

#endif /* MDC_UTILITIES_H */
