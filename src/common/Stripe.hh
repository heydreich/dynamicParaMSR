#ifndef _STRIPE_HH_
#define _STRIPE_HH_

#include "../inc/include.hh"
#include "../ec/ECBase.hh"
#include "../ec/ECDAG.hh"
#include "../ec/Task.hh"
#include "Bandwidth.hh"

using namespace std;

class State{
    public:
    int _load = 0;
    int _bdwt = 0;
    State(int load, int bdwt){
        _load = load;
        _bdwt = bdwt;
    }
};



class Stripe {

    private:
        int _stripe_id;

        // node that stores blocks of this stripe
        vector<int> _nodelist;

        // the ecdag to repair a block
        ECDAG* _ecdag;

        // map a sub-packet idx to a **real physical** node id
        unordered_map<int, int> _coloring;
        
        // statistics
        // for each node, count the number of sub-packets that it receives
        unordered_map<int, int> _in; 
        // for each node, count the number of sub-packets that it sends
        unordered_map<int, int> _out;

        int _bdwt;
        int _load;

        // for prototype
        string _stripe_name;
        vector<string> _blklist;
        vector<unsigned int> _loclist;
        unordered_map<int, vector<Task*>> _taskmap;

        // for single failure
        int _fail_blk_idx;
        vector<int> _fail_blk_idxs;

        

    public:
        Bandwidth* _bandwidth; // lx add

        Stripe(int stripeid, vector<int> nodelist);
        Stripe(int stripeid, string stripename, vector<string> blklist, vector<unsigned int> loclist, vector<int> nodelist);
        ~Stripe();
        vector<int> getPlacement();

        int getStripeId();

        ECDAG* genRepairECDAG(ECBase* ec, int fail_node_id);
        ECDAG* genRepairECDAG(ECBase* ec, string blkname);
        ECDAG* genRepairECDAG(ECBase* ec, vector<int> failidx);
        ECDAG* getECDAG();
        
        void setColoring(unordered_map<int, int> coloring);
        unordered_map<int, int> getColoring();
        void evaluateColoring();

        unordered_map<int, int> getInMap();
        unordered_map<int, int> getOutMap();
        int getBdwt();
        int getLoad();

        unordered_map<int, vector<Task*>> genRepairTasks(int batchid, int ecn, int eck, int ecw, unordered_map<int, int> fail2repair);
        void genLevel0(int batchid, int ecn, int eck, int ecw, vector<int> leaves, unordered_map<int, vector<Task*>>& res);
        void genIntermediateLevels(int batchid, int ecn, int eck, int ecw, vector<int> leaves, unordered_map<int, vector<Task*>>& res);
        void genLastLevel(int batchid, int ecn, int eck, int ecw, vector<int> leaves, unordered_map<int, vector<Task*>>& res);

        int getTaskNumForNodeId(int nodeid);
        vector<Task*> getTaskForNodeId(int nodeid);
        int _new_node = -1;
        void dumpLoad(int);
        void dumpBottleneck();
        void dumpTrans();
        vector<vector<int>> evalColoringGlobal(vector<vector<int>> loadTable);
        void changeColor(int idx, int new_color);
        vector<vector<int>> evaluateChange(int agent_num, int idx, int new_color);
        int getMinOutput(int nodeid);
        vector<int> getsolution();
        int getBlockIdxByName(string blkname);
        int getNodeIdByBlock(string blkname);
        //lx add
        void setBandwidth(Bandwidth* bdwt);
        double getBottleneck();
        void evaluateBottleneck(int& bottlenode, int& port, double& bottleneck);
        void evaluateColorLoad(vector<int> idxs, vector<int>* Load, int newColor);
        void changeBlockColor(int idx, int new_color);
        int evaluateIdleColor( const vector<int>& idleColors);
        ECDAG* genMutiRepairECDAG(ECBase* ec, string blkname, string codename, int repair_node_id);
        double limitedBottleneck();
        double getNodeBottleneck(int nodeid);
        double preEvaluate(int repair_node_id, int ecn, int ecw, ECDAG* selectECDAG);
        vector<int> getNodeLoad(int nodeid);

        void clearContant();
};

#endif
