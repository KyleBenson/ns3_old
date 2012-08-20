#! /usr/bin/python
RON_TRACE_ANALYZER_DESCRIPTION = '''A helper script for analyzing NS3 Resilient Overlay Network simulation traces and visualizing the data.'''
#
# (c) University of California Irvine 2012
# @author: Kyle Benson

import argparse, numpy, os.path, os, decimal, math, heapq

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
    (directories should group together runs from different parameters being studied). \
    Hidden files (those starting with '.') and subdirectories are ignored (use them to store parameters/observations/graphs).''')

    # Labeling
    parser.add_argument('--label', '-l', nargs='+',
                        help='label each respective file or directory with these args instead of the file/directory names')

    parser.add_argument('--append_label', nargs='+',
                        help='(*1/N) Append the given arguments to the group labels given in the legend.')

    parser.add_argument('--prepend_label', nargs='+',
                        help='(*1/N) Prepend the given arguments to the group labels given in the legend.')

    # Titling
    parser.add_argument('--title', nargs='+',
                        help='manually enter title(s) for the graph(s)')

    parser.add_argument('--append_title', nargs='+',
                        help='(*1/N) Append the given arguments to the title of the respective graphs.')

    parser.add_argument('--prepend_title', nargs='+',
                        help='(*1/N) Prepend the given arguments to the title of the respective graphs.')


    # Graph types to build
    parser.add_argument('--time', action='store_true',
                        help='show graph of total ACKs received over time')

    parser.add_argument('--congestion', action='store_true',
                        help='graph number of packets sent for each group'
                        + 'CURRENTLY NOT IMPLEMENTED*')

    parser.add_argument('--improvement', '-i', action='store_true',
                        help='graph percent improvement over not using an overlay for each file or group')


    # Graph output modifiers
    parser.add_argument('--average', '-a', action='store_true',
                        help='average together all files or groups instead of comparing them \
    (files within a group are always averaged unless --separate is specified) \
CURRENTLY NOT IMPLEMENTED*')

    parser.add_argument('--separate', action='store_true',
                        help='don\'t average together the runs within a directory: put them each in a separate group.')

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



    #TODO: add something to do with the number of forwards during the event?
    #TODO: what about when we want to specify the x-axis for groups?
#TODO: how to find out the total number of overlay nodes and not just ones that participated???



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
        self.acks = 0
        self.directAcks = 0
        self.firstAckTime = 0
        self.sends = 0
        self.forwards = 0

    # TODO: implement getting more than one ACK????

    def __repr__(self):
        return "Node " + self.id + ((" received first" + ("direct" if self.directAcks else "indirect") + " ACK at " + self.ackTime) if self.acks else "") + " sent %d and forwarded %d packets" % (self.sends, self.forwards)

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
        self.name = "Run" if not filename else os.path.split(filename)[1]
        self.nAcks = None
        self.nDirectAcks = None
        self.forwardTimes = {}
        self.ackTimes = {}
        self.sendTimes = {}

        sigDigits = int(round(-math.log(TraceRun.TIME_RESOLUTION,10)))

        with open(filename) as f:

            for line in f.readlines():
                parsed = line.split()
                nodeId = parsed[TraceRun.NODE_ID_INDEX]
                node = self.nodes.get(nodeId)

                if len(parsed) > TraceRun.TIME_INDEX:
                    time = round(float(parsed[TraceRun.TIME_INDEX]), sigDigits)

                if not node:
                    node = self.nodes[nodeId] = TraceNode(nodeId)

                if 'ACK' in line:
                    node.acks += 1
                    self.ackTimes[time] = self.ackTimes.get(time,0) + 1

                    if not node.firstAckTime:
                        node.firstAckTime = time

                    if 'direct' == parsed[TraceRun.DIRECT_ACK_INDEX]:
                        node.directAcks += 1

                if 'forwarded' == parsed[TraceRun.ACTION_INDEX]:
                    node.forwards += 1
                    self.forwardTimes[time] = self.forwardTimes.get(time,0) + 1

                if 'sent' == parsed[TraceRun.ACTION_INDEX]:
                    node.sends += 1
                    self.sendTimes[time] = self.sendTimes.get(time,0) + 1

    def getNNodes(self):
        return len(self.nodes)

    def getNAcks(self):
        if not self.nAcks:
            self.nAcks = sum([i.acks for i in self.nodes.values()])
        return self.nAcks

    def getNSends(self):
        if not self.nSends:
            self.nSends = sum([i.sends for i in self.nodes.values()])
        return self.nSends

    def getNDirectAcks(self):
        if not self.nDirectAcks:
            self.nDirectAcks = sum([i.directAcks for i in self.nodes.values()])
        return self.nDirectAcks

    def getSendTimes(self):
        '''
        Returns a two-tuple of lists where the first list is the time values and the second
        is the number of occurences in that time slice.
        '''
        if isinstance(self.sendTimes, dict):
            items = sorted(self.sendTimes.items())
            self.sendTimes = ([i for i,j in items], [j for i,j in items])
        return self.sendTimes

    def getAckTimes(self):
        '''
        Returns a two-tuple of lists where the first list is the time values and the second
        is the number of occurences in that time slice.
        '''
        if isinstance(self.ackTimes, dict):
            items = sorted(self.ackTimes.items())
            self.ackTimes = ([i for i,j in items], [j for i,j in items])
        return self.ackTimes
        #return (self.ackTimes.keys(), self.ackTimes.values())

    def getForwardTimes(self):
        '''
        Returns a two-tuple of lists where the first list is the time values and the second
        is the number of occurences in that time slice.
        '''
        if isinstance(self.forwardTimes, dict):
            items = sorted(self.forwardTimes.items())
            self.forwardTimes = ([i for i,j in items], [j for i,j in items])
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
        self.sendTimes = None
        self.ackTimes = None
        self.directAckTimes = None

        if folder:
            for f in os.listdir(folder):
                if not f.startswith('.'):
                    self.traces.append(TraceRun(os.path.join(folder,f)))
        else:
            print folder, "is not a directory!"

    def getNNodes(self):
        '''
        Get average number of nodes for all TraceRuns in this group.
        '''
        if not self.nNodes:
            self.nNodes = sum([t.getNNodes() for t in self.traces])/float(len(self.traces))
        return self.nNodes
        
    def getNAcks(self):
        '''
        Get average number of acks for all TraceRuns in this group.
        '''
        if not self.nAcks:
            self.nAcks = sum([t.getNAcks() for t in self.traces])/float(len(self.traces))
        return self.nAcks

    def getNDirectAcks(self):
        '''
        Get average number of direct acks for all TraceRuns in this group.
        '''
        if not self.nDirectAcks:
            self.nDirectAcks = sum([t.getNDirectAcks() for t in self.traces])/float(len(self.traces))
        return self.nDirectAcks

    def averageTimes(self, runs):
        '''
        Takes as input a list of TraceRun.getXTimes() outputs.  Merges the lists together so that the counts
        having the same time value are added, averages all of the counts, and then returns this new 2-tuple
        that matches the format of the TraceRun.getXTimes() funcions.
        '''
        newTimes = []
        newCounts = []
        nRuns = len(runs)
        nextIndex = nRuns*[0]
        pointsLeft = [len(run[0]) for run in runs]

        # Continually add together all counts that correspond to the same timestamp, building up the newTimes
        # and newCounts until all of the runs have been fully accounted for.
        while any(pointsLeft):
            nextTime = min([run[0][nextIndex[irun]] if pointsLeft[irun] else sys.maxint for irun,run in enumerate(runs)])
            newTimes.append(nextTime)
            timeAdded = False

            for irun,run in enumerate(runs):
                if pointsLeft[irun] and run[0][nextIndex[irun]] == nextTime:
                    if timeAdded:
                        newCounts[-1] += run[1][nextIndex[irun]]
                    else:
                        newCounts.append(run[1][nextIndex[irun]])
                        timeAdded = True

                    pointsLeft[irun] -= 1
                    nextIndex[irun] += 1

        # Average the elements
        nRuns = float(nRuns)
        return (newTimes, [count/nRuns for count in newCounts])

    def getSendTimes(self):
        '''
        Get average send times for all TraceRuns in this group. This will likely create a more smoothed curve.
        '''
        if not self.sendTimes:
            self.sendTimes = self.averageTimes([t.getSendTimes() for t in self.traces])
        return self.sendTimes

    def getAckTimes(self):
        '''
        Get average ack times for all TraceRuns in this group. This will likely create a more smoothed curve.
        '''
        if not self.ackTimes:
            self.ackTimes = self.averageTimes([t.getAckTimes() for t in self.traces])
        return self.ackTimes

    def getForwardTimes(self):
        '''
        Get average forward times for all TraceRuns in this group. This will likely create a more smoothed curve.
        '''
        if not self.forwardTimes:
            self.forwardTimes = self.averageTimes([t.getForwardTimes() for t in self.traces])
        return self.forwardTimes

################################# MAIN ####################################


if __name__ == '__main__':

    args = ParseArgs ()

    if not args.files and not args.dirs:
        print "You must specify some files or directories to parse!"
        exit(1)

    # First, deal with the globally affecting args
    if args.resolution:
        TraceRun.TIME_RESOLUTION = args.resolution

    # Build actual traces
    traceGroups = []
    if args.files:
        for f in args.files:
            traceGroups.append(TraceRun(f))

    elif args.dirs:
        for d in args.dirs:
            group = TraceGroup(d)
            if args.separate:
                traceGroups.extend(group.traces)
            else:
                traceGroups.append(group)
    
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
    #import matplotlib.axes as ax

    def adjustXAxis(plot):
        '''
        Adjust the x-axis so that the left side is more visible.
        '''
        xmin, xmax = plot.xlim()
        adjusted = xmin - 0.05 * (xmax - xmin)
        plot.xlim(xmin=adjusted)

    nextTitleIdx = 0

    if args.time:
        for g in traceGroups:
            plt.plot(*g.getAckTimes(), label=g.name)
        try:
            plt.title(" ".join(((args.prepend_title[nextTitleIdx if len(args.prepend_title) > 1 else 0]
                                 if args.prepend_title else ''),
                                (args.title[nextTitleIdx if len(args.title) > 1 else 0]
                                 if args.title else "ACKs over time"),
                                (args.append_title[nextTitleIdx if len(args.append_title) > 1 else 0]
                                 if args.append_title else ''))))
        except IndexError:
            print "You must specify the same number of titles, prepend_titles, and append_titles, or else give "\
                "just 1 to apply to all plots (not all args must be specified, just make sure the ones you use match up)."
            exit(2)
        nextTitleIdx += 1
        plt.xlabel("Time (resolution = %ss)"% str(TraceRun.TIME_RESOLUTION))
        plt.ylabel("Count")
        plt.legend()
        adjustXAxis(plt)
        #ax.Axes.autoscale_view() #need instance

        if not args.no_windows:
            plt.show()

        '''if not args.no_save:
            savefig(os.path.join(args.output_directory, )'''

    if args.congestion:
        for g in traceGroups:
            plt.plot(*g.getSendTimes(), label=g.name)
        try:
            plt.title(" ".join(((args.prepend_title[nextTitleIdx if len(args.prepend_title) > 1 else 0]
                                 if args.prepend_title else ''),
                                (args.title[nextTitleIdx if len(args.title) > 1 else 0]
                                 if args.title else "Connection attempts over time"),
                                (args.append_title[nextTitleIdx if len(args.append_title) > 1 else 0]
                                 if args.append_title else ''))))
        except IndexError:
            print "You must specify the same number of titles, prepend_titles, and append_titles, or else give "\
                "just 1 to apply to all plots (not all args must be specified, just make sure the ones you use match up)."
            exit(2)
        plt.xlabel("Time (resolution = %ss)"% str(TraceRun.TIME_RESOLUTION))
        plt.ylabel("Count")
        plt.legend()
        adjustXAxis(plt)

        if not args.no_windows:
            plt.show()

    #TODO: this
    if args.improvement:
        for g in traceGroups:
            plt.plot(*g.getSendTimes(), label=g.name)
        try:
            plt.title(" ".join(((args.prepend_title[nextTitleIdx if len(args.prepend_title) > 1 else 0]
                                 if args.prepend_title else ''),
                                (args.title[nextTitleIdx if len(args.title) > 1 else 0]
                                 if args.title else "% improvement from overlay usage"),
                                (args.append_title[nextTitleIdx if len(args.append_title) > 1 else 0]
                                 if args.append_title else ''))))
        except IndexError:
            print "You must specify the same number of titles, prepend_titles, and append_titles, or else give "\
                "just 1 to apply to all plots (not all args must be specified, just make sure the ones you use match up)."
            exit(2)
        plt.title("ACKs over time")
        plt.xlabel("Time (resolution = %ss)"% str(TraceRun.TIME_RESOLUTION))
        plt.ylabel("Count")
        plt.legend()
        adjustXAxis(plt)

        if not args.no_windows:
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
