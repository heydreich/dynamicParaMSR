#ifndef _REPAIRGROUP_HH_
#define _REPAIRGROUP_HH_

#include "../inc/include.hh"
#include "../ec/ECDAG.hh"
#include "../ec/NodeBatchTask.hh"
#include "../common/Stripe.hh"
#include "../common/Config.hh"
#include "../protocol/AGCommand.hh"
#include "../util/DistUtil.hh"


using namespace std;

class RepairGroup {

    private:
        vector<vector<int>> idx2group;
        vector<string> idx2childs;
        unordered_map<string, int> childsMapIdx;
        int groupNum;

    public:
        RepairGroup();
        ~RepairGroup();
        bool hasGroup(string Gstr);
        int getGroupNum();
        void push(string Gstr, int idx);
        void dump();
        vector<int> getGroupByIdx(int index);
};

#endif
