
#ifndef MERGEDGLIDERDATAFILE_HPP
#define MERGEDGLIDERDATAFILE_HPP

#include <vector>
#include <map>
#include <utility>
#include <string>
#include <set>

#include "IGliderDataExporter.hpp"

using namespace std;

class MergedGliderDataFile{
private:
    string path;
    string flightExt;
    set<string>& flightFiles;
    string scienceExt;
    set<string>& scienceFiles;
    
public:
    MergedGliderDataFile(string _path, string _flightExt, set<string>& _flightFiles, string _scienceExt, set<string>& _scienceFiles);
    ~MergedGliderDataFile();
    void exportData(vector<IGliderDataExporter*> exporters);
};

#endif
