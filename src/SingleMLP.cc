#include "common/Config.hh"
#include "common/Stripe.hh"
#include "common/StripeStore.hh"
#include "ec/ECDAG.hh"
#include "protocol/AGCommand.hh"
#include "util/DistUtil.hh"

#include "ec/Clay.hh"
#include "ec/BUTTERFLY.hh"
#include "ec/HHXORPlus.hh"
#include "ec/RDP.hh"

#include "sol/ParallelSolution.hh"

using namespace std;

// const bool DEBUG_ENABLE = true;

void usage()
{
     // ./LimitedGen code n k w reapirIdx loadRatio digits initStr 
     cout << "limitedGen usage: " << endl;
     cout << "  1. code" << endl;
     cout << "  2. n" << endl;
     cout << "  3. k" << endl;
     cout << "  4. w" << endl;
     cout << "  5. reapirIdx" << endl;
}


int main(int argc, char **argv)
{
    // ./SingleMLP code n k w reapirIdx curveSize loadRatio digits initStr 
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    
    if (argc != 6)
    {
        usage();
        return -1;
    }
    string code(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int w = atoi(argv[4]);
    int repairIdx = atoi(argv[5]);
    srand(time(NULL));
    vector<unsigned int> loclist;
    vector<int> nodelist;
    for (int i=0; i<n; i++) {
        nodelist.push_back(i);
    }
    vector<string> blklist;
    for (int i=0; i<n; i++) {
        string blkname = "blk"+to_string(i);
        blklist.push_back(blkname);              
    }
    Stripe* stripe = new Stripe(0, "stripe0", blklist, loclist, nodelist);
    
    ECBase* ec;
    vector<string> param;
    if (code == "Clay") {
        ec = new Clay(n, k, w, {to_string(n-1)});
    } else if (code == "RDP") {
        ec = new RDP(n, k, w, param);
    } else if (code == "HHXORPlus") {
        ec = new HHXORPlus(n, k, w, param);
    } else if (code == "BUTTERFLY") {
        ec = new BUTTERFLY(n, k, w, param);
    } else {
        cout << "Non-supported code!" << endl;
        return 0;
    }
    stripe->genRepairECDAG(ec,repairIdx);
    string configpath = "conf/sysSetting.xml";
    Config* conf = new Config(configpath);
    ParallelSolution* sol = new ParallelSolution(1,1,n+1);
    sol->init({stripe}, ec, code, conf);
    auto placement = stripe->getPlacement();
    placement.erase(std::remove(placement.begin(), placement.end(), repairIdx), placement.end());
    unordered_map<int, int> coloring;
    sol->genColoringForSingleFailure(stripe, coloring, repairIdx,"standby", placement);
    gettimeofday(&time2, NULL);
    double singleTime = DistUtil::duration(time1, time2);
    cout << "Coloring for " << singleTime << endl;
    // stripe->dumpLoad(n);

    // stripe->getECDAG()->dumpTOPO();
    return 0;
}
