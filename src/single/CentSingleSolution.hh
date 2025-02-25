#ifndef _CENTSINGLESOLUTION_HH_
#define _CENTSINGLESOLUTION_HH_

#include "../ec/ECBase.hh"
#include "SingleSolutionBase.hh"

using namespace std;

class CentSingleSolution : public SingleSolutionBase {

    private:

        //void genRepairBatchesForSingleFailure(int fail_node_id,  string scenario, bool enqueue);
        int genCentralizedColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int failblkidx);

    public:

        CentSingleSolution();

        int genRepairSolution(string blkname);
};

#endif
