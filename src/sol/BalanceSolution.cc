#include "BalanceSolution.hh"
#include "OfflineSolution.hh"

BalanceSolution::BalanceSolution(){
}

BalanceSolution::BalanceSolution(int batchsize, int standbysize, int agentsnum) {
    _batch_size = batchsize;
    _standby_size = standbysize;
    _agents_num = agentsnum;
    _cluster_size = standbysize + agentsnum;
}

vector<vector<int>> deepCopyTable(const std::vector<std::vector<int>>& source) {
    vector<vector<int>> destination;
    destination.resize(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        destination[i].resize(source[i].size());
        copy(source[i].begin(), source[i].end(), destination[i].begin());
    }
    return destination;
}

void dumpTable(vector<vector<int>> table)
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

void BalanceSolution::genRepairBatches(int num_failures, vector<int> fail_node_list, string scenario, bool enqueue) {
    cout << "BalanceSolution::genRepairBatches start" << endl;
    // We assume that the replacement node ids are the same with the failed ids
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;
    string offline_solution_path = _conf->_tpDir+"/"+_codename+"_"+to_string(ecn)+"_"+to_string(eck)+"_"+to_string(ecw)+".xml";
    _tp = new TradeoffPoints(offline_solution_path);

    // Xiaolu comment
    // enqueue == true: generated RepairBatch is pushed into a queue in SolutionBase::_batch_queue
    // enqueue == false: generated RepairBatch is added into a vector in SolutionBase::_batch_list
    _enqueue = enqueue;

    if (num_failures == 1) {
        genRepairBatchesForSingleFailure(fail_node_list[0], scenario, enqueue);
    }else{
        cout << "not support multiple failure" << endl;
        exit(1);
    }
    cout << "after gen batchs" << endl;

    // Xiaolu comment
    // we have finished generating RepairBatches
    _finish_gen_batches = true;
}


vector<RepairBatch*> BalanceSolution::formatReconstructionSets() {
    vector<RepairBatch*> toret;
    for(int i=0; i<_rg_num; i++){
        vector<Stripe*> cursetstripe;
        for(int j=0; j<_num_stripes_per_group; j++){
            int idx = _RepairGroup[i*_num_stripes_per_group+j];
            if (idx >= 0) {
                Stripe* repstripe = _stripe_list[idx];
                cursetstripe.push_back(repstripe);
            }
        }
        if (cursetstripe.size() > 0) {
            RepairBatch* curset = new RepairBatch(i, cursetstripe);
            toret.push_back(curset);
        }
    }
    
    cout << "debug placement1, batch_num = "<< toret.size() << endl;
    for(auto batch : toret)
    {
        for(auto stripe: batch->getStripeList())
        {
            cout << " " << stripe->getStripeId();
        }
        cout << endl;
    }

    return toret;
}



// if st1 is better than st2, then return ture;
bool BalanceSolution::isBetter(State st1, State st2)
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
bool BalanceSolution::isBetter(State st1,int color1, State st2, int color2,const vector<vector<int>> & table)
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

int BalanceSolution::chooseColor_single(Stripe* stripe, vector<int> childColors, unordered_map<int, int> coloring, int idx)
{
    // choose the color which has the minimum local load among the childColors node.
    int bestColor = -1;  
    State bestState(INT_MAX, INT_MAX);
    for(auto newColor : childColors)
    {   
        // try new color, and evaluate state
        // cout << "   child color: " << newColor << endl; 
        if(newColor == -1) continue;
        vector<vector<int>> testTable = stripe->evaluateChange(_cluster_size, idx, newColor);
        State state = evalTable(testTable, childColors);
        if(isBetter(state, bestState)){
            bestState = state;
            bestColor = newColor;
        }
    }
    return bestColor;
}

int BalanceSolution::chooseColor_fullnode(Stripe* stripe,const vector<int> & childColors, const vector<vector<int>> & loadTable, int idx)
{
    // choose the color which leads to minimum global_load
    int bestColor = -1;
    int minLoad = INT_MAX;  
    State bestState(INT_MAX, INT_MAX);
    for(auto newColor : childColors) 
    {   
        // try new color, and evaluate state
        
        vector<vector<int>> stripeTable = stripe->evaluateChange(loadTable.size(), idx, newColor);
        vector<vector<int>> currTable = loadTable;
        for(int i = 0; i < loadTable.size(); i++)
        {
            currTable[i][0] += stripeTable[i][0];
            currTable[i][1] += stripeTable[i][1];
        }

        State state = evalTable(currTable); // eval gloabl load
        if(isBetter(state, newColor, bestState, bestColor, currTable)){
            bestState = state;
            bestColor = newColor;
        }
    }
    return bestColor;
}

void BalanceSolution::SingleMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidates,ECDAG * ecdag, unordered_map<int, int> & coloring)
{
    if(DEBUG_ENABLE)
        cout << "BalanceSolution::SingleMLP for stripe " << stripe->getStripeId() << endl;

    // 1. init information and blank solution
    vector<int> topoIdxs = ecdag->genTopoIdxs();
    for(auto it : itm_idx)
    {
        coloring.insert(make_pair(it, -1));
    }
    stripe->setColoring(coloring);
    stripe->evaluateColoring();   
  
    // 2. coloring the blank node one by one    
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        // init global tabl
        int idx = topoIdxs[i];
        ECNode* node = ecdag->getECNodeMap()[idx];
        int oldColor = coloring[idx];

        // color the vetex with child colors
        vector<int> childColors = node->getChildColors(coloring);
        
        int bestColor = chooseColor_single(stripe, childColors, coloring, idx);
        for(auto child : node->getChildNodes()){
            cout << "   child " << child->getNodeId() << " color is " << coloring[child->getNodeId()] << endl;
        }
        cout << "for node " << idx  << " color = " << bestColor << endl;
        
        if (bestColor == oldColor)
            continue;
        coloring[idx] = bestColor;
        stripe->changeColor(idx, bestColor);
        stripe->dumpLoad(14);
        cout << stripe->getBdwt() << " " << stripe->getLoad() << endl;
    }
    
    stripe->evaluateColoring(); 
    vector<int> ret = stripe->getsolution();
    for(auto it : ret) {
        cout << it << " " ;
    }
    cout << endl;

    return;
}

void BalanceSolution::GloballyMLP(Stripe* stripe, const vector<int> & itm_idx, const vector<int> & candidates,ECDAG * ecdag, unordered_map<int, int> & coloring, vector<vector<int>> & 
loadTable)
{
    if(DEBUG_ENABLE)
        cout << "GloballyMLP for stripe " << stripe->getStripeId() << endl;

    // 1. init information and blank solution
    vector<int> topoIdxs = ecdag->genTopoIdxs();
    for(auto it : itm_idx)
    {
        coloring.insert(make_pair(it, -1));
    }
    stripe->setColoring(coloring);
     
    // 2. coloring the blank node one by one
    for (int i = 0; i < topoIdxs.size(); i++) 
    {
        // init global tabl
        int idx = topoIdxs[i];
        ECNode* node = ecdag->getECNodeMap()[idx];
        int oldColor = coloring[idx];

        // color the vetex with child colors
        vector<int> childColors = node->getChildColors(coloring);
        int bestColor = chooseColor_fullnode(stripe, childColors, loadTable, idx);
        if (bestColor == oldColor)
            continue;
        coloring[idx] = bestColor;
        stripe->changeColor(idx, bestColor);
    }
    // 3. add into loadtable
    loadTable = stripe->evalColoringGlobal(loadTable);
    stripe->evaluateColoring();  
    return;
}

void BalanceSolution::genBalanceColoringForSingleFailure(Stripe* stripe, 
        int fail_node_id, string scenario,  vector<vector<int>> & loadTable) {
    // map a sub-packet idx to a real physical node id
    if(DEBUG_ENABLE)
        cout << "BalanceSolution::genBalanceColoringForSingleFailure" << endl;
    ECDAG* ecdag = stripe->getECDAG();
    unordered_map<int, int> res;
    vector<int> curplacement = stripe->getPlacement();
    int ecn = _ec->_n;
    int eck = _ec->_k;
    int ecw = _ec->_w;

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
        }
        res.insert(make_pair(dagidx, nodeid));
    }

    // choose new node after coloring
    int repair_node_id;
    vector<int> candidates;
    repair_node_id = getReplacementNode(stripe->getStripeId(), scenario, avoid_node_ids, loadTable);
    stripe->_new_node = repair_node_id;
    res.insert(make_pair(ecHeaders[0], repair_node_id));

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
    GloballyMLP(stripe ,itm_idx, candidates, ecdag, res, loadTable);
} 

void dumpPlacement(vector<RepairBatch*> batch_list)
{
    for(auto batch: batch_list)
    {
        for(auto stripe: batch->getStripeList())
        {
            cout << " " << stripe->getStripeId();
        }
        cout << " load=" << batch->getLoad() <<" avgload = " << batch->getLoad()*1.0/ batch->getStripeList().size();
        cout << endl;
    }
}

State BalanceSolution::evalTable(const vector<vector<int>> & table)
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

State BalanceSolution::evalTable(vector<vector<int>> table, vector<int> colors)
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

vector<int> genBitVec(vector<int> locList, int clusterSize)
{
    vector<int> bitVec(clusterSize, 0);
    for(auto it : locList)
    {
        bitVec[it] = 1;
    }
    return bitVec;
}

void BalanceSolution::genRepairBatchesForSingleFailure(int fail_node_id,  string scenario, bool enqueue) {

    cout << "BalanceSolution::genRepairBatchesForSingleFailure.fail_node_id = " << fail_node_id << endl;

    // 0. we first figure out stripes that stores a block in $fail_node_id
    filterFailedStripes({fail_node_id});
    cout << "BalanceSolution::genRepairBatchesForSingleFailure.stripes to repair: " << _stripes_to_repair.size() << endl;

    // 1. we divide stripes to repair into batches of size $batchsize
    _num_batches = _stripes_to_repair.size() / _batch_size;
    if (_stripes_to_repair.size() % _batch_size != 0) {        
        _num_batches += 1; 
    }
    cout << "BalanceSolution::genRepairBatchesForSingleFailure.num batches = " << _num_batches << endl;

    for (int batchid=0; batchid<_num_batches; batchid++) {
        vector<Stripe*> cur_stripe_list;
        vector<vector<int>> loadtable = vector<vector<int>> (_cluster_size, {0,0});
        // i refers to the i-th stripe in this batch
        for (int i=0; i<_batch_size; i++) {
            // stripeidx refers to the idx in _stripes_to_repair
            int stripeidx = batchid * _batch_size + i;
            if (stripeidx < _stripes_to_repair.size()) {
                // stripeid refers to the actual id of stripe in all the stripes
                int stripeid = _stripes_to_repair[stripeidx];
                Stripe* curstripe = _stripe_list[stripeid];
                // 1.1 construct ECDAG to repair
                ECDAG* curecdag = curstripe->genRepairECDAG(_ec, fail_node_id);
                // 1.2 generate centralized coloring for the current stripe
                genBalanceColoringForSingleFailure(curstripe, fail_node_id, scenario, loadtable);
                // 1.3 set the coloring result in curstripe
                // 1.4 evaluate the coloring solution
                curstripe->dumpLoad(_cluster_size);
                // 1.4 insert curstripe into cur_stripe_list
                cur_stripe_list.push_back(curstripe);
            }

        }

        // generate a batch based on current stripe list
        RepairBatch* curbatch = new RepairBatch(batchid, cur_stripe_list);
        curbatch->evaluateBatch(_cluster_size);
        // insert current batch into batch list or batch queue
        if (enqueue) {
            _batch_queue.push(curbatch);
        } else {
            _batch_list.push_back(curbatch);
        }
        curbatch->dump();

        //break;
    }
}
