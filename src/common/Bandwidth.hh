#ifndef _BANDWIDTH_HH_
#define _BANDWIDTH_HH_

#include "../inc/include.hh"
#include "../util/tinyxml2.h"
#include "float.h"

using namespace tinyxml2;

class Bandwidth {
  public:
    Bandwidth(std::string& filePath);
    bool LoadNext();
    ~Bandwidth();
    void Close();
    double getBottleneck(int newColor, std::vector<int> testTable);
 
    //nodeid -> (upload, download)
  private:
    std::unordered_map<int, std::pair<double, double>> _idx2bdwt;
    int _bwNum;
    int _nodeNum;
    int _cur;
    std::fstream _bwf;

};
#endif
