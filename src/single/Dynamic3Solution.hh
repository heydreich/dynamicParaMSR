#ifndef _Dynamic3Solution_HH_
#define _Dynamic3Solution_HH_

#include "../common/TradeoffPoints.hh"
#include "../ec/ECBase.hh"
#include "SingleSolutionBase.hh"
#include "RepairGroup.hh"

#define DEBUG_ENABLE3 false

#define GLOBAL_COLOR3 false

using namespace std;

class Color3Sort {
    public:
        int color;
        double bandwidthperblock;
};  

class Degree3Table {
    public:
        int _color;
        unordered_map<string, int> degree2setIdx;  //
        vector<unordered_set<int>> idx2set;
        int index;

        Degree3Table();
        void pushDegree(string degree, int groupIndex);
};


class Dynamic3Solution : public SingleSolutionBase {

    private:
        TradeoffPoints* _tp;
        RepairGroup* _groupList;
        vector<vector<int>> _interLoadTable;
        vector<Color3Sort*> candidatesSort;
        int candidatesNum;
    
    public:
        // han add
        bool _debug = false;
        int _batch_size;
        double cur_thre;
        

        // xiaolu add

        State evalTable(const vector<vector<int>> & table);
        State evalTable(vector<vector<int>> table, vector<int> colors);
        void SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidates,ECDAG * ecdag, unordered_map<int, int> & coloring);
        int chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, int idxs);
        bool isBetter(State st1, State st2);
        bool isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table);

        int genRepairSolution(string blkname);
        void genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int fail_node_id);
        void useIdleNodesForSingleFailure(Stripe* stripe, const vector<int> & itm_idx, vector<int> & idleColors,ECDAG * ecdag, unordered_map<int, int> & coloring, double limitedbn);

        // end
        Dynamic3Solution();
        Dynamic3Solution(int batchsize, int standbysize, int agentsnum);
        void sortInit(vector<int>& candidateColors);

};

#endif