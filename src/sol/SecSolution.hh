#ifndef _CENTSOLUTION_HH_
#define _CENTSOLUTION_HH_

#include "../ec/ECBase.hh"
#include "SolutionBase.hh"
#include "RepairBatch.hh"
#include "FlowGraph.hh"

using namespace std;

class SecSolution : public SolutionBase {

    private:
        //int _batch_size;
        //int _num_batches;

        void genRepairBatchesForSingleFailure(int fail_node_id,  string scenario, bool enqueue);
        void genCentralizedColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int fail_node_id,  string scenario, int repairnodeid);

    public:

        SecSolution();
        //SecSolution(string param);

        void genRepairBatches(int num_failures, vector<int> fail_node_list, string scenario, bool enqueue);
};

#endif
