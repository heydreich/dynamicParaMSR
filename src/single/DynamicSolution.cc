#include "DynamicSolution.hh"
#include "ParaSingleSolution.hh"

DynamicSolution::DynamicSolution(){
    _groupList = new RepairGroup();
}

DynamicSolution::DynamicSolution(int batchsize, int standbysize, int agentsnum) {
    _batch_size = batchsize;
}

vector<vector<int>> deepCopyTable2(const std::vector<std::vector<int>>& source) {
    vector<vector<int>> destination;
    destination.resize(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        destination[i].resize(source[i].size());
        copy(source[i].begin(), source[i].end(), destination[i].begin());
    }
    return destination;
}

void dumpTable2(vector<vector<int>> table)
{
    cout << "in  ";
    for(auto it : table)
    {
        cout  << it[1]<<" ";
    }
    cout << endl;

    cout << "out ";
    for(auto it : table)
    {
        cout  << it[0] << " ";
    }
    cout << endl;
}





// if st1 is better than st2, then return ture;
bool DynamicSolution::isBetter(State st1, State st2)
{
    // if load and bandwidth both equal, will return false
    if(st1._load < st2._load){
        return true;
    }else if(st1._load == st2._load){
        // if(st1._bdwt < st2._bdwt){
        if(st1._bdwt <= st2._bdwt){
            return true;
        }else if(st1._bdwt == st2._bdwt){
            return rand()%2 == 0 ;
        }
    }
    return false;
}




// if st1 is better than st2, then return ture;
bool DynamicSolution::isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table)
{
    // just consider global input, because input load is easier to occur imbanlance
    if(st1._load < st2._load){
        return true;
    }else if(st1._load == st2._load){
        if (st1._bdwt < st2._bdwt){
            return true;
        }else{
            return table[color1][0] < table[color2][0];
        }

    }
    return false;
}


int DynamicSolution::chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, vector<int> idxs)
{
    // choose the color which has the minimum local load among the childColors node.
    int bestColor = -1;  
    double nodeBottleneck = 0;
    int n = _ec->_n;
    int repair_node_id = candidateColors[candidateColors.size()-1];
    for(auto newColor:candidateColors)
    {   
        // try new color, and evaluate state
        // cout << "   child color: " << newColor << endl; 

        vector<int> testload = {0,0};
        testload[0] = _interLoadTable[newColor][0];
        testload[1] = _interLoadTable[newColor][1];
        // cout << "before"  << idxs[0] << ":" << testload[0] << "," << testload[1] << endl;
        stripe->evaluateColorLoad(idxs, &testload, newColor);
        if (DEBUG_ENABLE2) cout << "newColor" << newColor << ":" << testload[0] << "," << testload[1] << endl;
        double newNodeBottleneck = stripe->_bandwidth->getBottleneck(newColor, testload);
        if (DEBUG_ENABLE2) cout << "99!!!" << newColor << ":" << newNodeBottleneck << endl;
        if (newNodeBottleneck > nodeBottleneck) {
            nodeBottleneck = newNodeBottleneck;
            bestColor = newColor;
        } else if (newNodeBottleneck == nodeBottleneck) {
            if (newColor == repair_node_id) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = repair_node_id;
            } else if (find(childColors.begin(), childColors.end(), newColor) != childColors.end()) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = newColor;
            }
        } else {
            if (newColor == repair_node_id && find(childColors.begin(), childColors.end(), repair_node_id) != childColors.end()) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = repair_node_id;
            }
        }

        // vector<vector<int>> testTable = stripe->evaluateChange(_cluster_size, idx, newColor);
        // State state = evalTable(testTable, childColors);
        // if(isBetter(state, bestState)){
        //     bestState = state;
        //     bestColor = newColor;
        // }
    }
    vector<int> testload = {0,0};
    testload[0] = _interLoadTable[bestColor][0];
    testload[1] = _interLoadTable[bestColor][1];
    // cout << "before"  << idxs[0] << ":" << testload[0] << "," << testload[1] << endl;
    stripe->evaluateColorLoad(idxs, &testload, bestColor);
    if (DEBUG_ENABLE2) cout << "bestColor" << bestColor << ":" << testload[0] << "," << testload[1] << endl;
    _interLoadTable[bestColor][0] = testload[0];
    _interLoadTable[bestColor][1] = testload[1];

    return bestColor;
}

string childs2string(vector<int> childNodes) {
    string res = "";
    for (auto childIdx : childNodes) {
        res += to_string(childIdx);
        res += "_";
    }
    return res;
}

void DynamicSolution::SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidateColors,ECDAG * ecdag, unordered_map<int, int> & coloring)
{
    if(DEBUG_ENABLE2)
        cout << "DynamicSolution::SingleMLP for stripe " << stripe->getStripeId() << endl;

    // 1. init information and blank solution
    vector<int> topoIdxs = ecdag->genTopoIdxs();
    for(auto it : itm_idx)
    {
        coloring.insert(make_pair(it, -1));
    }
    stripe->setColoring(coloring);
    stripe->evaluateColoring();   
  
    // 2. group all blanknodes
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        int idx = topoIdxs[i];
        ECNode* node = ecdag->getECNodeMap()[idx];
        int oldColor = coloring[idx];

        // color the vetex with child colors
        vector<int> childNodes = node->getChildIndices();
        string childsStr = childs2string(childNodes); 
        _groupList->push(childsStr, idx);
    }
    if (DEBUG_ENABLE2) _groupList->dump();

    // 2. coloring the blank node one by one 
    int groupnum = _groupList->getGroupNum();   
    for (int i = 0; i < groupnum; i++) 
    {
        // init global tabl
        vector<int> idxs = _groupList->getGroupByIdx(i);
        int idx0 = idxs[0];
        ECNode* node = ecdag->getECNodeMap()[idx0];
        int oldColor = coloring[idx0];

        // color the vetex with child colors
        vector<int> childColors = node->getChildColors(coloring);
        
        int greedyColor = chooseColor_single(stripe, candidateColors, childColors, coloring, idxs);
        if (DEBUG_ENABLE2) cout << "for node " << idxs[0]  << " color = " << greedyColor << endl;
        if (greedyColor == oldColor)
            continue;
        for (auto idx : idxs) {
            if (DEBUG_ENABLE2) cout << "for node " << idx  << " color = " << greedyColor << endl;
            coloring[idx] = greedyColor;
            stripe->changeColor(idx, greedyColor);
        }
        if (DEBUG_ENABLE2) stripe->dumpLoad(_conf->_agents_num + 1);
        if (DEBUG_ENABLE2) cout << stripe->getBdwt() << " " << stripe->getLoad() << endl;
    }
    stripe->dumpLoad(_conf->_agents_num + 1);
    stripe->dumpBottleneck();
    stripe->evaluateColoring(); 
    unordered_map<int,int> ret = stripe->getColoring();
    // for(auto it : ret) {
    //     cout << it.first << ":" << it.second << " " << endl;
    // }
    cout << endl;

    return;
}


void DynamicSolution::genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res,
        int fail_node_id) {
    // map a sub-packet idx to a real physical node id
    if(DEBUG_ENABLE2)
        cout << "DynamicSolution::genDynamicColoringForSingleFailure" << endl;
    ECDAG* ecdag = stripe->getECDAG();
    vector<int> curplacement = stripe->getPlacement();
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;

    vector<int> candidateColors;
    for (int i = 0; i < _conf->_agentsIPs.size()+1; i++) 
    {
        _interLoadTable.push_back(vector<int>(2, 0));
        // candidateColors
        if (i != fail_node_id) candidateColors.push_back(i);
    }
    
    int fail_block_idx = -1;
    for (int i=0; i<curplacement.size(); i++) {
        if (curplacement[i] == fail_node_id)
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
    avoid_node_ids.push_back(fail_node_id);
    for (auto dagidx: ecLeaves) {
        int blkidx = dagidx / ecw;
        int nodeid = -1;

        if (blkidx < ecn) {
            nodeid = curplacement[blkidx];
            avoid_node_ids.push_back(nodeid);
            realLeaves++;
            // if(_interLoadTable[nodeid][0] == 0) candidateColors.push_back(nodeid);
            _interLoadTable[nodeid][0]++;
        }
        if (DEBUG_ENABLE2) cout << "dagidx : " << dagidx << " , blkidx : " << blkidx <<  " , nodeid : " << nodeid << endl;
        res.insert(make_pair(dagidx, nodeid));
    }


    // choose new node after coloring
    int repair_node_id;

    repair_node_id = _conf->_agentsIPs.size();
    stripe->_new_node = repair_node_id;
    res.insert(make_pair(ecHeaders[0], repair_node_id));
    _interLoadTable[repair_node_id][1] = ecw;
    // candidateColors.push_back(repair_node_id);
    // dumpTable2(_interLoadTable);
    // for (auto color : candidateColors) {
    //     cout << "candidateColor : " << color << endl;
    // }

    // intermediate node idxs
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
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    SingleMLP(stripe ,itm_idx, candidateColors, ecdag, res);
    gettimeofday(&time2,NULL);
    double latency  = DistUtil::duration(time1, time2);
    cout << "Runtime: " << latency << endl;
} 


State DynamicSolution::evalTable(const vector<vector<int>> & table)
{
    int bdwt = 0;
    int load = 0;
    for(auto item: table)
    {
        // bdwt += item[0]; // out
        bdwt += item[1]; // in
        load = max(load, item[0]); 
        load = max(load, item[1]);
    }
    return State(load,bdwt);
}

State DynamicSolution::evalTable(vector<vector<int>> table, vector<int> colors)
{
    // return the load which is the maximum load among the colors_node
    // return the bdwt which is full dag bandwidth
    int bdwt = 0;
    int load = 0;
    for(auto item: table)
    {
        bdwt += item[1]; 
    }

    for(auto color : colors)
    {
        load = max(load, table[color][0]); 
        load = max(load, table[color][1]);
    }
    return State(load,bdwt);
}



void DynamicSolution::genRepairSolution(string blkname){
    _blkname = blkname;
    
    // 1.1 construct ECDAG to repair
    ECDAG* curecdag = _stripe->genRepairECDAG(_ec, blkname);

    // 1.2 get fail block idx
    int fail_blk_idx = _stripe->getBlockIdxByName(blkname);
    int fail_node_id = _stripe->getNodeIdByBlock(blkname);
    cout << "fail_node_id: " << fail_node_id << endl;
    
    // 1.2 generate centralized coloring for the current stripe
    unordered_map<int, int> curcoloring;
    genDynamicColoringForSingleFailure(_stripe, curcoloring, fail_blk_idx);
    
    // set the coloring result in curstripe
    _stripe->setColoring(curcoloring);
    
    // evaluate the coloring solution
    _stripe->evaluateColoring();
    // _stripe->dumpLoad(5);
}