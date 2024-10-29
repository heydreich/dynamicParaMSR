#include "CentSingleSolution.hh"

CentSingleSolution::CentSingleSolution(){

}

int CentSingleSolution::genRepairSolution(string blkname) {

    _blkname = blkname;
    
    // 1.1 construct ECDAG to repair
    ECDAG* curecdag = _stripe->genRepairECDAG(_ec, blkname);

    // 1.2 get fail block idx
    int fail_blk_idx = _stripe->getBlockIdxByName(blkname);
    int fail_node_id = _stripe->getNodeIdByBlock(blkname);
    cout << "fail_node_id: " << fail_node_id << endl;
    
    // 1.2 generate centralized coloring for the current stripe
    unordered_map<int, int> curcoloring;
    genCentralizedColoringForSingleFailure(_stripe, curcoloring, fail_blk_idx);
    
    // set the coloring result in curstripe
    _stripe->setColoring(curcoloring);
    
    // evaluate the coloring solution
    _stripe->evaluateColoring();
}


void CentSingleSolution::genCentralizedColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int failblkidx) {
    // map a sub-packet idx to a real physical node id
    
    ECDAG* ecdag = stripe->getECDAG();
    vector<int> curplacement = stripe->getPlacement();
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;

    // 0. get leave vertices
    vector<int> leaves = ecdag->getECLeaves();
    //cout << "leave num: " << leaves.size() << endl;
    //cout << "  ";
    //for (int i=0; i<leaves.size(); i++)
    //    cout << leaves[i] << " ";
    //cout << endl;

    // 1. get all vertices
    vector<int> allvertices = ecdag->getAllNodeIds();
    //cout << "all idx: " << allvertices.size() << endl;
    //cout << "  ";
    //for (int i=0; i<allvertices.size(); i++)
    //    cout << allvertices[i] << " ";
    //cout << endl;

    // 2. figure out node id of leaves
    vector<int> avoid_node_ids;
    for (int i=0; i<leaves.size(); i++) {
        int dagidx = leaves[i];
        int blkidx = dagidx/ecw;

        int nodeid = -1;
        if (blkidx < ecn) {
            // it's a real block, otherwise, it's a virtual block(shortening)
            nodeid = curplacement[blkidx];
            avoid_node_ids.push_back(nodeid);
        }

        //cout << "dagidx: " << dagidx << ", blkidx: " << blkidx << ", nodeid: " << nodeid << endl;
        res.insert(make_pair(dagidx, nodeid));
    }
    // 2.1 avoid fail nodeid
    int fail_node_id = curplacement[failblkidx];
    avoid_node_ids.push_back(fail_node_id);

    // 3. figure out a nodeid that performs the centralized repair
    // note that we assign the newnode for repair
    int repair_node_id = _conf->_agentsIPs.size();
    stripe->_new_node = repair_node_id;

    // 4. for all the dagidx in allvertices, record nodeid in res
    for (int i=0; i<allvertices.size(); i++) {
        int dagidx = allvertices[i];
        if (res.find(dagidx) != res.end())
            continue;
        else {
            res.insert(make_pair(dagidx, repair_node_id));
        }
    }

//    // // debug
//    // for (auto item: res) {
//    //     int dagidx = item.first;
//    //     int nodeid = item.second;
//    //     cout << "dagidx: " << dagidx << ", nodeid: " << nodeid << endl;
//    // }
//
//
    
}
