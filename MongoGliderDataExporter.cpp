
#include "MongoGliderDataExporter.hpp"

#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>

MongoGliderDataExporter::MongoGliderDataExporter(string _hostString, string _gliderName) : hostString(_hostString),  gliderName(_gliderName){
}


MongoGliderDataExporter::~MongoGliderDataExporter(){

}

// ScopedDbConnection in Mongo is wonky.  Just going to connect in processReading function
// May want to change this later...
void MongoGliderDataExporter::open(string flightFileType, vector < pair<string,string> >& flightHeaders, string scienceFileType, vector < pair<string,string> >& scienceHeaders){
    fliFileType = flightFileType;
    sciFileType = scienceFileType;

    fprintf(stdout,"Initializing Mongo Connection\r\n");
    db = new DBClientConnection();
    string errMsg;
    try{
        db->connect(hostString,errMsg);
    } catch (DBException &e){
        fprintf(stderr,"Database connection exception: %s.  Message: %s\r\n",e.what(),errMsg.c_str());
    }

    currentTime = time(NULL);
}


void MongoGliderDataExporter::close(){
    fprintf(stdout,"Closing Mongo Connection\r\n");
    delete db;
}

void pullOutGPS(BSONObjBuilder& b, map<string,double>& readings,string gpsField){
    if(readings.count(gpsField) > 0){
        size_t pos = gpsField.find("lon") != string::npos ? gpsField.find("lon") : gpsField.find("lat");
        string fieldBase = gpsField.substr(0,pos);

        BSONArrayBuilder gps(2);
        gps.append(readings[fieldBase+"lon-lon"]);
        gps.append(readings[fieldBase+"lat-lat"]);
        b.append(fieldBase+"lonlat",gps.done());

        readings.erase(fieldBase+"lon-lon");
        readings.erase(fieldBase+"lat-lat");
    }
}

void MongoGliderDataExporter::processReading(map<string,double>& readings){
    BSONObjBuilder b;
    
    // Pull out times
    double timestamp = 0;
    if(readings.count("m_present_time-timestamp") > 0){
        timestamp = readings["m_present_time-timestamp"];
        readings.erase("m_present_time-timestamp");
    }
    if(readings.count("sci_m_present_time-timestamp") > 0){
        timestamp = readings["sci_m_present_time-timestamp"];
        readings.erase("sci_m_present_time-timestamp");
    }

    b.appendTimeT("timestamp",(time_t)timestamp);

    string gpsFields[3] = {"m_gps_lon-lon","m_lon-lon","c_wpt_lon-lon"};
    for(int i=0; i < 3; i++){
        pullOutGPS(b,readings,gpsFields[i]);
    }

    map<string,double>::iterator it;
    for(it=readings.begin(); it != readings.end(); it++){
        b.append((*it).first,(*it).second);
    }
    try{
        db->insert("GDAM."+gliderName+"."+fliFileType+sciFileType,b.obj());
    } catch (DBException &e) {
        fprintf(stderr,"Unable to insert value: %s.\r\n",e.what());
    }
}

void MongoGliderDataExporter::filesProcessed(set<string>& fileNames){
    set<string>::iterator it;

    for(it=fileNames.begin(); it != fileNames.end(); it++){
        struct stat fileDetails;
        BSONObjBuilder b;
        if(stat((*it).c_str(),&fileDetails) == 0){
            b.appendTimeT("date_processed",(long long int)this->currentTime);
            b.appendTimeT("access_time",fileDetails.st_atime);
            b.appendTimeT("modify_time",fileDetails.st_mtime);
            b.appendTimeT("create_time",fileDetails.st_ctime);
            b.appendTimeT("file_size",fileDetails.st_size);
            size_t lastSlash = (*it).rfind("/");
            string name = (*it);
            if(lastSlash != string::npos){
                name = (*it).substr(lastSlash+1);
            }
            b.append("name",name);
            try{
                db->insert("GDAM."+gliderName+".processed_files",b.obj());
            } catch (DBException &e){
                fprintf(stderr,"Unable to insert value: %s.\r\n",e.what());
            }
        } else {
            fprintf(stderr,"Cannot stat file at %s.\r\n",(*it).c_str());
        }
    }
}

time_t MongoGliderDataExporter::getLastTime(){
    DBClientConnection* timeConn = new DBClientConnection();
    string errMsg;

    try{
        timeConn->connect(hostString,errMsg);
    } catch (DBException &e){
        fprintf(stderr,"Database connection exception: %s.  Message: %s\r\n",e.what(),errMsg.c_str());
    }

    Query qu = BSONObj();
    auto_ptr<DBClientCursor> cursor = timeConn->query("GDAM."+gliderName+".processed_files",qu.sort("modify_time",-1));
    time_t retVal = 0;
    while(cursor->more()){
        BSONObj p = cursor->next();
        retVal = p.getField("modify_time").Date().toTimeT();
        break; // Only want first result
    }
    delete timeConn;
    return retVal;
}
