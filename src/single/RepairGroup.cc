#include "RepairGroup.hh"

RepairGroup::RepairGroup() {
    groupNum = 0;
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
    if (hasGroup(Gstr)) {
        int index = childsMapIdx[Gstr];
        idx2group[index].push_back(idx);
    } else {
        childsMapIdx.insert(make_pair(Gstr, groupNum));
        idx2childs.push_back(Gstr);
        idx2group.push_back(vector<int>{idx});
        groupNum++;
    }
}

void RepairGroup::dump() {
    for (int i = 0; i < groupNum; i++) {
        cout << idx2childs[i] << ":";
        for (auto index : idx2group[i]) cout << index << ",";
        cout << endl;
    }
    
}

int RepairGroup::getGroupNum(){
    return groupNum;
}

vector<int> RepairGroup::getGroupByIdx(int index) {
    return vector<int>(idx2group[index]);
}