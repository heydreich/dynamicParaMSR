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


class GroupInfo {
    public:
        vector<int> members;
        vector<int> parentGroupIndices;
        vector<int> childGroupIndices;
        int inDegree;
        int outDegree;
        int level;
        int color;
        bool isRequest;
        bool isReal;

        GroupInfo();
};

class RepairGroup {

    private:
        vector<GroupInfo*> idx2group;
        vector<string> idx2childs;
        unordered_map<string, int> childsMapIdx;
        unordered_map<string, int> groupMapIdx;
        int groupNum;
        int virGroupNum;
        int _ecn;

    public:
        RepairGroup();
        ~RepairGroup();
        bool hasGroup(string Gstr);
        int getGroupNum();
        void push(string Gstr, int idx);
        void dump();
        vector<int> getGroupByIdx(int index);
        void setLevel(int groupIndex, int level);
        int getLevel(int groupIndex);
        void FinishPush();
        int getIdxByGroupStr(string str);
        void AddParentGroup(int groupIndex, int parentIndex);
        void AddChildGroup(int groupIndex, int childIndex);
        void setColor(int groupIndex, int Color);
        void changeColor(int groupIndex, int Color);
        int getColor(int groupIndex);
        void evaluateDegree(ECDAG * ecdag, unordered_map<int, int> & coloring, int ecw, int ecn);
        vector<int> getDegree(int groupIndex);
        void setecn(int ecn);

        string array2string(vector<int>& Nodes);
};

#endif
