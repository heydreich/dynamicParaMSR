#include "RepairGroup.hh"

GroupInfo::GroupInfo() {
        inDegree = 0;
        outDegree = 0;
        level = 0;
        color = -1;
        isRequest = false;
}

RepairGroup::RepairGroup() {
    groupNum = 0;
    virGroupNum = 0;
}

RepairGroup::~RepairGroup() {

}

bool RepairGroup::hasGroup(string Gstr){
    if (childsMapIdx.find(Gstr) == childsMapIdx.end()) {
        return false;
    }
    return true;
}

void RepairGroup::push(string Gstr, int idx) {
    if (!hasGroup(Gstr)) {
        childsMapIdx.insert(make_pair(Gstr, virGroupNum));
        idx2childs.push_back(Gstr);       
        idx2group.push_back(new GroupInfo());
        groupNum++;
        virGroupNum++;
    }
    int index = childsMapIdx[Gstr];
    // cout << "GSTR : " << Gstr << " , index : " << childsMapIdx[Gstr] << endl;
    idx2group[index]->members.push_back(idx);
    idx2group[index]->isRequest = false;
    if (idx > REQUESTOR-_ecn) {
        idx2group[index]->isRequest = true;
        groupNum--;
    }
}

void RepairGroup::dump() {
    for (int i = 0; i < virGroupNum; i++) {
        cout << idx2childs[i] << ":";
        for (auto index : idx2group[i]->members) cout << index << ", ";
        cout << "parent: ";
        for (auto index : idx2group[i]->parentGroupIndices) cout << index << ", ";
        cout << "child: ";
        for (auto index : idx2group[i]->childGroupIndices) cout << index << ", ";
        cout << "  level : " << idx2group[i]->level;
        cout << endl;
    }
    
}

void RepairGroup::setecn(int ecn){
    _ecn = ecn;
}

int RepairGroup::getGroupNum(){
    return groupNum;
}

vector<int> RepairGroup::getGroupByIdx(int index) {
    return vector<int>(idx2group[index]->members);
}

void RepairGroup::setLevel(int groupIndex, int level) {
    idx2group[groupIndex]->level = level;
}

int RepairGroup::getLevel(int groupIndex){
    return idx2group[groupIndex]->level;
}

void RepairGroup::FinishPush(){
    for (int i = 0; i < virGroupNum; i++) {
        string sstr = array2string(idx2group[i]->members);
        groupMapIdx.insert(make_pair(sstr, i));
    }
    
}

int RepairGroup::getIdxByGroupStr(string str) {
    return groupMapIdx[str];
}

string RepairGroup::array2string(vector<int>& Nodes) {
    string res = "";
    for (auto Idx : Nodes) {
        res += to_string(Idx);
        res += "_";
    }
    return res;
}

void RepairGroup::AddParentGroup(int groupIndex, int parentIndex) {
    idx2group[groupIndex]->parentGroupIndices.push_back(parentIndex);
}

void RepairGroup::AddChildGroup(int groupIndex, int childIndex) {
    idx2group[groupIndex]->childGroupIndices.push_back(childIndex);
}

void RepairGroup::setColor(int groupIndex, int Color) {
    idx2group[groupIndex]->color = Color;
}

void RepairGroup::changeColor(int groupIndex, int Color) {
    idx2group[groupIndex]->color = Color;
}

int RepairGroup::getColor(int groupIndex) {
    if (groupIndex > groupNum) {
        return -1;
    }
    return idx2group[groupIndex]->color;
}

void RepairGroup::evaluateDegree(ECDAG * ecdag, unordered_map<int, int> & coloring, int ecw, int ecn){
    for (int i = 0; i < groupNum; i++) {
        // cout << i << " : " << endl; 
        int SameparentColorNum = 0;
        int SamechildColorNum = 0;
        int DiffparentColorNum = 0;
        int DiffchildColorNum = 0;
        int ownColor = idx2group[i]->color;
        // cout << "ownColor : " << ownColor << endl;
        for (int parentIndex : idx2group[i]->parentGroupIndices) {
            if (idx2group[parentIndex]->color == ownColor) SameparentColorNum++;
            else DiffparentColorNum++;
            // cout << "idx2group[parentIndex]->color : " << idx2group[parentIndex]->color << endl;
        }
        int mem = idx2group[i]->members[0];
        ECNode* node = ecdag->getECNodeMap()[mem];
        vector<int> childNodes = node->getChildIndices();
        for (int childIndex: childNodes) {
            int blkidx = childIndex / ecw;
            int nodeid = -1;

            if (coloring[childIndex] == -1) continue;
            if (coloring[childIndex] == ownColor) SamechildColorNum++;
            else DiffchildColorNum++;
            // cout << "idx2group[childIndex]->color : " << idx2group[childIndex]->color << endl;
        }
        idx2group[i]->inDegree = SameparentColorNum-DiffchildColorNum;
        idx2group[i]->outDegree = SamechildColorNum-DiffparentColorNum;

        // cout << "SameparentColorNum : " << SameparentColorNum << endl;
        // cout << "SamechildColorNum : " << SamechildColorNum << endl;
        // cout << "DiffparentColorNum : " << DiffparentColorNum << endl;
        // cout << "DiffchildColorNum : " << DiffchildColorNum << endl;
    }
}

vector<int> RepairGroup::getDegree(int groupIndex) {
    return vector<int>{idx2group[groupIndex]->outDegree, idx2group[groupIndex]->inDegree};
}