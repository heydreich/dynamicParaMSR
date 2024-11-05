#ifndef _DYNAMICSOLUTION_HH_
#define _DYNAMICSOLUTION_HH_

#include "../common/TradeoffPoints.hh"
#include "../ec/ECBase.hh"
#include "SingleSolutionBase.hh"
#include "RepairGroup.hh"

#define DEBUG_ENABLE2 false

#define GLOBAL_COLOR false

using namespace std;

class ColorSort {
    public:
        int color;
        double bandwidthperblock;
};  

class DegreeTable {
    public:
        int _color;
        unordered_map<string, int> degree2setIdx;  //
        vector<unordered_set<int>> idx2set;
        int index;

        DegreeTable();
        void pushDegree(string degree, int groupIndex);
};


class DynamicSolution : public SingleSolutionBase {

    private:
        TradeoffPoints* _tp;
        RepairGroup* _groupList;
        vector<vector<int>> _interLoadTable;
        vector<ColorSort*> candidatesSort;
        int candidatesNum;
        int _repair_nodeid;
        double _limitedbn;
    
    public:
        // han add
        bool _debug = false;
        int _batch_size;
        double cur_thre;
        

        // xiaolu add

        State evalTable(const vector<vector<int>> & table);
        State evalTable(vector<vector<int>> table, vector<int> colors);
        void SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidates,ECDAG * ecdag, unordered_map<int, int> & coloring,vector<int> & idleColors);
        int chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, vector<int> idxs, vector<int> & idleColors);
        bool isBetter(State st1, State st2);
        bool isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table);

        int genRepairSolution(string blkname);
        int genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int fail_node_id);
        int useIdleNodes1ForSingleFailure(Stripe* stripe, const vector<int> & itm_idx, vector<int> & idleColors,ECDAG * ecdag, unordered_map<int, int> & coloring, double limitedbn, int times);

        // end
        DynamicSolution();
        DynamicSolution(int batchsize, int standbysize, int agentsnum);
        void sortInit(vector<int>& candidateColors);

};

#endif