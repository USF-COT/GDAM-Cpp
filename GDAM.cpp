/* GDAM - Glider Database Alternative with Mongo
 * 
 * A program that parses given "from-glider" folders.
 * When a new file is newer than the last processing time, 
 * it converts it to ASCII using Webb's dbd2asc, parses it, 
 * then loads it into a mongo database
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "GliderDeploymentDetails.hpp"
#include "MongoGliderDataExporter.hpp"
#include "MergedGliderDataFile.hpp"

using namespace std;
using namespace boost;

void findNewFiles( pair<set <string>,set <string> >& filesToProcess, pair<string,string> flightSciencePair,string directory, time_t startTime){
    
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir(directory.c_str())) == NULL){
        fprintf(stderr,"Unable to open directory %s.\r\n",directory.c_str());
    }

    struct stat fileStats;
    while((dirp = readdir(dp)) != NULL){
        if(dirp->d_name[0] != '.'){
            string fileName(dirp->d_name);
            string extension = fileName.substr(fileName.length()-3,3);
            string fullPath = directory + fileName;
            stat(fullPath.c_str(),&fileStats);
            if(fileStats.st_mtime > startTime){
                if(extension == flightSciencePair.first){
                    filesToProcess.first.insert(fullPath);
                    //printf("Added file %s\r\n",fullPath.c_str());
                } else if(extension == flightSciencePair.second){
                    filesToProcess.second.insert(fullPath);
                    //printf("Added file %s\r\n",fullPath.c_str());
                }
            }
        } else {
            printf("Skipping %s\r\n",dirp->d_name);
        }
    }
    
}

void printUsage(){
    cerr << "Incorrect usage." << endl;
    cerr << "GDAM <glider name:from-glider path> ... <mongo database host>" << endl;
}

int main(int argc, char** argv){
    ssize_t numRead;
    char* p;

    if(argc < 3){
        printUsage();
        exit(EXIT_FAILURE);
    }

    // Find Glider Name, Mission, and Receive Paths
    vector< GliderDeploymentDetails > details;
    vector<string> tokens;
    for(int i=1; i < argc-1; ++i){
        string gliderPathArg(argv[i]);
        boost::split(tokens,gliderPathArg,boost::is_any_of(":"),token_compress_on);
        if(tokens.size() == 2){
            GliderDeploymentDetails det;
            det.name = tokens[0];
            det.receivePath = tokens[1][tokens[1].size()-1] == '/' ? tokens[1] : tokens[1] + "/";
            details.push_back(det);
            printf("Processing %s for glider %s\r\n",det.receivePath.c_str(),det.name.c_str());
        }
    }

    // Find Mongo Host and Port (if entered)
    string hostString = argv[argc-1];

    // Add Exporters Here
    vector<IGliderDataExporter*> exporters;
    for(int i=0; i < details.size(); ++i){
        exporters.push_back(new MongoGliderDataExporter(hostString,details[i].name));
    }

    // Setup pairs
    vector< pair<string,string> > flightSciencePairs;
    flightSciencePairs.push_back( pair<string, string>("dbd","ebd"));
    flightSciencePairs.push_back( pair<string, string>("sbd","tbd"));
    flightSciencePairs.push_back( pair<string, string>("mbd","nbd"));

    for(int i=0; i < details.size(); ++i){ 
        time_t lastTime = exporters[i]->getLastTime();
        cout << "Last time: " << lastTime << endl;
        for(int j=0; j < flightSciencePairs.size(); ++j){
            pair< set<string>,set<string> > filesToProcess;
            findNewFiles(filesToProcess, flightSciencePairs[j],details[i].receivePath, lastTime);
            if(filesToProcess.first.size() > 0){
                printf("Initializing Merger for %s and %s file types.\r\n",flightSciencePairs[j].first.c_str(),flightSciencePairs[j].second.c_str());
                MergedGliderDataFile merger(details[i].receivePath,flightSciencePairs[j].first,filesToProcess.first,flightSciencePairs[j].second,filesToProcess.second);
                printf("Running Exporter.\r\n");
                merger.exportData(exporters);
            }
        }
    }

    exit(EXIT_SUCCESS);
}
