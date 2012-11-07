#!/usr/bin/python

# Generate Glider CSV - Connects to the GDAM Mongo Database on OOMA, queries for a given mission data set, produces a CSV data given predefined fields, and outputs the data to a file
#
# By: Michael Lindemuth
# University of South Florida
# College of Marine Science
# Center for Ocean Technology

import argparse
import pymongo
import csv

parser = argparse.ArgumentParser(description="Connects to the GDAM Mongo Database on OOMA, queries for a given mission data set, produces a CSV data given predefined fields, and outputs the data to a file.  Users may specify one of the following tuples of arguments: (mission), (mission,start), and (mission,start,end) to limit data output by the script.")
parser.add_argument('--hoursAgo',type=float,default=-1,help='Get data from x hours ago.  Can be decimal number');
parser.add_argument('--start',type=float,default=-1,help='Beginning of Time Range in Query')
parser.add_argument('--end',type=float,default=-1,help='End of Time Range in Query')
parser.add_argument('mission',type=int,help='Mission Number')
parser.add_argument('outputFile',metavar='OutCSVPath',help='CSV Output Path')

args = parser.parse_args()

# Setup query constants
pairPriorities = ("dbdebd","sbdtbd","mbdnbd")
fields = ("timestamp","m_present_secs_into_mission","m_water_vx","m_water_vy","m_depth","c_wpt_lon","c_wpt_lat","m_gps_lon","m_gps_lat","m_pitch","m_roll","m_altitude","m_heading","m_battery","m_avg_speed","m_depth_rate","sci_m_present_secs_into_mission","sci_bbfl2s_bb_scaled","sci_bbfl2s_cdom_scaled","sci_bbfl2s_chlor_scaled","sci_ocr507i_irrad1","sci_ocr507r_rad1","sci_oxy3835_oxygen","sci_water_pressure","sci_water_temp","sci_water_cond")
units = ("timestamp","s","m/s","m/s","m","lon","lat","lon","lat","rad","rad","m","rad","volts","m/s","m/s","s","nodim","ppb","ug/l","uW/cm^2","uW/cm^2","mg/l","bar","degC","S/m")

fieldDict = dict.fromkeys(fields,1);

from pymongo import Connection
connection = Connection('ooma.marine.usf.edu',27017)
db = connection.GDAM

glider = "";
start = -1;
end = -1;

# Set timezone
import os
import time
os.environ['TZ'] = 'UTC'
time.tzset()

#Set start if specified in args
if args.start != -1:
    start = args.start
if args.hoursAgo != -1:
    start = time.time() - 3600*args.hoursAgo;

#Set end if specified in args
if args.end != -1:
    end = args.end

for m in db['missions'].find():
    if(args.mission == m["number"]):
        print "Found Mission " + str(m["number"])
        glider = m["glider"]
        if(start == -1):
            start = m["start"]
        if(m.has_key('end') and end == -1):
            end = m["end"]
        break;

if glider == "":
    import sys
    print "EXIT FAILURE: No Mission " + str(args.mission) + " Found"
    sys.exit(1)

#Set end (UTC) to now if not specified
if end == -1:
    end = time.time()

print "Running query for mission " + str(args.mission) + " from " + str(start) + " to " + str(end)
import datetime
query = {'timestamp':{'$gte':datetime.datetime.fromtimestamp(start),'$lte':datetime.datetime.fromtimestamp(end)}}

for p in pairPriorities:
    col = db[glider+"."+p]
    if(col.find(query).count() > 0):
        print "Outputting " + p + " Pair Data"
        with open(args.outputFile,'wb') as csvfile:
            csvwriter = csv.writer(csvfile,delimiter=',',quotechar='"',quoting=csv.QUOTE_NONNUMERIC)
            csvwriter.writerow(fields)
            csvwriter.writerow(units)
            from collections import OrderedDict
            csvRow = OrderedDict()
            for row in col.find(query):
                for f in fields:
                    csvRow[f] = "NaN"
                for k,v in row.items():
                    keyVals = k.split('-')
                    # More complex if handling gps coords due to storing them as a 2D array for index
                    if(keyVals[0].find('lonlat') >= 0):
                        lonlatParts = keyVals[0].partition('lonlat')
                        lonKey = lonlatParts[0] +'lon'
                        latKey = lonlatParts[0] +'lat'
                        if(csvRow.has_key(lonKey) and csvRow.has_key(latKey)):
                            csvRow[lonKey] = v['0']
                            csvRow[latKey] = v['1']
                    elif(csvRow.has_key(keyVals[0])):
                        csvRow[keyVals[0]] = v
                csvwriter.writerow(list(csvRow.values()))
        break;
    else:
        print "No data found for pair " + p
            
