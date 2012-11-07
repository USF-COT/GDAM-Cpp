/* IGliderDataExporter.hpp - Defines and interface for classes that are fed a flight and science data merged glider file in space separated values format.  The first three lines are headers with the following information:
 *   1) sensor value names
 *   2) sensor units
 *   3) sensor value bytes (not used often)
 *
 * By: Michael Lindemuth
 * University of South Florida
 * College of Marine Science
 * Center for Ocean Technology
 */

#include <vector>
#include <map>
#include <set>
#include <string>
#include <time.h>

#ifndef IGLIDERDATAEXPORTER_HPP
#define IGLIDERDATAEXPORTER_HPP

using namespace std;

class IGliderDataExporter{
    public:
        virtual ~IGliderDataExporter(){}
        virtual void open(string flightFileType, vector < pair<string,string> >& flightHeaders, string scienceFileType, vector < pair<string,string> >& scienceHeaders){}
        virtual void close(){}
        virtual void processReading(map<string,double>& readings){}
        virtual void filesProcessed(set<string>& fileNames){}
        virtual time_t getLastTime(){}
};

#endif
