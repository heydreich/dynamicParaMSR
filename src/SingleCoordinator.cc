//#include "common/CmdDistributor.hh"
#include "common/Config.hh"
#include "common/Bandwidth.hh"
#include "common/Coordinator.hh"
#include "common/StripeStore.hh"
#include "inc/include.hh"
#include "sol/RepairBatch.hh"

using namespace std;

void usage() {
    cout << "Usage: ./SingleCoordinator " << endl;
    cout << "   1. method [centralize|pararc|naive|dynamic]" << endl;
    cout << "   2. blockname " << endl;
    cout << "   2. onlyPrintBandwidth [false]" << endl;
}

int main(int argc, char** argv) {

    if (argc < 4) {
        usage();
        return 0;
    }

    string method = string(argv[1]);
    string blkname = string(argv[2]);
    string only_print = string(argv[3]);
    bool PrintOnly = false;
    if (only_print == "true") PrintOnly = true;

    cout << "method: " << method << ", blkname: " << blkname << endl;
    //int failnodeid = atoi(argv[2]);
    //read config file and bandwidth file 
    string configpath = "conf/sysSetting.xml";
    string bandwidthFile  = "conf/bandwidth/bandwidth.txt";
    Config* conf = new Config(configpath);

    int ecn = conf->_ecn;
    int eck = conf->_eck;
    int ecw = conf->_ecw;

    Bandwidth* bdwt = new Bandwidth(bandwidthFile, PrintOnly);
    bdwt->LoadNext();
    // bdwt->setBandwidth(conf);

    
    // generate repair solution
    StripeStore* ss = new StripeStore(conf); 
    Coordinator* coor = new Coordinator(conf, ss);
    ss->setBandwidth(bdwt);

    struct timeval time1, time2, time3, time4, time5;
    // gettimeofday(&time1, NULL);
    coor->repairSingleBlock(method, blkname);
    // gettimeofday(&time2, NULL);
    // cout << "SingleCoordinator::repair time = " << DistUtil::duration(time1, time2) << endl;

    while (bdwt->LoadNext()) coor->repairSingleBlock(method, blkname);
    // clean
    if (coor)
        delete coor;
    if (ss)
        delete ss;
    if (conf)
        delete conf;
    return 0;
}
