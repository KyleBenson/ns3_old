#! /usr/bin/python
RON_TRACE_ANALYZER_DESCRIPTION = '''A helper script for analyzing NS3 Resilient Overlay Network simulation traces and visualizing the data.'''
#
# (c) University of California Irvine 2012
# @author: Kyle Benson

import argparse
from os.path import isdir
from os import listdir

class TraceNode:
    '''
    Represents a single node during a simulation.
    Stores important values pertaining to it and can perform some calculations.
    '''
    def __init__(self,id):
        self.id = id
        self.ackRecvd = False
        self.ackTime = 0
        self.sends = 0
        self.forwards = 0

    def __repr__(self):
        return "Node " + self.id + ((" received ACK at " + self.ackTime) if self.ackRecvd else "") + " sent %d and forwarded %d packets" % (self.sends, self.forwards)

    def getTotalSends(self):
        return self.sends + self.forwards

class TraceRun:
    '''
    A single simulation run from one file. Holds the nodes and performs calculations.
    '''
    # Indices of important data in case trace files change
    TIME_INDEX = 5
    NODE_ID_INDEX = 1

    TIME_RESOLUTION = 0.1 #In seconds

    def __init__(self,filename):
        self.nodes = {}
        self.nAcks = False
        self.name = "Run" if not filename else filename

        with open(filename) as f:
            sendTimes = []

            for line in f.readlines():
                parsed = line.split()
                nodeId = parsed[TraceRun.NODE_ID_INDEX]
                node = self.nodes.get(nodeId)

                if not node:
                    node = self.nodes[nodeId] = TraceNode(nodeId)

                if 'ACK' in line:
                    node.ackTime = parsed[TraceRun.TIME_INDEX]
                    node.ackRecvd = True

                if 'forward' in line:
                    node.forward += 1
                    sendTimes.append(float(parsed[TraceRun.TIME_INDEX]))

                if 'sent packet' in line:
                    node.sends += 1
                    sendTimes.append(float(parsed[TraceRun.TIME_INDEX]))
                
        # Finally, properly store the send times as counters
        self.sendTimes = [0]*int(sendTimes[-1] / TraceRun.TIME_RESOLUTION + 1)
        for i in sendTimes:
            self.sendTimes[int(i/TraceRun.TIME_RESOLUTION)] += 1

    def getNNodes(self):
        return len(self.nodes)

    def getNAcks(self):
        if not self.nAcks:
            self.nAcks = sum([1 for i in self.nodes.values() if i.ackRecvd])
        return self.nAcks

    def getSendTimes(self):
        return self.sendTimes

class TraceGroup:
    '''
    A group of several TraceRuns, which typically were found in a single folder
    and had the same simulation parameters.
    '''

    def __init__(self,folder=None):
        self.traces = []
        self.name = "Group" if not folder else folder

        if folder:
            for f in listdir(folder):
                self.traces.append(TraceRun(f))

    def getNNodes(self):
        return sum([t.getNNodes() for t in self.traces])/len(self.traces)

    def getNAcks(self):
        return sum([t.getNAcks() for t in self.traces])/len(self.traces)

##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
##################################################################################
def PARSE_ARGS():

    parser = argparse.ArgumentParser(description=RON_TRACE_ANALYZER_DESCRIPTION,
                                     #formatter_class=argparse.RawTextHelpFormatter
)

    # Grouping/labeling
    parser.add_argument('--files', metavar='file', type=str, nargs='*',
                        help='''files from which to read trace data''')
    parser.add_argument('--dirs', type=str, nargs='*',
                        help='''directories from which to read trace data files\
    (directories should group together runs from different parameters being studied)''')
    parser.add_argument('--failure_probs',action='store',type=float,nargs='*',
                        help='(label) associated probabilities of failures for each respective file or directory')
    parser.add_argument('--title', type=str, nargs='*',
                        help='manually enter title(s) for the graph(s)')

    # Graphs
    parser.add_argument('--time',action='store_true',
                        help='show graph of total ACKs received over time')
    parser.add_argument('--congestion',action='store_true',
                        help='graph number of packets sent for each group')
    parser.add_argument('--improvement',action='store_true',
                        help='graph percent improvement over not using an overlay for each file or group')
    parser.add_argument('--average',action='store_true',
                        help='average together all files or groups instead of comparing them \
    (files within a group are always averaged unless --separate is specified)')

    # Text data
    parser.add_argument('--total_clients',action='store_true',
                        help='print how many total clients were in simulation')
    parser.add_argument('--conn_clients',action='store_true',
                        help='print how many clients successfully connected to the server')

    return parser.parse_args()

################################# MAIN ####################################

if __name__ == '__main__':

    args = PARSE_ARGS ()

    traceGroups = []
    if args.files:
        group = TraceGroup()

        for f in args.files:
            group.traces.append(TraceRun(f))

        traceGroups.append(group)

    elif args.dirs:
        for d in args.dirs:
            traceGroups.append(TraceGroup(d))

    if args.total_clients:
        print "Total number of clients:"
        for g in traceGroups:
            print g.name, "had", g.getNNodes(), "total nodes"
        print ""

    if args.conn_clients:
        print "Number of clients that reached the server:"
        for g in traceGroups:
            print g.name, "had", g.getNAcks(), "nodes reach the server"
        print ""

    #################################################################################################
    ###################################   PLOTS    ##################################################
    #################################################################################################

    from matplotlib.pyplot import hist,subplot,subplots_adjust
    import matplotlib.pyplot as plt
    from math import ceil,sqrt

    next_axes = 1
    if args.time:
        resolution = args.time
        subplot(nrows,ncols,next_axes)
        times = gdata.get_times(picks,resolution)
        hist(times,resolution)
        plt.title("Time histogram in resolution of %dms." % resolution)
        plt.xlabel("Time offset from first event.")
        plt.ylabel("Count")
        plt.plot()
        next_axes += 1


    plt.show()



    #try for smart subplot arrangements
    '''
    if nplots > 3:
        nrows = int(sqrt(nplots)+0.5)
        ncols = ceil(nplots/float(nrows))
    else:
        nrows = 1
        ncols = nplots

        if 'x' in arg:
            xvalues = gdata.get_xvalues(picks)
            subplot(nrows,ncols,next_axes)
            hist(xvalues,resolution)
            plt.title("Histogram of x-axis PGA readings")
            plt.xlabel("PGA (m/s^2)")
            plt.ylabel("Count")
            plt.plot()
            next_axes += 1

            hist([xvalues,yvalues,zvalues],resolution)
    '''
