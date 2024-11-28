#include "DynamicSolution.hh"
#include "ParaSingleSolution.hh"

DynamicSolution::DynamicSolution(){
    _groupList = new RepairGroup();
    candidatesNum = 0;
    _repair_nodeid = 0;
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


int DynamicSolution::chooseColor_single(Stripe* stripe, vector<int> candidateColors, vector<int> childColors, unordered_map<int, int> coloring, 
    vector<int> idxs, vector<int> & idleColors)
{
    // choose the color which has the minimum local load among the childColors node.
    int bestColor = -1;  
    double nodeBottleneck = 0;
    int n = _ec->_n;
    int idx = 0;
    int repair_node_id = _repair_nodeid;
    // cout << "repair_node_id : " << repair_node_id << endl;
    // for (auto color : candidateColors) {
    //     cout << "candidateColor : " << color << endl;
    // }
    // for (auto color : childColors) {
    //     cout << "childColors : " << color << endl;
    // }
    // cout << "candidatesNum : " << candidatesNum << endl;
    for(int i = 0; i < candidatesNum; i++)
    {   
        ColorSort* newColoritem = candidatesSort[i];
        if (nodeBottleneck > newColoritem->bandwidthperblock) {
            candidatesSort[idx]->bandwidthperblock=nodeBottleneck;
            int  j = idx + 1;
            while (j < candidatesNum && candidatesSort[j]->bandwidthperblock > nodeBottleneck) {
                ColorSort* tmp = candidatesSort[j-1];
                candidatesSort[j-1] = candidatesSort[j];
                candidatesSort[j] = tmp; 
                j++;
            }
            break;
        }
        int newColor = newColoritem->color;
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
            idx = i;
        } else if (newNodeBottleneck == nodeBottleneck) {
            if (newColor == repair_node_id) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = repair_node_id;
                idx = i;
            } else if (find(childColors.begin(), childColors.end(), newColor) != childColors.end()) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = newColor;
                idx = i;
            }
        } else {
            if (newColor == repair_node_id && find(childColors.begin(), childColors.end(), repair_node_id) != childColors.end()) {
                nodeBottleneck = newNodeBottleneck;
                bestColor = repair_node_id;
                idx = i;
            }
        }


        // vector<vector<int>> testTable = stripe->evaluateChange(_cluster_size, idx, newColor);
        // State state = evalTable(testTable, childColors);
        // if(isBetter(state, bestState)){
        //     bestState = state;
        //     bestColor = newColor;
        // }
    }
    if (nodeBottleneck < _limitedbn) {
        for(int i = 0; i < idleColors.size(); i++)
        {   
            int newColor = idleColors[i];
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
                idx = i;
            } else if (newNodeBottleneck == nodeBottleneck) {
                if (newColor == repair_node_id) {
                    nodeBottleneck = newNodeBottleneck;
                    bestColor = repair_node_id;
                    idx = i;
                } else if (find(childColors.begin(), childColors.end(), newColor) != childColors.end()) {
                    nodeBottleneck = newNodeBottleneck;
                    bestColor = newColor;
                    idx = i;
                }
            } else {
                if (newColor == repair_node_id && find(childColors.begin(), childColors.end(), repair_node_id) != childColors.end()) {
                    nodeBottleneck = newNodeBottleneck;
                    bestColor = repair_node_id;
                    idx = i;
                }
            }


            // vector<vector<int>> testTable = stripe->evaluateChange(_cluster_size, idx, newColor);
            // State state = evalTable(testTable, childColors);
            // if(isBetter(state, bestState)){
            //     bestState = state;
            //     bestColor = newColor;
            // }
        }
    }

    vector<int> testload = {0,0};
    testload[0] = _interLoadTable[bestColor][0];
    testload[1] = _interLoadTable[bestColor][1];
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

void DynamicSolution::SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidateColors,ECDAG * ecdag, 
    unordered_map<int, int> & coloring, vector<int> & idleColors)
{
    if(DEBUG_ENABLE2)
        cout << "DynamicSolution::SingleMLP for stripe " << stripe->getStripeId() << endl;

    // 1. init information and blank solution
    vector<int> topoIdxs = ecdag->genTopoIdxs();
    for(auto it : itm_idx)
    {
        coloring.insert(make_pair(it, -1));
    }
    // for (auto i : topoIdxs) cout << ", " << i;
    // cout << endl;
    stripe->setColoring(coloring);
    // cout << " stripe->setColoring(coloring); " << endl;
    stripe->evaluateColoring();   
  
    // cout << "group init start " << endl;
    // 2. group all blanknodes
    // group init start
    _groupList->setecn(_ec->_n);
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        int idx = topoIdxs[i];
        //  cout << "197 group init start " << endl;
        ECNode* node = ecdag->getECNodeMap()[idx];
        //  cout << "199 group init start " << endl;
        int oldColor = coloring[idx];
//  cout << "201group init start " << endl;
        // color the vetex with child colors
        vector<int> childNodes = node->getChildIndices();
        //  cout << "204group init start idx " << idx << endl;
        string childsStr = childs2string(childNodes); 
        //  cout << "206group init start childsStr" << childsStr << endl;
        _groupList->push(childsStr, idx);
    }
    // cout << "209group init start " << endl;
    for (auto i : ecdag->getECHeaders()) {
        ECNode* node = ecdag->getECNodeMap()[i];
        // cout << " nodeid " << node->getNodeId() << endl;
        vector<int> childNodes = node->getChildIndices();
        string childsStr = childs2string(childNodes); 
        _groupList->push(childsStr, i);
    }
     _groupList->FinishPush();
    //group init finish
     if(DEBUG_ENABLE2) cout << "group init finished" << endl;
    //  cout << "group init finished" << endl;
    // _groupList->dump();

    for (int i = 0; i < _groupList->getGroupNum(); i++) {
        vector<int> group = _groupList->getGroupByIdx(i);
        if (_groupList->getLevel(i) == 0) _groupList->setLevel(i, 1);
        int cur_level = _groupList->getLevel(i);
        for (auto mem : group) {
            ECNode* node = ecdag->getECNodeMap()[mem];
            vector<int> parentNodes = node->getParentIndices();
            string gstr = childs2string(parentNodes);
            // cout << "gstr : " << gstr << endl;
            int parentIndex = _groupList->getIdxByGroupStr(gstr);
            _groupList->setLevel(parentIndex, cur_level + 1);
            _groupList->AddChildGroup(parentIndex, i);
            _groupList->AddParentGroup(i, parentIndex);
        }
    }
    //add parent child in
    if (DEBUG_ENABLE2) _groupList->dump();
    // _groupList->dump();

    // 2. coloring the blank node one by one 
    int groupnum = _groupList->getGroupNum();   
    // cout << "245!!!  groupnum  " << groupnum << endl; 
    for (int i = 0; i < groupnum; i++) 
    {
        // init global tabl
        vector<int> idxs = _groupList->getGroupByIdx(i);
        int idx0 = idxs[0];
        ECNode* node = ecdag->getECNodeMap()[idx0];
        int oldColor = coloring[idx0];

        // color the vetex with child colors
        vector<int> childColors = node->getChildColors(coloring); 
        
        int greedyColor;
        if (childColors.size() == 0) {
            greedyColor = -1;
        } else {
            greedyColor = chooseColor_single(stripe, candidateColors, childColors, coloring, idxs, idleColors);
        }

        if (greedyColor == oldColor)
            continue;
        for (auto idx : idxs) {
            if (DEBUG_ENABLE2) cout << "for node " << idx  << " color = " << greedyColor << endl;
            coloring[idx] = greedyColor;
            // stripe->changeColor(idx, greedyColor);
            stripe->changeBlockColor(idx, greedyColor);

        }
        _groupList->setColor(i, greedyColor);
        if (DEBUG_ENABLE2) stripe->dumpLoad(_conf->_agents_num + 1);
        if (DEBUG_ENABLE2) cout << stripe->getBdwt() << " " << stripe->getLoad() << endl;
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


int DynamicSolution::genDynamicColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res,
        int fail_node_id) {
    // map a sub-packet idx to a real physical node id
    if(DEBUG_ENABLE2)
        cout << "DynamicSolution::genDynamicColoringForSingleFailure" << endl;
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
        if (GLOBAL_COLOR && i != fail_node_id) candidateColors.push_back(i);
    }
    
    int fail_block_idx = -1;
    for (int i=0; i<curplacement.size(); i++) {
        if (curplacement[i] == fail_node_id)
            fail_block_idx = i;
        // else if (!GLOBAL_COLOR && curplacement[i] != fail_node_id) candidateColors.push_back(i);
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
            if(!GLOBAL_COLOR && _interLoadTable[nodeid][0] == 0) candidateColors.push_back(nodeid);
            _interLoadTable[nodeid][0]++;
        }
        if (DEBUG_ENABLE2) cout << "dagidx : " << dagidx << " , blkidx : " << blkidx <<  " , nodeid : " << nodeid << endl;
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
    _repair_nodeid = repair_node_id;
    for (auto header : ecHeaders) {
        res.insert(make_pair(header, repair_node_id));
    }
    _interLoadTable[repair_node_id][1] = ecw;
    stripe->_bandwidth->setFull(repair_node_id);
    if(!GLOBAL_COLOR) candidateColors.push_back(repair_node_id);
    for (int i = 0; i <= _conf->_agentsIPs.size(); i++) {
        double newbottleneck = stripe->_bandwidth->getBottleneck(i, vector<int>{_interLoadTable[i][0], _interLoadTable[i][1]});
        // cout << "newbottleneck : " << newbottleneck << endl;
        if (newbottleneck < limitedBottleneck) limitedBottleneck = newbottleneck;
    }

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
    // cout << "373 itm_idx_num : " << itm_idx.size() << endl;
    sort(itm_idx.begin(), itm_idx.end());    
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    sortInit(candidateColors);
    // cout << "sortInit complete" << endl;
    // for (auto item: candidatesSort) {
    //     cout << item->color << "," << item->bandwidthperblock << endl;
    // }
    _limitedbn = limitedBottleneck;
    SingleMLP(stripe ,itm_idx, candidateColors, ecdag, res, idleColors);
    gettimeofday(&time2,NULL);

    //idle nodes
    cout << "limitedBottleneck: " << limitedBottleneck << endl;
    int times = 1;
    int round = 0;
   
    // while (times <= 4) {times = useIdleNodes1ForSingleFailure(stripe, itm_idx, idleColors, ecdag, res, limitedBottleneck, times);}
    stripe->evaluateColoring();
    round++;
    
    stripe->evaluateColoring(); 
    stripe->dumpLoad(_conf->_agents_num + 1);
    double idlebn = stripe->getBottleneck();
    cout << "idlebottleneck: " << idlebn << endl;
    cout << "idlelimitedBottleneck: " << limitedBottleneck << endl;



    double latency  = DistUtil::duration(time1, time2);
    cout << "Runtime: " << latency << endl;

    if (idlebn * ecw >= 10) {
        stripe->_bandwidth -> setBandwidth(_conf);
        return 1;
    } else {
        return 0;
    }
} 

void DynamicSolution::sortInit(vector<int>& candidateColors) {
    vector<int> testload = {0,0};
    for (int i = 0; i < candidateColors.size(); i++) {
        candidatesSort.push_back(new ColorSort());
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
                ColorSort* temp = candidatesSort[j];
                candidatesSort[j]=candidatesSort[j+1];
                candidatesSort[j+1]=temp;
            }
        }
    }
}

vector<int> Stringsplit(string str,const char split)
{
	istringstream iss(str);	
	string token;			
    vector<int> res;
	while (getline(iss, token, split))	
	{
		res.push_back(atoi(token.c_str())); 
	}
    return res;
}

int DynamicSolution::useIdleNodes1ForSingleFailure(Stripe* stripe, const vector<int> & itm_idx, vector<int>& idleColors,ECDAG * ecdag, unordered_map<int, int> & coloring, double limitedbn, int times) {
    _groupList->evaluateDegree(ecdag, coloring, _ec->_w, _ec->_n);
    vector<int> usedColors;
    unordered_map<int, DegreeTable*> color2table;
    for (int i = 0; i < _groupList->getGroupNum(); i++) {
        vector<int> degree = _groupList->getDegree(i);
        int color = _groupList->getColor(i);
        // cout << "color" << color << endl;
        string degreeStr = to_string(degree[0]) + "_" + to_string(degree[1]);
        if (color2table.find(color) == color2table.end()) {
            color2table.insert(make_pair(color, new DegreeTable()));
            color2table[color]->_color=color;
        }
        color2table[color]->pushDegree(degreeStr, i);
    }

    // for (auto &t : color2table) {
    //     cout << "color : " << t.first << endl;
    //     for (auto t2 : t.second->degree2setIdx) {
    //         cout << "degree : " << t2.first << " , " << " groupIndex : ";
    //         for (auto t3 : t.second->idx2set[t2.second]) cout << t3 << ",";
    //         cout << endl;
    //     }
    // }

    int bottlenode;
    int port;
    double bottleneck;
    int IdleUse = -1;
    double limitedBottleneck ;
    double IdleNodeBottleneck;
    bool complete = false;

    stripe->evaluateBottleneck(bottlenode, port, bottleneck);
    if (bottleneck == limitedbn || idleColors.size() == 0) return 8;
    int ii = 0;
    unordered_map<int, unordered_map<string, bool>> hasVisit;
    double bottlenodeneck;
    while (!complete) {

        // cout << "bottlenode : " << bottlenode << " , port : " << port << " , bottleneck : " << bottleneck << endl;
        if (bottleneck < 0) {
            stripe->evaluateColoring(); 
            stripe->evaluateBottleneck(bottlenode, port, bottleneck);
        }
        if (find(usedColors.begin(), usedColors.end(), bottlenode) != usedColors.end()) {
            return times;
        }
        if (IdleUse ==  -1) {
            IdleUse = stripe->evaluateIdleColor(idleColors);
            if (IdleUse == -1) break;
            remove(idleColors.begin(), idleColors.end(), IdleUse);
            usedColors.push_back(IdleUse);
        }

        vector<int> nodeLoad = stripe->getNodeLoad(IdleUse);
        IdleNodeBottleneck = stripe->_bandwidth->getBottleneck(IdleUse, vector<int>{nodeLoad[0], nodeLoad[1]});

        // vector<int> idealLoad = stripe->_bandwidth->getIdealLoad();
        vector<int> degree = {0, 0};
        for (auto t : color2table[bottlenode]->degree2setIdx) {
            if (hasVisit[bottlenode][t.first]) continue;
            vector<int> temp = Stringsplit(t.first, '_');
            // cout << temp[0] << "_" << temp[1] << endl;
            // cout << t.first << endl;
            if (degree[port] > temp[port] || (degree[port] == temp[port] && degree[1-port] > temp[1-port]) ) degree = temp;
        }
        // cout << degree[0] << "_" << degree[1] << endl;
        if (degree[port] >= 0) {
            if (times >= 3) break;
            else return times+1;
        }
        string degreeStr = to_string(degree[0]) + "_" + to_string(degree[1]);
        hasVisit[bottlenode][degreeStr] = true;

        int old_color;
        for (int idx : color2table[bottlenode]->idx2set[color2table[bottlenode]->degree2setIdx[degreeStr]]) {
            vector<int> idxs = _groupList->getGroupByIdx(idx);
            old_color = _groupList->getColor(idx);
            for (auto idx2 : idxs) {
                if (DEBUG_ENABLE2) cout << "for node " << idx2  << " color = " << IdleUse << endl;
                // cout << "for node " << idx2  << " color = " << IdleUse << endl;
                
                coloring[idx2] = IdleUse;
                // stripe->changeColor(idx, greedyColor);
                stripe->changeColor(idx2, IdleUse);

            }

            _groupList->changeColor(idx, IdleUse);
            vector<int> nodeLoad2 = stripe->getNodeLoad(bottlenode);
            bottlenodeneck = stripe->_bandwidth->getBottleneck(bottlenode, nodeLoad2);
            vector<int> nodeLoad = stripe->getNodeLoad(IdleUse);
            // cout << "IdleNode: " << nodeLoad[0] << " , " << nodeLoad[1] << endl;
            IdleNodeBottleneck = stripe->_bandwidth->getBottleneck(IdleUse, vector<int>{nodeLoad[0], nodeLoad[1]});
            // if (bottlenodeneck > limitedbn) break;
            if (IdleNodeBottleneck <= limitedbn) {
                for (auto idx2 : idxs) {
                    coloring[idx2] = old_color;
                    // stripe->changeColor(idx, greedyColor);
                    // cout << "for node " << idx2  << " return color = " << old_color << endl;
                    stripe->changeColor(idx2, old_color);
                    // hasVisit[bottlenode][degreeStr] = false;
                }
                hasVisit[bottlenode][degreeStr] = false;
                _groupList->changeColor(idx, IdleUse);
                break;
            }
            if (bottlenodeneck > limitedbn) break;
        }
        
        if (IdleNodeBottleneck <= limitedbn) {
            IdleUse = -1;
            ii = 0;
            stripe->evaluateColoring(); 
            return times + 1;
        }
        
        stripe->evaluateBottleneck(bottlenode, port, bottleneck);
        if (bottleneck >= limitedbn) {
            complete = true;
            return 8;    
        }
        ii++;
    }
    stripe->evaluateColoring(); 
    return times;
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



int DynamicSolution::genRepairSolution(string blkname){
    _blkname = blkname;
    
    // 1.1 construct ECDAG to repair
    int repair_node_id = _conf->_agentsIPs.size();
    ECDAG* curecdag = _stripe->genMutiRepairECDAG(_ec, blkname, repair_node_id);
    // curecdag->dump();

    // 1.2 get fail block idx
    int fail_blk_idx = _stripe->getBlockIdxByName(blkname);
    int fail_node_id = _stripe->getNodeIdByBlock(blkname);
    cout << "fail_node_id: " << fail_node_id << endl;
    
    // 1.2 generate centralized coloring for the current stripe
    unordered_map<int, int> curcoloring;
    int toGen = genDynamicColoringForSingleFailure(_stripe, curcoloring, fail_blk_idx);
    
    // set the coloring result in curstripe
    _stripe->setColoring(curcoloring);
    
    // evaluate the coloring solution
    _stripe->evaluateColoring();
    // _stripe->dumpLoad(5);
    return toGen;
}


DegreeTable::DegreeTable(){
    index=0;
}


void DegreeTable::pushDegree(string degree, int groupIndex) {
    if (degree2setIdx.find(degree) == degree2setIdx.end()) {
        degree2setIdx.insert(make_pair(degree, index));
        index++;
        idx2set.push_back(unordered_set<int>{});
    }
    idx2set[degree2setIdx[degree]].insert(groupIndex);
}