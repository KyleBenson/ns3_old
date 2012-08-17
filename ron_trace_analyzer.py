#! /usr/bin/python
RON_TRACE_ANALYZER_DESCRIPTION = '''A helper script for analyzing NS3 Resilient Overlay Network simulation traces and visualizing the data.'''
#
# (c) University of California Irvine 2012
# @author: Kyle Benson

import argparse
import numpy
import os.path
import os

##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

def ParseArgs():

    parser = argparse.ArgumentParser(description=RON_TRACE_ANALYZER_DESCRIPTION,
                                     #formatter_class=argparse.RawTextHelpFormatter
                                     epilog='(*1/N): These arguments will be applied to their respective graphs/groups in the order they are specified in.  If only one argument is given, it will be applied to all the groups.')

    # Input Traces
    parser.add_argument('--files', '-f', type=str, nargs='+',
                        help='''files from which to read trace data''')

    parser.add_argument('--dirs', '-d', type=str, nargs='+',
                        help='''directories from which to read trace data files\
    (directories should group together runs from different parameters being studied)''')

    # Labeling
    parser.add_argument('--label', '-l', nargs='+',
                        help='label each respective file or directory with these args instead of the file/directory names')

    parser.add_argument('--append_label', nargs='+',
                        help='(*1/N) Append the given arguments to the group labels given in the legend.')

    parser.add_argument('--prepend_label', nargs='+',
                        help='(*1/N) Prepend the given arguments to the group labels given in the legend.')

    # Titling
    parser.add_argument('--titles', nargs='+',
                        help='manually enter title(s) for the graph(s)')

    parser.add_argument('--append_title', nargs='+',
                        help='(*1/N) Append the given arguments to the title of the respective graphs.')

    parser.add_argument('--prepend_title', nargs='+',
                        help='(*1/N) Prepend the given arguments to the title of the respective graphs.')

    #TODO: what about when we want to specify the x-axis for groups?

    # Graph types to build
    parser.add_argument('--time', action='store_true',
                        help='show graph of total ACKs received over time')

    parser.add_argument('--congestion', action='store_true',
                        help='graph number of packets sent for each group'
                        + 'CURRENTLY NOT IMPLEMENTED*')

    parser.add_argument('--improvement', '-i', action='store_true',
                        help='graph percent improvement over not using an overlay for each file or group')
    #TODO: add something to do with the number of forwards during the event?


    # Graph output modifiers
    parser.add_argument('--average', '-a', action='store_true',
                        help='average together all files or groups instead of comparing them \
    (files within a group are always averaged unless --separate is specified) \
CURRENTLY NOT IMPLEMENTED*')

    parser.add_argument('--separate', action='store_true',
                        help='don\'t average together the runs within a directory: put them each in a separate group. \
CURRENTLY NOT IMPLEMENTED*')

    parser.add_argument('--resolution', type=float,
                        help='Time resolution (in seconds) for time-based graphs.')

    parser.add_argument('--no_save', '-ns', action='store_true',
                        help='Don\'t save the graphs automatically.')

    parser.add_argument('--no_windows', '-nw', action='store_true',
                        help='Don\'t display the graphs after creating them')

    parser.add_argument('--format', default='.svg',
                        help='Image format used to export graphs. Default: %(default)s')

    parser.add_argument('--output_directory', default='ron_output',
                        help='Base directory to output graphics files to. Default: %(default)s')

    # Naming files
    parser.add_argument('--names', nargs='+',
                        help='Explicitly name the respective graph files that will be saved.')

    parser.add_argument('--append_name', nargs='+',
                        help='Append arg to all output graph files saved on this run.')

    parser.add_argument('--prepend_name', nargs='+',
                        help='Prepend arg to all output graph files saved on this run.')

    # Text data
    parser.add_argument('--total_clients', '-t', action='store_true',
                        help='print how many total clients were in simulation')

    parser.add_argument('--conn_clients', '-c', action='store_true',
                        help='print how many clients successfully connected to the server')

    parser.add_argument('--direct_clients', '-o', action='store_true',
                        help='print how many clients reached the server directly (didn\'t use the overlay)')

    return parser.parse_args()


############################################################################################################
############################        Trace Objects        ###################################################
############################################################################################################


class TraceNode:
    '''
    Represents a single node during a simulation.
    Stores important values pertaining to it and can perform some calculations.
    '''
    def __init__(self,id):
        self.id = id
        self.ackRecvd = False
        self.directAck = False
        self.ackTime = 0
        self.sends = 0
        self.forwards = 0

    # TODO: implement getting more than one ACK????

    def __repr__(self):
        return "Node " + self.id + ((" received " + ("direct" if self.directAck else "indirect") + " ACK at " + self.ackTime) if self.ackRecvd else "") + " sent %d and forwarded %d packets" % (self.sends, self.forwards)

    def getTotalSends(self):
        return self.sends + self.forwards


class TraceRun:
    '''
    A single simulation run from one file. Holds the nodes and performs calculations.
    '''
    # Indices of important data in case trace files change
    NODE_ID_INDEX = 1
    ACTION_INDEX = 2
    DIRECT_ACK_INDEX = 3
    TIME_INDEX = 6

    TIME_RESOLUTION = 0.1 #In seconds

    def __init__(self,filename):
        self.nodes = {}
        self.nAcks = None
        self.nDirectAcks = None
        self.name = "Run" if not filename else os.path.split(filename)[1]
        self.forwardTimes = None
        self.ackTimes = None

        with open(filename) as f:
            sendTimes = []
            ackTimes = []
            forwardTimes = []

            for line in f.readlines():
                parsed = line.split()
                nodeId = parsed[TraceRun.NODE_ID_INDEX]
                node = self.nodes.get(nodeId)

                if not node:
                    node = self.nodes[nodeId] = TraceNode(nodeId)

                if 'ACK' in line:
                    node.ackTime = parsed[TraceRun.TIME_INDEX]
                    node.ackRecvd = True
                    ackTimes.append(float(parsed[TraceRun.TIME_INDEX]))

                    if 'direct' == parsed[TraceRun.DIRECT_ACK_INDEX]:
                        node.directAck = True

                if 'forwarded' == parsed[TraceRun.ACTION_INDEX]:
                    node.forward += 1
                    forwardTimes.append(float(parsed[TraceRun.TIME_INDEX]))

                if 'sent' == parsed[TraceRun.ACTION_INDEX]:
                    node.sends += 1
                    sendTimes.append(float(parsed[TraceRun.TIME_INDEX]))
                
        # Finally, properly store the times as counters
        print sendTimes
        # durp can't just append the times they belong in some specific index...
        self.sendTimes = [0]*int(sendTimes[-1] / TraceRun.TIME_RESOLUTION + 1)
        print "BlAG",self.sendTimes
        for i in sendTimes:
            self.sendTimes[int(i/TraceRun.TIME_RESOLUTION)] += 1
        print "BlAG",self.sendTimes

            
        if self.forwardTimes:
            self.forwardTimes = [0]*int(forwardTimes[-1] / TraceRun.TIME_RESOLUTION + 1)
            for i in forwardTimes:
                self.forwardTimes[int(i/TraceRun.TIME_RESOLUTION)] += 1

        if self.ackTimes:
            self.ackTimes = [0]*int(ackTimes[-1] / TraceRun.TIME_RESOLUTION + 1)
            for i in ackTimes:
                self.ackTimes[int(i/TraceRun.TIME_RESOLUTION)] += 1

    def getNNodes(self):
        return len(self.nodes)

    def getNAcks(self):
        if not self.nAcks:
            self.nAcks = sum([1 for i in self.nodes.values() if i.ackRecvd])
        return self.nAcks

    def getNDirectAcks(self):
        if not self.nDirectAcks:
            self.nDirectAcks = sum([1 for i in self.nodes.values() if i.directAck])
        return self.nDirectAcks

    def getSendTimes(self):
        return self.sendTimes

    def getAckTimes(self):
        return self.ackTimes

    def getForwardTimes(self):
        return self.forwardTimes

class TraceGroup:
    '''
    A group of several TraceRuns, which typically were found in a single folder
    and had the same simulation parameters.

    Note that the getNNodes and getNAcks accessor functions will return the average value
    over these 
    '''

    def __init__(self,folder=None):
        self.traces = []
        self.name = "Group" if not folder else os.path.split(folder)[1]
        self.nAcks = None
        self.nNodes = None
        self.nDirectAcks = None

        if folder:
            for f in listdir(folder):
                self.traces.append(TraceRun(f))

    def getNNodes(self):
        if not self.nNodes:
            self.nNodes = sum([t.getNNodes() for t in self.traces])/float(len(self.traces))
        return self.nNodes
        
    def getNAcks(self):
        if not self.nAcks:
            self.nAcks = sum([t.getNAcks() for t in self.traces])/float(len(self.traces))
        return self.nAcks

    def getNDirectAcks(self):
        if not self.nDirectAcks:
            self.nDirectAcks = sum([t.getNDirectAcks() for t in self.traces])/float(len(self.traces))
        return self.nDirectAcks

    def getSendTimes(self):
        if not self.sendTimes:
            self.sendTimes = [i/float(len(self.traces)) for i in sum([numpy.array(t.getSendTimes) for t in self.traces])]
        return self.sendTimes


################################# MAIN ####################################


if __name__ == '__main__':

    args = ParseArgs ()

    if not args.files and not args.dirs:
        print "You must specify some files or directories to parse!"
        exit(1)

    # First, deal with the globally affecting args
    if args.resolution:
        TraceRun.TIME_RESOLUTION = args.TIME_RESOLUTION

    # Build actual traces
    traceGroups = []
    if args.files:
        for f in args.files:
            traceGroups.append(TraceRun(f))

    elif args.dirs:
        for d in args.dirs:
            traceGroups.append(TraceGroup(d))
    
    # Fix label for the groups
    if args.label:   
        if len(args.label) != len(traceGroups):
            print "Number of given label must equal the number of parsed directories!"
            exit(1)

        if len(args.label) > 1:
            for i,g in enumerate(traceGroups):
                g.name = args.label[i]
        else:
            for g in traceGroups:
                g.name = args.label[0]

    if args.prepend_label:   
        if len(args.prepend_label) != len(traceGroups):
            print "Number of given appendix label must equal the number of parsed directories!"
            exit(1)

        if len(args.prepend_label) > 1:
            for i,g in enumerate(traceGroups):
                g.name = g.name + args.prepend_label[i]
        else:
            for g in traceGroups:
                g.name = g.name + args.prepend_label[0]

    if args.append_label:   
        if len(args.append_label) != len(traceGroups):
            print "Number of given appendix label must equal the number of parsed directories!"
            exit(1)

        if len(args.append_label) > 1:
            for i,g in enumerate(traceGroups):
                g.name += args.append_label[i]
        else:
            for g in traceGroups:
                g.name += args.append_label[0]

    # Print any requested textual data
    if args.total_clients:
        print "Total number of clients:"
        for g in traceGroups:
            print g.name, "had", g.getNNodes(), "total nodes\n"

    if args.conn_clients:
        print "Number of clients that reached the server:"
        for g in traceGroups:
            print g.name, "had", g.getNAcks(), "nodes reach the server\n"
    
    if args.direct_clients:
        print "Number of clients that reached the server directly (without using the overlay):"
        for g in traceGroups:
            print g.name, "had", g.getNDirectAcks(), "nodes reach the server directly\n"


#################################################################################################
###################################   PLOTS    ##################################################
#################################################################################################


    import matplotlib.pyplot as plt
    import matplotlib.axes as ax
    from math import ceil,sqrt

    # Build titles for plots
    

    if args.time:
        for g in traceGroups:
            print g.getSendTimes()
            plt.plot(g.getSendTimes(), label=g.name)
        plt.title("ACKs over time")
        plt.xlabel("Time (%ds)"% TraceRun.TIME_RESOLUTION)
        plt.ylabel("Count")
        plt.legend()
        #ax.Axes.autoscale_view() #need instance

        '''if not args.no_save:
            savefig(os.path.join(args.output_directory, )'''

    if args.congestion:
        plt.title("ACKs over time")
        plt.xlabel("Time (%ds)"% resolution)
        plt.ylabel("Count")
        plt.legend()

        if not args.no_save:
            savefig(something)

    if args.improvement:
        plt.title("ACKs over time")
        plt.xlabel("Time (%ds)"% resolution)
        plt.ylabel("Count")
        plt.legend()

        if not args.no_save:
            savefig(something)

    plt.show()


        #subplot(nrows,ncols,next_axes)
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
