#! /usr/bin/python
NEW_SCRIPT_DESCRIPTION = '''## Read an input file (specified on the command line) and a directory full of .cch rocketfuel files (also specified via command line), parsing each file for the requested cities, looking up the associated latitude and longitude of the city, outputting just the city names and lat/lons to another file.'''

# @author: Kyle Benson
# (c) Kyle Benson 2012

import argparse
#from os.path import isdir
#from os import listdir
#from getpass import getpass
#password = getpass('Enter password: ')

def ParseArgs():
##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

    parser = argparse.ArgumentParser(description=NEW_SCRIPT_DESCRIPTION,
                                     #formatter_class=argparse.RawTextHelpFormatter,
                                     #epilog='Text to display at the end of the help print',
                                     )

    parser.add_argument('--in', type=str, nargs='1',
                        help='''file from which to read city location data''')
    parser.add_argument('--out', type=str, nargs='1',
                        help='''file to which we output the requested cities' location data''')
    parser.add_argument('--cch_files', type=str, nargs='+',
                        help='''files from which to read city names that we're requesting location data about''')

    return parser.parse_args()


def ParseCchCityName(line):
    return line.split()[1].replace('@','').replace('+',' ')

def ReadCch(filename, cities):
    with open(filename) as f:
        for line in f.readlines():
            city_name = ParseCchCityName(line)

def ReadRequestedCities(cch_files):
    cities = {}
    for f in cch_files:
        ReadCch(f, cities)

def Main(args):
    cities = ReadRequestedCities(args.cch_files)
    with open(args.in) as infile:
        

# Main
if __name__ == "__main__":
    
    import sys

    args = ParseArgs(sys.argv)

    Main(args)

    
