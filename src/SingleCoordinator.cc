//#include "common/CmdDistributor.hh"
#include "common/Config.hh"
#include "common/Coordinator.hh"
#include "common/StripeStore.hh"
#include "inc/include.hh"
#include "sol/RepairBatch.hh"

using namespace std;

void usage() {
    cout << "Usage: ./SingleCoordinator " << endl;
    cout << "   1. method [centralize|pararc|naive|dynamic]" << endl;
    cout << "   2. failnodeid [0]" << endl;
}

int main(int argc, char** argv) {

    if (argc < 4) {
        usage();
        return 0;
    }

    string method = string(argv[1]);
    int failnodeid = atoi(argv[2]);
    
    string configpath = "conf/sysSetting.xml";
    Config* conf = new Config(configpath);
    int ecn = conf->_ecn;
    int eck = conf->_eck;
    int ecw = conf->_ecw;
    
    StripeStore* ss = new StripeStore(conf); 
    Coordinator* coor = new Coordinator(conf, ss);
    coor->initSingleBlockRepair(method, failnodeid);
    struct timeval time1, time2, time3, time4, time5;

    coor->repairSingleBlock();

    // clean
    if (coor)
        delete coor;
    if (ss)
        delete ss;
    if (conf)
        delete conf;
    return 0;
}
