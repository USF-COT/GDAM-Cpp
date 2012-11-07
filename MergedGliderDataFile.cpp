
#include "MergedGliderDataFile.hpp"

#include <sstream>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <math.h>

using namespace std;
using namespace boost;

FILE* processFiles(string path, string fileType, set<string>& files){
    stringstream comSS;
    comSS << "dbd2asc -c /tmp ";
    if(files.size() > 50){
        comSS << path << "*." << fileType;
    } else {
        set<string>::iterator it;
        for(it=files.begin(); it != files.end(); it++){
            comSS << " " << (*it);
        }
    }
    string command = comSS.str();

    printf("Running the following command: %s\r\n", command.c_str());

    return popen(command.c_str(),"r");
}

#define DATALINEMAX 1048576

// Removes extraneous headers and returns a vector of sensor reading types and their units
void findHeaders(FILE* file, vector< pair<string,string> > &headersWithUnits){
    char line[DATALINEMAX];
    char* lastLine;

    // Bleed off extraneous headers
    while((lastLine = fgets(line,DATALINEMAX-1,file)) != NULL){
        if((strchr(line, ':')) == NULL)
            break;
    }

    if(lastLine == NULL){
        fprintf(stderr,"Not enough data found in file\r\n");
        return;
    }

    vector<string> headers;
    vector<string> units;

    split(headers, line, is_any_of(" "),token_compress_on);
    if(fgets(line,DATALINEMAX-1,file) != NULL){
        split(units, line, is_any_of(" "),token_compress_on);
    } else {
        fprintf(stderr,"Not enough data found in file\r\n");
        return;
    }

    // Remove extraneous bytes line
    lastLine = fgets(line,DATALINEMAX-1,file);

    stringstream ss;
    for(int i=0; i < headers.size(); ++i){
        trim(headers[i]);
        trim(units[i]);

        if(headers[i].size() > 0){
            ss << headers[i] << ",";
            if(i < units.size())
                headersWithUnits.push_back(pair <string,string>(headers[i],units[i]));
            else
                headersWithUnits.push_back(pair <string,string>(headers[i],""));
        }
    }

    printf("Found the following headers: %s\r\n",ss.str().c_str());
}

double getDecimalDegrees(double latLon){
    if(latLon == 0)
        return -1;

    string strVal;
    try{
        strVal = boost::lexical_cast<string>(latLon);
    } catch (boost::bad_lexical_cast &){
        fprintf(stderr,"Unable to cast %g to string\r\n",latLon);
        return -1;
    }

    string strDec;
    string strDecFractional;
    size_t decimalPlace = strVal.find_first_of(".");
    if(decimalPlace!=string::npos){
        strDec = strVal.substr(0,decimalPlace-2);
        strDecFractional = strVal.substr(decimalPlace-2);
    } else if (abs(latLon) < 181){
        if(abs(latLon/100) > 100){
            strDec = strVal.substr(0,3);
            strDecFractional = strVal.substr(3);
        }else{
            strDec = strVal.substr(0,2);
            strDecFractional = strVal.substr(2);
        }
    } else {
        return -1;
    } 

    try{
        double dec = boost::lexical_cast<double>(strDec);
        double decFrac = boost::lexical_cast<double>(strDecFractional);
        if(dec < 0) decFrac *= -1;
        return dec + decFrac/60;
    } catch (boost::bad_lexical_cast &){
        fprintf(stderr,"Error casting either %s or %s.\r\n",strDec.c_str(),strDecFractional.c_str());
        return -1;
    }
}

#define NaNVALUE "NaN"

bool mapLine(FILE* file, const vector < pair<string,string> > headers, map<string,double>& readings){
    char line[DATALINEMAX];

    // Clear the map if it isn't already
    if(!readings.empty())
        readings.clear();

    vector<string> values;
    if(fgets(line, DATALINEMAX-1, file) != NULL){
        split(values, line, is_any_of(" "), token_compress_on);
        for(int i=0; i < values.size(); ++i){
            if(values[i].compare(NaNVALUE) == 0){
                continue;
            } else {
                double currVal;
                try{
                    currVal = boost::lexical_cast<double>(values[i]);
                } catch (const std::exception&){
                    continue;
                }

                if(i < headers.size()){
                    if(headers[i].second.compare("lat") == 0 || headers[i].second.compare("lon") == 0){
                        currVal = getDecimalDegrees(currVal); // Always use decimal degrees
                    }
                    string key = headers[i].first + "-" + headers[i].second;
                    readings[key] = currVal;
                } else {
                    fprintf(stderr,"No header information for value in column %d\r\n", i);
                    stringstream keyStream;
                    keyStream << "unknown" << i;
                    readings[keyStream.str()] = currVal;
                } 
            }
        }
        return true;
    } else {
        return false; // no more lines
    }
}

void outputLine(vector<IGliderDataExporter*> exporters, map<string,double> reading){
    vector<IGliderDataExporter*>::iterator it;
    for(it=exporters.begin(); it != exporters.end(); it++){
        (*it)->processReading(reading);
    }
}

#define FLIGHTTIMEHEADER "m_present_time-timestamp"
#define SCIENCETIMEHEADER "sci_m_present_time-timestamp"

void mergeAndExport(FILE* flight, vector< pair<string,string> > &flightHeaders, FILE* science, vector< pair<string,string> > &scienceHeaders, vector<IGliderDataExporter*> exporters){
    bool flightRet,scienceRet;
    map<string,double> flightRead, sciRead;

    flightRet = mapLine(flight,flightHeaders,flightRead);
    scienceRet = mapLine(science,scienceHeaders,sciRead);

    while(flightRet || scienceRet){ // While there's flight or science data, export it
        if(!flightRet){ // If no more flight readings, just export the next science reading
            outputLine(exporters,sciRead);
            scienceRet = mapLine(science,scienceHeaders,sciRead);
        } else if(!scienceRet){ // If no more science readings, just export the next flight reading
            outputLine(exporters,flightRead);
            flightRet = mapLine(flight,flightHeaders,flightRead);
        } else {
            // To Merge or Not to Merge
            double flightTime = flightRead[FLIGHTTIMEHEADER];
            double scienceTime = sciRead[SCIENCETIMEHEADER];
            if(fabs(flightTime - scienceTime) < 1){
                flightRead.insert(sciRead.begin(),sciRead.end());
                outputLine(exporters,flightRead);
                flightRet = mapLine(flight,flightHeaders,flightRead);
                scienceRet = mapLine(science,scienceHeaders,sciRead);
            } else if(flightTime > scienceTime) {
                outputLine(exporters,sciRead);
                scienceRet = mapLine(science,scienceHeaders,sciRead); 
            } else { // (flightTime < sciTime)
                outputLine(exporters,flightRead);
                flightRet = mapLine(flight,flightHeaders,flightRead);
            }
        }
    }
}

MergedGliderDataFile::MergedGliderDataFile(string _path, string _flightExt, set<string>& _flightFiles, string _scienceExt, set<string>& _scienceFiles) : path(_path), flightExt(_flightExt), flightFiles(_flightFiles), scienceExt(_scienceExt), scienceFiles(_scienceFiles){
}

MergedGliderDataFile::~MergedGliderDataFile(){

}

void MergedGliderDataFile::exportData(vector<IGliderDataExporter*> exporters){
    FILE* flight = processFiles(path, flightExt, flightFiles);
    FILE* science = processFiles(path, scienceExt, scienceFiles);

    if((flight != NULL) && (science != NULL)){
        vector < pair<string,string> > flightHeaders;
        findHeaders(flight,flightHeaders);

        vector < pair<string,string> > scienceHeaders;
        findHeaders(science,scienceHeaders);

        if(flightHeaders.size() == 0){
            fprintf(stderr,"Not enough flight data to merge files\r\n");
            pclose(flight);
            pclose(science);
            return;
        }
/*
        if(scienceHeaders.size() == 0){
            fprintf(stderr,"Not enough science data to merge files\r\n");
            pclose(flight);
            pclose(science);
            return;
        }
*/
        map<string, double> line;

        vector<IGliderDataExporter*>::iterator it;
        for(it=exporters.begin(); it != exporters.end(); it++){
            (*it)->open(flightExt, flightHeaders, scienceExt, scienceHeaders);
        }

        mergeAndExport(flight,flightHeaders,science,scienceHeaders,exporters);

        pclose(flight);
        pclose(science);

        for(it=exporters.begin(); it != exporters.end(); it++){
            (*it)->filesProcessed(flightFiles);
            (*it)->filesProcessed(scienceFiles);
            (*it)->close();
        }

    } else {
        if(flight == NULL && science == NULL){
            fprintf(stderr,"Unable to create processes for reading both flight and science data\r\n");
        } else if(flight == NULL){
            fprintf(stderr,"Unable to create processes for reading flight data\r\n");
            pclose(science);
        } else {
            fprintf(stderr,"Unable to create process for reading science data\r\n");
            pclose(flight);
        }
    }
}

