
#include <string>
#include <map>
#include <set>
#include "IGliderDataExporter.hpp"

#include "mongo/client/dbclient.h"

#ifndef MONGOGLIDERDATAEXPORTER_HPP
#define MONGOGLIDERDATAEXPORTER_HPP

using namespace std;
using namespace mongo;

class MongoGliderDataExporter : public IGliderDataExporter{
private:
    string gliderName;
    string fliFileType;
    string sciFileType;

    time_t currentTime;

    string hostString;
    DBClientConnection* db;

public:
    MongoGliderDataExporter(string _hostString, string _gliderName);
    ~MongoGliderDataExporter();
    virtual void open(string flightFileType, vector < pair<string,string> >& flightHeaders, string scienceFileType, vector < pair<string,string> >& scienceHeaders);
    virtual void close();
    virtual void processReading(map<string,double>& readings);
    virtual void filesProcessed(set<string>& fileNames);
    virtual time_t getLastTime();
};

#endif

