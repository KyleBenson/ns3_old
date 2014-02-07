#! /usr/bin/python
VISUALIZE_DATA_DESCRIPTION = '''A helper script for analyzing PC3 Shanty Fire Scenario and Mobile Data Collector simulation traces and visualizing the data.'''
#
# (c) University of California Irvine 2014
# @author: Kyle Benson

import argparse, os.path, os, sys, scipy.stats, getpass

##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

def parse_args():

    parser = argparse.ArgumentParser(description=VISUALIZE_DATA_DESCRIPTION,
                                     #formatter_class=argparse.RawTextHelpFormatter
                                     epilog='(*1/N): These arguments will be applied to their respective graphs/groups in the order they are specified in.  If only one argument is given, it will be applied to all the groups.')

    # Input Traces
    parser.add_argument('--files', '-f', type=str, nargs='+',
                        help='''UNSUPPORTED trace files to read into the database before analyzing data''')

    parser.add_argument('--dirs', '-d', type=str, nargs='+',
                        help='''UNSUPPORTED directories containing trace files to read into the database before analyzing data''')

    # Labeling groups
    parser.add_argument('--label', '-l', nargs='+',
                        help='label each respective group with these args instead of a number')

    parser.add_argument('--append-label', nargs='+', dest='append_label',
                        help='(*1/N) Append the given arguments to the group labels given in the legend.')

    parser.add_argument('--prepend-label', nargs='+', dest='prepend_label',
                        help='(*1/N) Prepend the given arguments to the group labels given in the legend.')

    # Titling graphics
    parser.add_argument('--title', nargs='+',
                        help='manually enter title(s) for the graph(s)')

    parser.add_argument('--append-title', nargs='+', dest='append_title',
                        help='(*1/N) Append the given arguments to the title of the respective graphs.')

    parser.add_argument('--prepend-title', nargs='+', dest='prepend_title',
                        help='(*1/N) Prepend the given arguments to the title of the respective graphs.')


    # Graph types to build
    parser.add_argument('--time', '-t', action='store_true',
                        help='JUST X VS Y RIGHT NOW show graph of complete sensor data received over time')

    parser.add_argument('--congestion', '-c', action='store_true',
                        help='UNSUPPORTED graph number of packets sent over time for each group')


    # Graph parameters
    parser.add_argument('--resolution', type=float, default=0.1,
                        help='Time resolution (in seconds) for time-based graphs. ')


    # Graph output modifiers
    parser.add_argument('--save', action='store_true',
                        help='UNSUPPORTED Save the graphs automatically.')

    parser.add_argument('--no-windows', '-nw', action='store_true', dest='no_windows',
                        help='Don\'t display the graphs after creating them')

    parser.add_argument('--format', default='.svg',
                        help='Image format used to export graphs. Default: %(default)s')

    parser.add_argument('--output-directory', dest='output_directory', default='graphics',
                        help='Base directory to output graphics files to. Default: %(default)s')

    # Naming files
    parser.add_argument('--names', nargs='+',
                        help='Explicitly name the respective graph files that will be saved.')

    parser.add_argument('--append-name', dest='append_name', nargs='+',
                        help='Append arg to all output graph files saved on this run.')

    parser.add_argument('--prepend-name', dest='prepend_name', nargs='+',
                        help='Prepend arg to all output graph files saved on this run.')

    # Text data
    parser.add_argument('--summary', '-s', action='store_true',
                        help='UNSUPPORTED Print statistics summary about each file/group.  Includes total nodes, # ACKS, # direct ACKs')
    ## --utility will add those (expected/mean) values to summary
    parser.add_argument('--t-test', action='store_true',
                        help='UNSUPPORTED Compute 2-sample t-test for every pair of groups, taken two at a time in the order specified.')


    # Database arguments
    parser.add_argument('--db-hostname', type=str, dest='db_hostname', default='localhost',
                        help='Hostname to use for the database connection. Default: %(default)s')
    parser.add_argument('--db-user', '--username', dest='db_username', type=str, default=getpass.getuser(),
                        help='User name to use for the database connection. Default: %(default)s')
    parser.add_argument('--db-password', type=str, dest='db_password', default=None,
                        help='UNSUPPORTED Password to use for the database connection. Default: securely read from command line')
    parser.add_argument('--db', '--database', '-D', dest='db_name', type=str, default='Pc3', #TODO: diff name?
                        help='Database name to use for the database connection. Default: %(default)s')

    ##############################  parse args and return ####################

    parsed_args = parser.parse_args()

    # post-processing of arguments
    if parsed_args.db_password is None:
        pass
        #parsed_args.db_password = getpass.getpass("Enter database password for user %s:" % parsed_args.db_username)

    return parsed_args

############################## Helper Functions ##########################

# nothing here yet


################################# MAIN ####################################


if __name__ == '__main__':

    args = parse_args()

    # First, deal with the globally affecting args
    # TODO
    if args.resolution:
        pass

    # Consume traces first if requested
    if args.files:
        for f in args.files:
            pass
    if args.dirs:
        for d in args.dirs:
            pass

    #TODO:
    ngroups = 1

    # Fix label for the groups
    #TODO: inside an object on-demand
    if args.label:   
        if (len(args.label) != ngroups) and (len(args.label) != 1):
            print "Number of given labels must equal the number of parsed directories or 1!"
            exit(1)

        if len(args.label) > 1:
            for i,g in enumerate(traceGroups):
                g.name = args.label[i]
        else:
            for g in traceGroups:
                g.name = args.label[0]

    if args.prepend_label:   
        if (len(args.prepend_label) != ngroups) and (len(args.prepend_label) != 1):
            print "Number of given appendix labels must equal the number of parsed directories or 1!"
            exit(1)

        if len(args.prepend_label) > 1:
            for i,g in enumerate(traceGroups):
                g.name = g.name + args.prepend_label[i]
        else:
            for g in traceGroups:
                g.name = g.name + args.prepend_label[0]

    if args.append_label:   
        if (len(args.append_label) != ngroups) and (len(args.append_label) != 1):
            print "Number of given appendix labels must equal the number of parsed directories or 1!"
            exit(1)

        if len(args.append_label) > 1:
            for i,g in enumerate(traceGroups):
                g.name += args.append_label[i]
        else:
            for g in traceGroups:
                g.name += args.append_label[0]


    ################################################################################
    #####################  Print any requested textual data analysis  ##############
    ################################################################################

    #TODO: these

    if args.summary:
        print "\n================================================= Summary =================================================\n"
        headings = ('Group name\t\t', 'Active Nodes', '# ACKs\t', '# Direct ACKs', '% Improvement', 'Utility\t', 'Stdev')
        print '%s\t'*len(headings) % headings, '\n'

        for g in traceGroups:
            nAcks = g.getNAcks()
            nDirectAcks = g.getNDirectAcks()
            improvement = percentImprovement(nAcks, nDirectAcks)
            utility = g.getUtility()
            try:
                stdev = g.getStdevNAcks()
            except AttributeError:
                stdev = 0.0
            print "\t\t".join(["%-20s", '%.2f', '%.2f', '%.2f', '%.2f', '%.2f', '%.2f']) % (g.name, g.getNNodes(), nAcks, nDirectAcks,
                                                                                    improvement, utility, stdev)
        print '\n==========================================================================================================='

    if args.t_test:
        print "\n================================================= T-Test ==================================================\n"
        print '%s\t'*6 % ('Group 1 name\t\t', 'Group 2 name\t\t', 'Group 1 mean', 'Group 2 mean', 't-statistic', 'p-value'), '\n'
        for i in range(0, ngroups, 2):
            if i+1 >= ngroups:
                print 'Not testing %s as uneven number of groups provided.' % traceGroups[i].name
                break

            g1 = traceGroups[i]
            g2 = traceGroups[i+1]
            
            if args.utility:
                print 'Using utility instead of # ACKs'
                g1_utilities = [r.getUtility() for r in g1.traces]
                g2_utilities = [r.getUtility() for r in g2.traces]
                
                (t_statistic, p_value) = scipy.stats.ttest_ind(g1_utilities, g2_utilities)
                print "\t\t".join(["%-20s", "%-20s", '%.2f', '%.2f', '%.2f', '%.2f']) % (g1.name, g2.name, g1.getUtility(), g2.getUtility(), t_statistic, p_value)

            else:
                g1_acks = [t.getNAcks() - t.getNDirectAcks() for t in g1.traces]
                g2_acks = [t.getNAcks() - t.getNDirectAcks() for t in g2.traces]

                (t_statistic, p_value) = scipy.stats.ttest_ind(g1_acks, g2_acks)
                print "\t\t".join(["%-20s", "%-20s", '%.2f', '%.2f', '%.2f', '%.2f']) % (g1.name, g2.name, g1.getNAcks() - g1.getNDirectAcks(), g2.getNAcks() - g2.getNDirectAcks(), t_statistic, p_value)

        print '\n==========================================================================================================='


#################################################################################################
###################################   PLOTS    ##################################################
#################################################################################################


    import matplotlib.pyplot as plt
    #import matplotlib.axes as ax

    def adjust_x_axis(plot):
        '''
        Adjust the x-axis so that the left side is more visible.
        '''
        xmin, xmax = plot.xlim()
        adjusted = xmin - 0.05 * (xmax - xmin)
        plot.xlim(xmin=adjusted)

    nextTitleIdx = 0
    markers = 'x.*+do^s1_|'

    if args.time:
        '''Just a test plot for now'''

        # get the data from MySQL
        import MySQLdb

        #db = MySQLdb.connect(host=args.db_hostname, user=args.db_user, passwd=args.db_password, db=args.db_name)
        db = MySQLdb.connect(host=args.db_hostname, user=args.db_username, db=args.db_name)
        cursor = db.cursor()

        query = 'select * from Data'
        cursor.execute(query)
        result = cursor.fetchall()

        x = []
        y = []

        for record in result:
            x.append(record[0])
            y.append(record[1])

        plt.plot(x, y, label='data', marker=markers[nextTitleIdx%len(markers)])

        try:
            # build the title of the graph, using explicit command arguments when given
            plt.title(" ".join(((args.prepend_title[nextTitleIdx if len(args.prepend_title) > 1 else 0]
                                 if args.prepend_title else ''),
                                (args.title[nextTitleIdx if len(args.title) > 1 else 0]
                                 if args.title else "Cumulative ACKs over time"),
                                (args.append_title[nextTitleIdx if len(args.append_title) > 1 else 0]
                                 if args.append_title else ''))))
        except IndexError:
            print "You must specify the same number of titles, prepend_titles, and append_titles, or else give "\
                "just 1 to apply to all plots (not all args must be specified, just make sure the ones you use match up)."
            exit(2)

        nextTitleIdx += 1
        plt.xlabel("X")
        plt.ylabel("Y")
        plt.legend(loc=4) #set legend location to bottom right
        adjust_x_axis(plt)
        #ax.Axes.autoscale_view() #need instance

        if not args.no_windows:
            plt.show()

'''        if args.save:
            savefig(os.path.join(args.output_directory, )'''
