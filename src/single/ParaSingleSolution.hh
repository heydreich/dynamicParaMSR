#ifndef _PARASINGLESOLUTION_HH_
#define _PARASINGLESOLUTION_HH_

#include "../common/TradeoffPoints.hh"
#include "../ec/ECBase.hh"
#include "SingleSolutionBase.hh"

using namespace std;

class ParaSingleSolution : public SingleSolutionBase {

    private:
        TradeoffPoints* _tp;
        void genOfflineColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int failblkidx);

    public:
        // void genRepairBatchesForSingleFailure(int fail_node_id, string scenario, bool enqueue);


        ParaSingleSolution();
        ParaSingleSolution(TradeoffPoints* tp);
        
        int genRepairSolution(string blkname);         
        void setTradeoffPoints(TradeoffPoints* tp);
        
};

#endif