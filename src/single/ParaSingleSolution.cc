#include "ParaSingleSolution.hh"

ParaSingleSolution::ParaSingleSolution(){ 
}

ParaSingleSolution::ParaSingleSolution(TradeoffPoints* tp){
    setTradeoffPoints(tp);
}




void ParaSingleSolution::genOfflineColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int failblkidx) {
    // map a sub-packet idx to a real physical node id
    
    ECDAG* ecdag = stripe->getECDAG();
    vector<int> curplacement = stripe->getPlacement();
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;

    int fail_block_idx = -1;
    for (int i=0; i<curplacement.size(); i++) {
        if (curplacement[i] == failblkidx)
            fail_block_idx = i;
    }

    // 0. get data structures of ecdag
    unordered_map<int, ECNode*> ecNodeMap = ecdag->getECNodeMap();
    vector<int> ecHeaders = ecdag->getECHeaders();
    vector<int> ecLeaves = ecdag->getECLeaves();

    int intermediate_num = ecNodeMap.size() - ecHeaders.size() - ecLeaves.size();
    //cout << "number of intermediate vertices: " << intermediate_num << endl;

    // 1. color leave vertices 
    // If a leave vertex is part of a real block, we first figure out its block index, and then find corresponding node id
    // If a leave vertex is part of a virtual block (shortening), we mark nodeid as -1

    int realLeaves = 0;
    vector<int> avoid_node_ids;
    for (auto dagidx: ecLeaves) {
        int blkidx = dagidx / ecw;
        int nodeid = -1;

        if (blkidx < ecn) {
            nodeid = curplacement[blkidx];
            avoid_node_ids.push_back(nodeid);
            realLeaves++;
        }
        res.insert(make_pair(dagidx, nodeid));
    }
    avoid_node_ids.push_back(failblkidx);

    // 2. color header
    int repair_node_id =  _conf->_agentsIPs.size();    
    stripe->_new_node = repair_node_id;
    res.insert(make_pair(ecHeaders[0], repair_node_id));

    // 3. read from the offline solution file for coloring of intermediate nodes
    vector<int> itm_idx;
    for (auto item: ecNodeMap) {
        int dagidx = item.first;
        if (find(ecHeaders.begin(), ecHeaders.end(), dagidx) != ecHeaders.end())
            continue;
        if (find(ecLeaves.begin(), ecLeaves.end(), dagidx) != ecLeaves.end())
            continue;
        itm_idx.push_back(dagidx);
    }
    sort(itm_idx.begin(), itm_idx.end());

    // note that in offline solution, colors are within ecn
    // if color == fail_block_idx, find corresponding repair node as the real color
    // otherwise, color = block idx, find corresponding node id
    vector<int> itm_offline_coloring = _tp->getColoringByIdx(fail_block_idx);
    // for (int i = 0; i < itm_offline_coloring.size(); i++) {
    //     cout << itm_offline_coloring[i] << ",";
    // }

    //cout << "itm_idx.size: " << itm_idx.size() << ", itm_offline_coloring.size: " << itm_offline_coloring.size() << endl;

    for (int i=0; i<itm_idx.size(); i++) {
        int dagidx = itm_idx[i];
        int blkidx = itm_offline_coloring[i];
        //cout << "dagidx: " << dagidx << ", blkidx: " << blkidx << endl;

        int nodeid = -1;
        if (blkidx == fail_block_idx)
            nodeid = repair_node_id;
        else
            nodeid = curplacement[blkidx];

        res.insert(make_pair(dagidx, nodeid));
    }
    //// debug
    // 
    // for (auto item: res) {
    //    int dagidx = item.first;
    //    int nodeid = item.second;
    //    cout << "dagidx: " << dagidx << ", nodeid: " << nodeid << endl;
    // }
}

void ParaSingleSolution::genRepairSolution(string blkname) {
    _blkname = blkname;
    
    // 1.1 construct ECDAG to repair
    ECDAG* curecdag = _stripe->genRepairECDAG(_ec, blkname);

    // 1.2 get fail block idx
    int fail_blk_idx = _stripe->getBlockIdxByName(blkname);
    int fail_node_id = _stripe->getNodeIdByBlock(blkname);
    cout << "fail_node_id: " << fail_node_id << endl;
    
    // 1.2 generate centralized coloring for the current stripe
    unordered_map<int, int> curcoloring;
    genOfflineColoringForSingleFailure(_stripe, curcoloring, fail_blk_idx);
    
    // set the coloring result in curstripe
    _stripe->setColoring(curcoloring);
    
    // evaluate the coloring solution
    _stripe->evaluateColoring();
    _stripe->dumpLoad(_conf->_agents_num + 1);
}

void ParaSingleSolution::setTradeoffPoints(TradeoffPoints* tp) {
    _tp = tp;
}