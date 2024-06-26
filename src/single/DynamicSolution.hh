#ifndef _DYNAMICSOLUTION_HH_
#define _DYNAMICSOLUTION_HH_

#include "../common/TradeoffPoints.hh"
#include "../ec/ECBase.hh"
#include "SingleSolutionBase.hh"
#include "RepairGroup.hh"

#define DEBUG_ENABLE2 false

using namespace std;

class DynamicSolution : public SingleSolutionBase {

    private:
        TradeoffPoints* _tp;
        RepairGroup* _groupList;
        vector<vector<int>> _interLoadTable;
    
    public:
        // han add
        bool _debug = false;
        int _batch_size;
        double cur_thre;
        

        // xiaolu add

        State evalTable(const vector<vector<int>> & table);
        State evalTable(vector<vector<int>> table, vector<int> colors);
        void SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidates,ECDAG * ecdag, unordered_map<int, int> & coloring);
        int chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, vector<int> idxs);
        bool isBetter(State st1, State st2);
        bool isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table);

        void genRepairSolution(string blkname);
        void genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int fail_node_id);

        // end
        DynamicSolution();
        DynamicSolution(int batchsize, int standbysize, int agentsnum);

};

#endif