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
    cout << "   2. blockname " << endl;
}

int main(int argc, char** argv) {

    if (argc < 3) {
        usage();
        return 0;
    }

    string method = string(argv[1]);
    string blkname = string(argv[2]);
    cout << "method: " << method << ", blkname: " << blkname << endl;
    //int failnodeid = atoi(argv[2]);
    
    string configpath = "conf/sysSetting.xml";
    Config* conf = new Config(configpath);
    int ecn = conf->_ecn;
    int eck = conf->_eck;
    int ecw = conf->_ecw;
    
    // generate repair solution
    StripeStore* ss = new StripeStore(conf); 
    Coordinator* coor = new Coordinator(conf, ss);

    struct timeval time1, time2, time3, time4, time5;
    gettimeofday(&time1, NULL);
    coor->repairSingleBlock(method, blkname);
    gettimeofday(&time2, NULL);
    cout << "SingleCoordinator::repair time = " << DistUtil::duration(time1, time2) << endl;

    // clean
    if (coor)
        delete coor;
    if (ss)
        delete ss;
    if (conf)
        delete conf;
    return 0;
}
