#ifndef _BANDWIDTH_HH_
#define _BANDWIDTH_HH_

#include "../inc/include.hh"
#include "../util/tinyxml2.h"
#include "../util/RedisUtil.hh"
#include "Config.hh"
#include "float.h"

using namespace tinyxml2;

#define BANDWIDTHDEBUG false

class Bandwidth {
  public:
    Bandwidth(std::string& filePath, bool print);
    bool LoadNext();
    ~Bandwidth();
    void Close();
    double getBottleneck(int index, std::vector<int> testTable);
    int getBottlePort(int index, std::vector<int> testTable);
    double evaluateSort(int index, std::vector<int> testTable);
    double getglobalBottleneck(std::vector<std::vector<int>> testtable);
    std::vector<int> getIdealLoad(int index, double limitedbn);
    void setBandwidth(const Config* conf);
    void ResetBandwidth(const Config* conf);
 
    //nodeid -> (upload, download)
  private:
    std::unordered_map<int, std::pair<double, double>> _idx2bdwt;
    int _bwNum;
    int _nodeNum;
    int _cur;
    bool _if_print;
    std::fstream _bwf;
    std::string _eth;

    const std::string ksetCmd = "wondershaper -a ";
    const std::string ResetCmd = "wondershaper -c -a ";

};
#endif
