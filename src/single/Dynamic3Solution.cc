#include "Dynamic3Solution.hh"
#include "ParaSingleSolution.hh"

Dynamic3Solution::Dynamic3Solution(){
    _groupList = new RepairGroup();
}

Dynamic3Solution::Dynamic3Solution(int batchsize, int standbysize, int agentsnum) {
    _batch_size = batchsize;
}

vector<vector<int>> deepCopyTable3(const std::vector<std::vector<int>>& source) {
    vector<vector<int>> destination;
    destination.resize(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        destination[i].resize(source[i].size());
        copy(source[i].begin(), source[i].end(), destination[i].begin());
    }
    return destination;
}

void dumpTable3(vector<vector<int>> table)
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
bool Dynamic3Solution::isBetter(State st1, State st2)
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
bool Dynamic3Solution::isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table)
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


int Dynamic3Solution::chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, int idx)
{
    // choose the color which has the minimum local load among the childColors node.
    int bestColor = -1;  
    double nodeBottleneck = 0;
    int n = _ec->_n;
    for(int i = 0; i < candidateColors.size(); i++)
    {   

        int newColor = candidateColors[i];
        // try new color, and evaluate state

        vector<vector<int>> testTable = stripe->evaluateChange(_conf->_agentsIPs.size()+1, idx, newColor);
        
        if (DEBUG_ENABLE3) cout << "newColor" << newColor << ":" << testTable[newColor][0] << "," << testTable[newColor][1] << endl;
        double newNodeBottleneck = stripe->_bandwidth->getglobalBottleneck(testTable);
        if (DEBUG_ENABLE3) cout << "99!!!" << newColor << ":" << newNodeBottleneck << endl;
        if (newNodeBottleneck >= nodeBottleneck) {
            nodeBottleneck = newNodeBottleneck;
            bestColor = newColor;
        } 
        if (bestColor == -1) {
            nodeBottleneck = newNodeBottleneck;
            bestColor = newColor;
        }
        


        // vector<vector<int>> testTable = stripe->evaluateChange(_cluster_size, idx, newColor);
        // State state = evalTable(testTable, childColors);
        // if(isBetter(state, bestState)){
        //     bestState = state;
        //     bestColor = newColor;
        // }
    }


    return bestColor;
}

string childs3string(vector<int> childNodes) {
    string res = "";
    for (auto childIdx : childNodes) {
        res += to_string(childIdx);
        res += "_";
    }
    return res;
}

void Dynamic3Solution::SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidateColors,ECDAG * ecdag, unordered_map<int, int> & coloring)
{
    if(DEBUG_ENABLE3)
        cout << "Dynamic3Solution::SingleMLP for stripe " << stripe->getStripeId() << endl;

    // 1. init information and blank solution
    vector<int> topoIdxs = ecdag->genTopoIdxs();
   
    for(auto it : itm_idx)
    {
        coloring.insert(make_pair(it, -1));
    }
    stripe->setColoring(coloring);
    stripe->evaluateColoring();   
  
    // 2. group all blanknodes
    //group init start
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        int idx = topoIdxs[i];
        ECNode* node = ecdag->getECNodeMap()[idx];
        int oldColor = coloring[idx];

        // color the vetex with child colors
        vector<int> childNodes = node->getChildIndices();
        string childsStr = childs3string(childNodes); 
        _groupList->push(childsStr, idx);
    }
    if (1 == 1) {
        ECNode* node = ecdag->getECNodeMap()[REQUESTOR];
        vector<int> childNodes = node->getChildIndices();
        string childsStr = childs3string(childNodes); 
        _groupList->push(childsStr, REQUESTOR);
    }
     _groupList->FinishPush();
    //group init finish
     if(DEBUG_ENABLE3) cout << "group init finished" << endl;
    // _groupList->dump();

    for (int i = 0; i < _groupList->getGroupNum(); i++) {
        vector<int> group = _groupList->getGroupByIdx(i);
        if (_groupList->getLevel(i) == 0) _groupList->setLevel(i, 1);
        int cur_level = _groupList->getLevel(i);
        for (auto mem : group) {
            ECNode* node = ecdag->getECNodeMap()[mem];
            vector<int> parentNodes = node->getParentIndices();
            string gstr = childs3string(parentNodes);
            // cout << "gstr : " << gstr << endl;
            int parentIndex = _groupList->getIdxByGroupStr(gstr);
            _groupList->setLevel(parentIndex, cur_level + 1);
            _groupList->AddChildGroup(parentIndex, i);
            _groupList->AddParentGroup(i, parentIndex);
        }
    }
    //add parent child in
    if (DEBUG_ENABLE3) _groupList->dump();
    // _groupList->dump();

    // 2. coloring the blank node one by one 
    int groupnum = _groupList->getGroupNum();   
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        // init global tabl
        int idx = topoIdxs[i];
        ECNode* node = ecdag->getECNodeMap()[idx];
        int oldColor = coloring[idx];

        // color the vetex with child colors
        vector<int> childColors = node->getChildColors(coloring);
        if (DEBUG_ENABLE3) {
            cout << "child color : ";
            for (auto idx2 : childColors) {
                cout << idx2 << " , ";
            }
            cout << endl;
        }
        
        int greedyColor;
        if (childColors.size() == 0) {
            greedyColor = -1;
        } else {
            greedyColor = chooseColor_single(stripe, childColors, childColors, coloring, idx);
        }
        if (greedyColor == oldColor)
            continue;
        if (DEBUG_ENABLE3) cout << "for node " << idx  << " color = " << greedyColor << endl;
        coloring[idx] = greedyColor;
        stripe->changeColor(idx, greedyColor);
        // stripe->changeBlockColor(idx, greedyColor);

        
        if (DEBUG_ENABLE3) stripe->dumpLoad(_conf->_agents_num + 1);
        if (DEBUG_ENABLE3) cout << stripe->getBdwt() << " " << stripe->getLoad() << endl;
    }
    stripe->evaluateColoring(); 
    stripe->dumpLoad(_conf->_agents_num + 1);
    stripe->dumpBottleneck();
    // stripe->evaluateColoring(); 
    unordered_map<int,int> ret = stripe->getColoring();
    // for(auto it : ret) {
    //     cout << it.first << ":" << it.second << " " << endl;
    // }

    return;
}


void Dynamic3Solution::genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res,
        int fail_node_id) {
    // map a sub-packet idx to a real physical node id
    if(DEBUG_ENABLE3)
        cout << "Dynamic3Solution::genDynamicColoringForSingleFailure" << endl;
    ECDAG* ecdag = stripe->getECDAG();
    vector<int> curplacement = stripe->getPlacement();
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;
    double limitedBottleneck = DBL_MAX;

    vector<int> candidateColors;
    for (int i = 0; i < _conf->_agentsIPs.size()+1; i++) 
    {
        _interLoadTable.push_back(vector<int>(2, 0));
        // candidateColors
        if (GLOBAL_COLOR3 && i != fail_node_id) candidateColors.push_back(i);
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
            if(!GLOBAL_COLOR3 && _interLoadTable[nodeid][0] == 0) candidateColors.push_back(nodeid);
            _interLoadTable[nodeid][0]++;
        }
        if (DEBUG_ENABLE3) cout << "dagidx : " << dagidx << " , blkidx : " << blkidx <<  " , nodeid : " << nodeid << endl;
        res.insert(make_pair(dagidx, nodeid));
    }

    vector<int> idleColors;
    for (int i = 0; i < _conf->_agentsIPs.size(); i++) {
        if (find(candidateColors.begin(), candidateColors.end(), i) != candidateColors.end() || i == fail_node_id) continue;
        idleColors.push_back(i);
    }
    // for (auto i : idleColors) cout << i << "," << endl;


    // choose new node after coloring
    int repair_node_id;

    repair_node_id = _conf->_agentsIPs.size();
    stripe->_new_node = repair_node_id;
    res.insert(make_pair(ecHeaders[0], repair_node_id));
    _interLoadTable[repair_node_id][1] = ecw;
    if(!GLOBAL_COLOR3) candidateColors.push_back(repair_node_id);
    for (int i = 0; i <= _conf->_agentsIPs.size(); i++) {
        double newbottleneck = stripe->_bandwidth->getBottleneck(i, vector<int>{_interLoadTable[i][0], _interLoadTable[i][1]});
        // cout << "newbottleneck : " << newbottleneck << endl;
        if (newbottleneck < limitedBottleneck) limitedBottleneck = newbottleneck;
    }
    // dumpTable3(_interLoadTable);
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
    sortInit(candidateColors);
    // cout << "sortInit complete" << endl;
    // for (auto item: candidatesSort) {
    //     cout << item->color << "," << item->bandwidthperblock << endl;
    // }
    SingleMLP(stripe ,itm_idx, candidateColors, ecdag, res);
    gettimeofday(&time2,NULL);

    //idle nodes
    // cout << "limitedBottleneck : " << limitedBottleneck << endl;
    // useIdleNodesForSingleFailure(stripe, itm_idx, idleColors, ecdag, res, limitedBottleneck);

    double latency  = DistUtil::duration(time1, time2);
    cout << "Runtime: " << latency << endl;
} 

void Dynamic3Solution::sortInit(vector<int>& candidateColors) {
    vector<int> testload = {0,0};
    for (int i = 0; i < candidateColors.size(); i++) {
        candidatesSort.push_back(new Color3Sort());
        int icolor=candidateColors[i];
        candidatesSort[i]->color=icolor;
        testload[0] = _interLoadTable[icolor][0];
        testload[1] = _interLoadTable[icolor][1];
        candidatesSort[i]->bandwidthperblock=_stripe->_bandwidth->evaluateSort(icolor, testload);
        candidatesNum++;
    }

    for (int i = 0; i < candidatesSort.size()-1; i++) {
        for (int j = 0; j < candidatesSort.size()-1-i; j++) {
            double lBottle = candidatesSort[j]->bandwidthperblock;
            double rBottle = candidatesSort[j+1]->bandwidthperblock;
            if (lBottle < rBottle) {
                Color3Sort
                * temp = candidatesSort[j];
                candidatesSort[j]=candidatesSort[j+1];
                candidatesSort[j+1]=temp;
            }
        }
    }
}


void Dynamic3Solution::useIdleNodesForSingleFailure(Stripe* stripe, const vector<int> & itm_idx, vector<int>& idleColors,ECDAG * ecdag, unordered_map<int, int> & coloring, double limitedbn) {
    _groupList->evaluateDegree(ecdag, coloring, _ec->_w, _ec->_n);
    vector<int> usedColors;
    unordered_map<int, Degree3Table*> color2table;
    for (int i = 0; i < _groupList->getGroupNum(); i++) {
        vector<int> degree = _groupList->getDegree(i);
        int color = _groupList->getColor(i);
        // cout << "color" << color << endl;
        string degreeStr = to_string(degree[0]) + "_" + to_string(degree[1]);
        if (color2table.find(color) == color2table.end()) {
            color2table.insert(make_pair(color, new Degree3Table()));
            color2table[color]->_color=color;
        }
        color2table[color]->pushDegree(degreeStr, i);
    }

    for (auto &t : color2table) {
        cout << "color : " << t.first << endl;
        for (auto t2 : t.second->degree2setIdx) {
            cout << "degree : " << t2.first << " , " << " groupIndex : ";
            for (auto t3 : t.second->idx2set[t2.second]) cout << t3 << ",";
            cout << endl;
        }
    }

    int bottlenode;
    int port;
    double bottleneck;
    int IdleUse = -1;
    double limitedBottleneck ;
    double IdleNodeBottleneck;

    bool complete = false;

    stripe->evaluateBottleneck(bottlenode, port, bottleneck);
    while (!complete) {

        cout << "bottlenode : " << bottlenode << " , port : " << port << " , bottleneck : " << bottleneck << endl;
        if (1 == 1 ) complete = true;
        if (IdleUse ==  -1) {
            IdleUse = stripe->evaluateIdleColor(idleColors);
            remove(idleColors.begin(), idleColors.end(), IdleUse);
        }


        IdleNodeBottleneck = stripe->_bandwidth->getBottleneck(IdleUse, vector<int>{_interLoadTable[IdleUse][0], _interLoadTable[IdleUse][1]});
        if (IdleNodeBottleneck <= limitedbn) IdleUse = -1;
        if (bottleneck >= limitedbn) complete = true;
    }
}

State Dynamic3Solution::evalTable(const vector<vector<int>> & table)
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

State Dynamic3Solution::evalTable(vector<vector<int>> table, vector<int> colors)
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



int Dynamic3Solution::genRepairSolution(string blkname){
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


Degree3Table::Degree3Table(){
    index=0;
}


void Degree3Table::pushDegree(string degree, int groupIndex) {
    if (degree2setIdx.find(degree) == degree2setIdx.end()) {
        degree2setIdx.insert(make_pair(degree, index));
        index++;
        idx2set.push_back(unordered_set<int>{});
    }
    idx2set[degree2setIdx[degree]].insert(groupIndex);
}