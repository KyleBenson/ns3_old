/*
INSTRUCTIONS TO RUN THIS...

1. Create the schema that holds the MDC Trace Stats collected.
Syntax:
   mysql --local_infile=1 -u ranga -p
   Enter password: 
	mysql> source mdcTraceSchema.sql

2. Load the Trace Stats Captured...
We are assuming that the Trace file is 

	in the folder: /u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt
                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		Make sure this is in the right folder... If not change the script

Syntax:
   mysql --local_infile=1 -u ranga -p
   Enter password: 
	mysql> source mdcLoadCSVToMySql.sql

This script will set RUN_ID=nnnn to a value that distinguishes a simulation run
In order to do this, we are setting a RUN_ID that gets auto incremented here.
This will separate the data captured for a run from another one.

3. Obtaining / Viewing Packet Delay Stats...
Syntax:
	mysql> select * from RUN_RESULTS;

	If you are looking for a specific Run...
	mysql> select * from RUN_RESULTS where RUN_ID = <Run Number>

4. Clean up any data in the MDC Trace tables...
Syntax:
   mysql --local_infile=1 -u ranga -p
   Enter password: 
	mysql> source mdcTruncateTables.sql

------------------------------------------Load CSV Script -----------
*/

SHOW DATABASES;
USE pc3


SELECT MAX(RUN_ID)+1 INTO @RUN_ID
FROM CREATED_EVENTS ;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE CREATED_EVENTS 
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'EVENT_CREATED,' 
( EVENT_ID, SCHED_TIME, LOC_X, LOC_Y, LOC_Z, RADIUS ) 
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE DETECTED_EVENTS
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'EVENT_DETECTION,' 
(
NODE_ID,
DETECT_TIME,
LOC_X,
LOC_Y,
LOC_Z
)
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE SENSOR_TRACE
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'SENSORTRACE,' 
(
NODE_ID,
MESSAGE_TYPE,
MESSAGE_SIZE,
EVENT_TIME
 ) 
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE SINK_TRACE
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'SINK__TRACE,' 
(
NODE_ID,
PACKET_TYPE,
MESSAGE_TYPE,
PACKET_SIZE,
EXPECTED_SIZE,
FROM_IP,
VIA_IP,
TO_IP,
EVENT_TIME
)
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE COLL_TRACE
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'COLL__TRACE,' 
(
NODE_ID,
PACKET_TYPE,
MESSAGE_TYPE,
PACKET_SIZE,
EXPECTED_SIZE,
FROM_IP,
TO_IP,
EVENT_TIME
)
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE PACKET_DROP
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'PACKET_DROP,' 
(
PACKET_TYPE,
EVENT_TIME
)
SET RUN_ID=@RUN_ID;

LOAD DATA LOCAL INFILE '/u01/ns3/workspace/ns-3-dev-socis2013/mdcTraceFile.txt'  
INTO TABLE EVENT_DELAY
FIELDS TERMINATED BY ',' 
LINES STARTING BY 'EVENT_DELAY,' 
(
EVENT_ID,
SCHED_TIME,
SENSED_TIME,
COLLECT_TIME,
SINK_TIME,
SUMMARY_IND 
)
SET RUN_ID=@RUN_ID;

