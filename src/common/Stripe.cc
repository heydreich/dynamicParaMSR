#include "Stripe.hh"

Stripe::Stripe(int stripeid, vector<int> nodelist) {
    _stripe_id = stripeid;
    _nodelist = nodelist;
}

Stripe::Stripe(int stripeid, string stripename, vector<string> blklist, vector<unsigned int> loclist, vector<int> nodelist) {
    _stripe_id = stripeid;
    _stripe_name = stripename;
    _blklist = blklist;
    _loclist = loclist;
    _nodelist = nodelist;
}

Stripe::~Stripe() {
    // note that we generate ecdag inside each stripe
    // thus we should free ecdag inside stripe
    if (_ecdag)
        delete _ecdag;
}

vector<int> Stripe::getPlacement() {
    return _nodelist;
}

int Stripe::getStripeId() {
    return _stripe_id;
}

ECDAG* Stripe::genRepairECDAG(ECBase* ec, int fail_node_id) {
    vector<int> from;
    vector<int> to;

    int ecw = ec->_w;                                                                                                                                   
    int ecn = ec->_n;

    // blkidx refers to the idx of a block in this stripe
    for (int blkidx=0; blkidx<ecn; blkidx++){
        int curnodeid = _nodelist[blkidx];
        if (curnodeid == fail_node_id) {
            // if this node fails, we need to repair this block
            // offset refers the offset of sub-packets
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                to.push_back(pktidx);
            }
            _fail_blk_idx = blkidx;
        } else {
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                from.push_back(pktidx);
            }
        }
    }
    _ecdag = ec->Decode(from, to);
    // cout << "debug 57 " << endl;
    // _ecdag->dumpTOPO();
    _ecdag->Concact(to);
    return _ecdag;
}

ECDAG* Stripe::genRepairECDAG(ECBase* ec, string blkname) {
    vector<int> from;
    vector<int> to;

    int ecw = ec->_w;                                                                                                                                   
    int ecn = ec->_n;

    // blkidx refers to the idx of a block in this stripe
    for (int blkidx=0; blkidx<ecn; blkidx++){
        string curblk = _blklist[blkidx];
        if (curblk == blkname) {
            // this is the failed block, we need to repair this block
            // offset refers the offset of sub-packets
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                to.push_back(pktidx);
            }
            _fail_blk_idx = blkidx;
        } else {
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                from.push_back(pktidx);
            }
        }
    }
    _ecdag = ec->Decode(from, to);
    // cout << "debug 57 " << endl;
    // _ecdag->dumpTOPO();
    _ecdag->Concact(to);
    return _ecdag;
}

ECDAG* Stripe::getECDAG() {
    return _ecdag;
}

void Stripe::setColoring(unordered_map<int, int> coloring) {
    _coloring = coloring;
}

unordered_map<int, int> Stripe::getColoring() {
    return _coloring;
}

void Stripe::changeColor(int idx, int new_color)
{
    int debug;
    // cout << "for node " << idx  << " color = " << new_color << endl;

    ECNode *node = _ecdag->getECNodeMap()[idx];
    int old_color = _coloring[node->getNodeId()];
    _load = 0;
    _bdwt = 0;

    vector<ECNode *> parents = node->getParentNodes(); // parents evaluate by color
    vector<ECNode *> children = node->getChildNodes(); // children evaluate by node

    // dumpLoad(14);
    

    // for parent color:
    // if send chunk to parent, will not count repeat 
    set<int> parent_colors;
    // cout << "parent: " << endl; 
    for (auto parent : parents)
    {
        // cout << parent->getNodeId() << " color is " << _coloring[parent->getNodeId()] << endl;
        parent_colors.insert(_coloring[parent->getNodeId()]);
    }

    // cout << "childs" << endl;
    for (auto child : children)
    {
        // cout << child->getNodeId() << " color is " << _coloring[child->getNodeId()] << endl;
    }


    for (auto parent_color : parent_colors)
    {
        // cout << "parent_color " << parent_color << endl;
        if(parent_color == -1)
            continue;
        
        // for old color delete
        if(parent_color != old_color && old_color != -1){ // parent in --, old out --
            _in[parent_color] --;
            _out[old_color] --;
        }

        // for new color add
        if(parent_color != new_color && new_color != -1){ // parent in ++, new out ++
            _in[parent_color]++; 
            _out[new_color]++;
        }
    }

    // for child color: if receive from child, need to count if the child output to repeat color 
    vector<int> childsColor = node->getChildColors(_coloring);

    // if old color == -1, just consider the new color add new output
    if(old_color == -1)
    {
        for (auto child : children)
        {
            // cout << "   for child " << child->getNodeId()<< endl;
            vector<ECNode *> child_parents = child->getParentNodes();
            int child_color = _coloring[child->getNodeId()];
            if(child_color == -1){
                continue;
            }
            // if old_color_count = 1, old_color in --, child_color out --
            // if new_color_conut = 0, new_color in ++, child_color out ++
            int count_new_color = 0;
            for(auto child_parent : child_parents){
                // cout << "       node " << child_parent->getNodeId()<< "color is " << _coloring[child_parent->getNodeId()] << endl;
                if(_coloring[child_parent->getNodeId()] == new_color){
                    count_new_color++;
                }
            }
            // if child had not send chunk to ont vetex which is newcolor, then add new color will being  one chunk output
            if(count_new_color == 0 && new_color != child_color){
                _out[child_color]++;
                _in[new_color]++;
            }
        }
    }
    else{
        // for every child, consider its output, 
        // if child just send one chunk to one vectex which is oldcolor, then delete old color will minus one chunk output
        // if child had not send chunk to ont vetex which is newcolor, then add new color will being  one chunk output
        for (auto child : children)
        {
            // cout << "   for child " << child->getNodeId()<< endl;
            
            vector<ECNode *> child_parents = child->getParentNodes();
           
            int child_color = _coloring[child->getNodeId()];
            if(child_color == -1){
                continue;
            }
            // if old_color_count = 1, old_color in --, child_color out --
            // if new_color_conut = 0, new_color in ++, child_color out ++
            int count_old_color = 0, count_new_color = 0;
            
            // count this child`s parent vetex output color
            for(auto child_parent : child_parents){
                // cout << "       node " << child_parent->getNodeId()<< "color is " << _coloring[child_parent->getNodeId()] << endl;
                if(_coloring[child_parent->getNodeId()] == old_color){
                    count_old_color++;
                }
                if(_coloring[child_parent->getNodeId()] == new_color){
                    count_new_color++;
                }
            }
            
            // delete old_color will minus one chunk output 
            if(count_old_color == 1 && old_color != child_color){
                // _out[old_color]--;
                // _in[child_color]--;
                _in[old_color]--;
                _out[child_color]--;
            }
            
            // add new color will bring one chunk input
            if(count_new_color == 0 && new_color != child_color){
                _in[new_color]++;
                _out[child_color]++;
            }
        }
    }
    
    for (auto item: _in) {
        _bdwt += item.second;
        if (item.second > _load)
            _load = item.second;
    }

    for (auto item: _out) {
        if (item.second > _load)
            _load = item.second;
    }

    _coloring[idx] = new_color;
    // dumpLoad(14);
    
    return;
}


void Stripe::clearContant() {
    _taskmap.clear();
}

void Stripe::changeBlockColor(int idx, int new_color) {
    _coloring[idx] = new_color;
    return;
}

vector<int> Stripe::getNodeLoad(int nodeid) {
    return vector<int>{_in[nodeid], _out[nodeid]};
}

vector<vector<int>> Stripe::evaluateChange(int cluster_size, int idx, int new_color)
{
    // change vetex[idx] to new_color, then generate the local load table 

    // copy load table
    vector<vector<int>> loadTable(cluster_size, {0,0});

    for(auto it : _in)
    {
        loadTable[it.first][1] = it.second;
    }
    for(auto it : _out)
    {
        loadTable[it.first][0] = it.second;
    }
    
    ECNode *node = _ecdag->getECNodeMap()[idx];
    int old_color = _coloring[node->getNodeId()];

    vector<ECNode *> parents = node->getParentNodes(); // parents evaluate by color
    vector<ECNode *> children = node->getChildNodes(); // children evaluate by node
    // for parent color:
    // if send chunk to parent, will not count repeat 
    set<int> parent_colors;
    for (auto parent : parents)
    {
        parent_colors.insert(_coloring[parent->getNodeId()]);
    }

    for (auto parent_color : parent_colors)
    {
        if(parent_color == -1){
            continue;
        }

        // for old color delete
        if(parent_color !=  old_color && old_color != -1){ // parent in --, old out --
            loadTable[old_color][0]--;
            loadTable[parent_color][1]--;
        }
        // for new color add
        if(parent_color != new_color && new_color != -1){ // parent in ++, new out ++
            loadTable[new_color][0]++;
            loadTable[parent_color][1]++;
        }
    }

    // for child color: if receive from child, need to count if the child output to repeat color
    vector<int> childsColor = node->getChildColors(_coloring);
    // if old color == -1, just consider the new color add new output
    if(old_color == -1)
    {
        for (auto child : children)
        {
            vector<ECNode *> child_parents = child->getParentNodes();
            int child_color = _coloring[child->getNodeId()];
            if(child_color == -1){
                continue;
            }
            // if old_color_count = 1, old_color in --, child_color out --
            // if new_color_conut = 0, new_color in ++, child_color out ++
            int count_new_color = 0;
            for(auto child_parent : child_parents){
                if(_coloring[child_parent->getNodeId()] == new_color){
                    count_new_color++;
                }
            }
            // if child had not send chunk to ont vetex which is newcolor, then add new color will being  one chunk output
            if(count_new_color == 0 && new_color != child_color){
                loadTable[child_color][0] ++;
                loadTable[new_color][1]++;
            }
        }
    }
    else{
        cout << "debug old color != -1" << endl;
        // cin >> old_color;
        // for every child, consider it s output, 
        // if child just send one chunk to one vectex which is oldcolor, then delete old color will minus one chunk output
        // if child had not send chunk to ont vetex which is newcolor, then add new color will being  one chunk output
        for (auto child : children)
        {
            vector<ECNode *> child_parents = child->getParentNodes();
            int child_color = _coloring[child->getNodeId()];
            if(child_color == -1){
                continue;
            }
            // if old_color_count = 1, old_color in --, child_color out --
            // if new_color_conut = 0, new_color in ++, child_color out ++
            int count_old_color = 0, count_new_color = 0;
            
            // count this child`s parent vetex output color
            for(auto child_parent : child_parents){
                if(_coloring[child_parent->getNodeId()] == old_color){
                    count_old_color++;
                }
                if(_coloring[child_parent->getNodeId()] == new_color){
                    count_new_color++;
                }
            }
            
            // delete old_color will minus one chunk output 
            if(count_old_color == 1 && old_color != child_color){
                loadTable[old_color][0]--;
                loadTable[child_color][1]--;
            }
            
            // add new color will bring one chunk input
            if(count_new_color == 0 && new_color != child_color){
                loadTable[new_color][1]++;
                loadTable[child_color][0]++;
            }
        }
    }
    return loadTable;
}

int Stripe::getMinOutput(int nodeid){
    int count = 0;
    for(auto it : _ecdag->getECLeaves()){
        if(_coloring[it] == nodeid)
            count++;
    }
    return count;
}

void Stripe::evaluateColoring() {
    _load = 0;
    _bdwt = 0;
    _in.clear();
    _out.clear();
    // cout << "Stripe::evaluateColoring() coloring351 " << endl;
    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap();

    // evaluate inmap and outmap
    for (auto item: _coloring) {
        // cout << "coloring351 " << item.first << " " << item.second << endl;
        int dagidx = item.first;
        int mycolor = item.second;

        if (mycolor == -1) {
            // this is a shortening vertex, does not generate traffic
            continue;
        }

        ECNode* node = ecNodeMap[dagidx];
        vector<ECNode*> parents = node->getParentNodes();
        vector<int> parent_colors;
        for (int i=0; i<parents.size(); i++) {
            ECNode* parentnode = parents[i];
            
            int parentidx = parentnode->getNodeId();
            // cout << "pkt id : " << dagidx << " parent_idxs " << parentidx << endl;
            // int parentcolor = _coloring[parentidx];
            int parentcolor = _coloring.at(parentidx);

            if (find(parent_colors.begin(), parent_colors.end(), parentcolor) == parent_colors.end())
                parent_colors.push_back(parentcolor);
        }
        for (int i=0; i<parent_colors.size(); i++) {
            // cout << "pkt id : " << dagidx << " parent_colors " << parent_colors[i] << endl;
            if (mycolor != parent_colors[i] && parent_colors[i] != -1) {
                if (_out.find(mycolor) == _out.end())
                    _out.insert(make_pair(mycolor, 1));
                else
                    _out[mycolor]++;

                if (_in.find(parent_colors[i]) == _in.end())
                    _in.insert(make_pair(parent_colors[i], 1));
                else
                    _in[parent_colors[i]]++;
            }
        }

    }
    for (auto item: _in) {
        _bdwt += item.second;
        // cout << "item.second in " << item.second << endl;
        if (item.second > _load)
            _load = item.second;
    }

    for (auto item: _out) {
        // cout << "item.second in " << item.second << endl;
        if (item.second > _load)
            _load = item.second;
    }
    // cout << "debug 398 " << _load << " " << _bdwt << endl;
    
    // if (_load == 0)
    //     cout << "error!" << endl;ss
}



unordered_map<int, int> Stripe::getInMap() {
    return _in;
}

unordered_map<int, int> Stripe::getOutMap() {
    return _out;
}

int Stripe::getBdwt() {
    return _bdwt;
}

int Stripe::getLoad() {
    return _load;
}

void Stripe::dumpTrans()
{
    cout << "Dump Transmission" << endl;
    _load = 0;
    _bdwt = 0;
    _in.clear();
    _out.clear();
    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap();

    // evaluate inmap and outmap
    for (auto item: _coloring) {
        int dagidx = item.first;
        int mycolor = item.second;

        if (mycolor == -1) {
            // this is a shortening vertex, does not generate traffic
            continue;
        }

        ECNode* node = ecNodeMap[dagidx];
        vector<ECNode*> parents = node->getParentNodes();
        vector<int> parent_colors;
        for (int i=0; i<parents.size(); i++) {
            ECNode* parentnode = parents[i];
            int parentidx = parentnode->getNodeId();
            int parentcolor = _coloring[parentidx];

            if (find(parent_colors.begin(), parent_colors.end(), parentcolor) == parent_colors.end())
                parent_colors.push_back(parentcolor);
        }
        for (int i=0; i<parent_colors.size(); i++) {
            if (mycolor != parent_colors[i] && parent_colors[i] != -1) {
                cout << mycolor << "->" << parent_colors[i] << endl;

                if (_out.find(mycolor) == _out.end())
                    _out.insert(make_pair(mycolor, 1));
                else
                    _out[mycolor]++;
                if (_in.find(parent_colors[i]) == _in.end())
                    _in.insert(make_pair(parent_colors[i], 1));
                else
                    _in[parent_colors[i]]++;
            }
        }

    }

    for (auto item: _in) {
        _bdwt += item.second;
        if (item.second > _load)
            _load = item.second;
    }

    for (auto item: _out) {
        if (item.second > _load)
            _load = item.second;
    }
    
    // if (_load == 0)
    //     cout << "error!" << endl;ss
}

void Stripe::dumpLoad(int agents_num)
{
    cout << "Stripe " << _stripe_id << endl;
    cout << "  inmap:  ";
    for (int nodeid=0; nodeid<agents_num; nodeid++) {
        cout  << _in[nodeid] << " ";
    }
    cout << endl;

    cout << "  outmap: ";
    for (int nodeid=0; nodeid<agents_num; nodeid++) {
        cout  <<_out[nodeid] << " ";
    }
    cout << endl;

    cout << "load = " << _load << endl;
    cout << "bdwt = " << _bdwt << endl;
}


vector<vector<int>> Stripe::evalColoringGlobal(vector<vector<int>> loadTable)
{
    // return the global table after this execute coloring
    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap();
    // evaluate inmap and outmap
    for (auto item: _coloring) {
        // cout << "509 coloring " <<  item.first << " " << item.second << endl;
        int dagidx = item.first;
        int mycolor = item.second;

        if (mycolor == -1) {
            // this is a shortening vertex, does not generate traffic
            continue;
        }
        ECNode* node = ecNodeMap[dagidx];
        vector<ECNode*> parents = node->getParentNodes();

        vector<int> parent_colors;
        for (int i=0; i<parents.size(); i++) {
            ECNode* parentnode = parents[i];
            int parentidx = parentnode->getNodeId();
            int parentcolor = _coloring[parentidx];
            if (find(parent_colors.begin(), parent_colors.end(), parentcolor) == parent_colors.end()){
                parent_colors.push_back(parentcolor);
            }
                
        }
        for (int i=0; i<parent_colors.size(); i++) {
            // cout << i << " " << mycolor << " " << parent_colors[i] <<  endl;
            // cout << "table size = " << loadTable.size() << endl;
            if (mycolor != parent_colors[i] && parent_colors[i] != -1) {
                loadTable[mycolor][0]++;
                loadTable[parent_colors[i]][1]++;
            }
        }
    }

    return loadTable;
}

unordered_map<int, vector<Task*>> Stripe::genRepairTasks(int batchid, int ecn, int eck, int ecw, unordered_map<int, int> fail2repair) {

    cout << "Stripe::genRepairTasks" << endl;
    
    // 0. get leveled topological sorting of the current ecdag
    vector<vector<int>> topolevel = _ecdag->genLeveledTopologicalSorting();
    int num_level = topolevel.size();

    // 1. remap coloring with fail2repair
    for (auto item: _coloring) {
        int dagidx = item.first;
        int color = item.second;

        if (fail2repair.find(color) != fail2repair.end())
            _coloring[dagidx] = fail2repair[color];
    }

    // 1. for each level, generate corresponding tasks
    // level 0: Read task + Send task (with a Recv task in dst)
    //          For shortening dagidx, figure out the targeting physical node and Read zero
    // level 1: Compute task + Send task (with a Recv task in dst)
    // ...
    // level n: Concact task
    for (int level=0; level<topolevel.size(); level++) {
        vector<int> leaves = topolevel[level];

        //if (level > 1)
        //    break;

        if (level == 0) {
            // first level
            genLevel0(batchid, ecn, eck, ecw, leaves, _taskmap);
        } else if (level == num_level - 1) {
            // last level
            genLastLevel(batchid, ecn, eck, ecw, leaves, _taskmap);
        } else {
            // intermediate level
            genIntermediateLevels(batchid, ecn, eck, ecw, leaves, _taskmap);
        }
    }

    for (auto item: _taskmap) {
        int nodeid = item.first;
        vector<Task*> list = item.second;

        cout << "----- nodeid: " << nodeid << endl;
        for (int i=0; i<list.size(); i++) {
            cout << i << " : ";
            list[i]->dump();
        }
    }

    return _taskmap;
}

void Stripe::genLevel0(int batchid, int ecn, int eck, int ecw,
        vector<int> leaves, unordered_map<int, vector<Task*>>& res) {

    vector<int> realidx;
    vector<int> virtidx;

    // 0. figure out dagidx that are from real blocks, and virtual blocks
    for (int i=0; i<leaves.size(); i++) {
        int dagidx = leaves[i];
        int blkidx = dagidx / ecw;

        if (blkidx < ecn) {
            // this is from a real block
            realidx.push_back(dagidx);
        } else {
            // this is ZERO
            virtidx.push_back(dagidx);
        }
    }

    //cout << "Stripe::genLevel0:realidx: ";
    //for (auto item: realidx)
    //    cout << item << " ";
    //cout << endl;
    //cout << "Stripe::genLevel0:virtidx: ";
    //for (auto item: virtidx)
    //    cout << item << " ";
    //cout << endl;

    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap(); 

    // 1. deal with realidx
    // for read:
    // blkidx2readdagidxlist: blkidx 2 dagidx that are read from disk
    unordered_map<int, vector<int>> blkidx2readdagidxlist;
    // readdagidx2usage: dagidx -> <same, diff>
    unordered_map<int, pair<bool, bool>> readdagidx2usage;

    // for send
    // sendfrom2dagidxlist: nodeid -> dagidxlist that are send from this nodeid
    unordered_map<int, vector<int>> sendfrom2dagidxlist;
    // senddagidx2nodeidlist: dagidx -> sendto nodeidlist
    unordered_map<int, vector<int>> senddagidx2nodeidlist;

    // for recv
    // recvnodeid2dagidxlist: nodeid -> dagidxlist that recv in this nodeid
    unordered_map<int, vector<int>> recvnodeid2dagidxlist;

    // 1.0 prepare
    for (int i=0; i<realidx.size(); i++) {
        int dagidx = realidx[i];
        int blkidx = dagidx/ecw;

        // 1.0.0 add dagidx to blkidx2readdagidxlist
        if (blkidx2readdagidxlist.find(blkidx) == blkidx2readdagidxlist.end()) {
            vector<int> list = {dagidx};
            blkidx2readdagidxlist.insert(make_pair(blkidx, list));
        } else {
            blkidx2readdagidxlist[blkidx].push_back(dagidx);
        }

        // <same, diff>:
        //      same = true: it will be used in the same color
        //      diff = true: it will be used in a different color
        bool same=false;
        bool diff=false;

        // 1.0.1 figure out nodeidlist that dagidx will be used
        ECNode* curnode = ecNodeMap[dagidx];
        int mycolor = _coloring[dagidx];

        //cout << "Stripe::genLevel0.dagidx = " << dagidx << ", blkidx = " << blkidx << ", mycolor = " << mycolor << endl;

        vector<ECNode*> parentvertices = curnode->getParentNodes();
        vector<int> nodeidlist; // record all the parent nodeids
        for (auto pnode: parentvertices) {
            int p_dagidx = pnode->getNodeId();
            int p_color = _coloring[p_dagidx];

            if (find(nodeidlist.begin(), nodeidlist.end(), p_color) == nodeidlist.end())
                nodeidlist.push_back(p_color);
        }

        //cout << "    parent nodeid list: ";
        //for (auto p_color: nodeidlist)
        //    cout << p_color << " ";
        //cout << endl;

        // 1.0.2 figure out usage
        for (auto p_color: nodeidlist) {
            if (p_color == mycolor)
                same = true;
            else
                diff = true;

            if (same && diff)
                break;
        }
        pair<bool, bool> usage = make_pair(same, diff);
        readdagidx2usage.insert(make_pair(dagidx, usage));

        //cout << "    usage: " << same << ", " << diff << endl; 

        // 1.0.2 prepare for send
        // remove same node from nodeidlist, we do not need to send to the same nodeid
        if (find(nodeidlist.begin(), nodeidlist.end(), mycolor) != nodeidlist.end())
            nodeidlist.erase(find(nodeidlist.begin(), nodeidlist.end(), mycolor));

        if (nodeidlist.size() > 0) {
            // mark we send dagidx from mycolor
            if (sendfrom2dagidxlist.find(mycolor) == sendfrom2dagidxlist.end()) {
                vector<int> list = {dagidx};
                sendfrom2dagidxlist.insert(make_pair(mycolor, list));
            } else {
                sendfrom2dagidxlist[mycolor].push_back(dagidx);
            }
            
            // mark where we send dagidx
            senddagidx2nodeidlist.insert(make_pair(dagidx, nodeidlist));
            
            //cout << "    sendto: ";
            //for (auto p_color: nodeidlist)
            //    cout << p_color << " ";
            //cout << endl;
        }
        
        // 1.0.3 prepare for recv
        for (auto p_color: nodeidlist) {
            if (recvnodeid2dagidxlist.find(p_color) == recvnodeid2dagidxlist.end()) {
                vector<int> list = {dagidx};
                recvnodeid2dagidxlist.insert(make_pair(p_color, list));
            } else {
                recvnodeid2dagidxlist[p_color].push_back(dagidx);
            }
        }
    }

    // // debug
    // // for read:
    // cout << "READ: " << endl;
    // for (auto item: blkidx2readdagidxlist) {
    //     int blkidx = item.first;
    //     vector<int> dagidxlist = item.second;
    //     int mycolor = _coloring[dagidxlist[0]];
    //     cout << "    blk: " << blkidx << ", nodeid: " << mycolor << ", dagidxlist: ";
    //     for (auto tmpitem: dagidxlist) {
    //         pair<bool, bool> tmpusage = readdagidx2usage[tmpitem];
    //         cout << tmpitem << " (" << tmpusage.first << ", " << tmpusage.second << ") ";
    //     }
    //     cout << endl;
    // }
    // 

    // // for send
    // cout << "SEND: " << endl;
    // for (auto item: sendfrom2dagidxlist) {
    //     int sendfrom = item.first;
    //     vector<int> dagidxlist = item.second;
    //     cout << "    sendfrom: " << sendfrom << ", dagidxlist: " << endl;
    //     for (auto tmpitem: dagidxlist) {
    //         cout << "        dagidx: " << tmpitem << ", sendto: ";
    //         vector<int> sendtolist = senddagidx2nodeidlist[tmpitem];
    //         for (auto ii: sendtolist) {
    //             cout << ii << " ";
    //         }
    //         cout << endl;
    //     }
    // }

    // // for recv
    // cout << "RECV: " << endl;
    // for (auto item: recvnodeid2dagidxlist) {
    //     int recvin = item.first;
    //     vector<int> dagidxlist = item.second;
    //     cout << "recvin: " << recvin << ", dagidxlist: ";
    //     for (auto ii: dagidxlist)
    //         cout << ii << " ";
    //     cout << endl;
    // }

    // 2. generate tasks for realidx
    // 2.1 for read
    for (auto item: blkidx2readdagidxlist) {
        int blkidx = item.first;
        string blkname = _blklist[blkidx];
        vector<int> dagidxlist = item.second;
        int mycolor = _coloring[dagidxlist[0]];

        sort(dagidxlist.begin(), dagidxlist.end());
        vector<pair<bool, bool>> usagelist;
        for (int i=0; i<dagidxlist.size(); i++)
            usagelist.push_back(readdagidx2usage[dagidxlist[i]]);

        Task* readtask = new Task(batchid, _stripe_id);
        readtask->buildReadTask(0, blkname, dagidxlist, usagelist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {readtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(readtask);
        }
    }
    // 2.2 for send
    for (auto item: sendfrom2dagidxlist) {
        int mycolor = item.first;
        vector<int> dagidxlist = item.second;
        vector<vector<int>> dagidxsendtolist;
        for (int i=0; i<dagidxlist.size(); i++) {
            int dagidx = dagidxlist[i];
            vector<int> sendtolist = senddagidx2nodeidlist[dagidx];;
            dagidxsendtolist.push_back(sendtolist);
        }

        Task* sendtask = new Task(batchid, _stripe_id);
        sendtask->buildSendTask(1, dagidxlist, dagidxsendtolist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {sendtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(sendtask);
        }
    }

    // 2.3 for recv
    for (auto item: recvnodeid2dagidxlist) {
        int mycolor = item.first;
        vector<int> dagidxlist = item.second;

        Task* recvtask = new Task(batchid, _stripe_id);
        recvtask->buildRecvTask(2, dagidxlist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {recvtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(recvtask);
        }
    }

    // 3. deal with virt idx
    unordered_map<int, vector<int>> nodeid2virdagidxlist;
    for (int i=0; i<virtidx.size(); i++) {
        int dagidx = virtidx[i];
        //cout << "virt idx: " << dagidx << ", color: " << _coloring[dagidx] << endl;

        // 3.1 figure out nodeids that uses dagidx
        ECNode* curnode = ecNodeMap[dagidx];
        vector<ECNode*> parentvertices = curnode->getParentNodes();
        vector<int> nodeidlist; // record all the parent nodeids
        for (auto pnode: parentvertices) {
            int p_dagidx = pnode->getNodeId();
            int p_color = _coloring[p_dagidx];

            // // debug for -1
            // vector<ECNode*> childvertices = pnode->getChildNodes();
            // vector<int> p_child_color;
            // for (auto cnode: childvertices) {
            //     int p_c_dagidx = cnode->getNodeId();
            //     int p_c_color = _coloring[p_c_dagidx];
            //     p_child_color.push_back(p_c_color);
            // }
            // cout << "    p_color: " << p_color << ", <- ( ";
            // for (auto p_c_color: p_child_color)
            //     cout << p_c_color << " ";
            // cout << " )" << endl;

            // if p_color == -1, means that there is no need to generate it
            // we delay the generation to the next level
            if (p_color == -1)
                continue;

            if (find(nodeidlist.begin(), nodeidlist.end(), p_color) == nodeidlist.end())
                nodeidlist.push_back(p_color);
        }

        for (auto p_color: nodeidlist) {
            //cout << "    p_color = " << p_color << endl;
            if (nodeid2virdagidxlist.find(p_color) == nodeid2virdagidxlist.end()) {
                vector<int> list = {dagidx};
                nodeid2virdagidxlist.insert(make_pair(p_color, list));
            } else {
                nodeid2virdagidxlist[p_color].push_back(dagidx);
            }
        }
    }

    // cout << "READ VIRT:" << endl;
    // for (auto item: nodeid2virdagidxlist) {
    //     int nodeid = item.first;
    //     vector<int> dagidxlist = item.second;
    //     cout << "    nodeid: " << nodeid << ", dagidxlist: ";
    //     for (auto idx: dagidxlist)
    //         cout << idx << " ";
    //     cout << endl;
    // }

    // 3.2 generate read virt tasks
    for (auto item: nodeid2virdagidxlist) {
        int mycolor = item.first;
        string blkname = "ZERO";
        vector<int> dagidxlist = item.second;
        vector<pair<bool, bool>> usagelist;
        
        Task* readtask = new Task(batchid, _stripe_id);
        readtask->buildReadTask(0, blkname, dagidxlist, usagelist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {readtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(readtask);
        }
    }
}

void Stripe::genIntermediateLevels(int batchid, int ecn, int eck, int ecw, 
        vector<int> leaves, unordered_map<int, vector<Task*>>& res) {
    
    // for zero:
    vector<int> realidx;
    vector<int> zeroidx;

    // for compute:
    // nodeid2dagidxlist: nodeid -> dagidx computed in this nodeid
    unordered_map<int, vector<int>> nodeid2dagidxlist;
    // dagidx2usage: compute dagidx -> <same, diff>
    unordered_map<int, pair<bool, bool>> dagidx2usage;

    // for send
    // sendfrom2dagidxlist: nodeid -> dagidxlist that are send from this nodeid
    unordered_map<int, vector<int>> sendfrom2dagidxlist;
    // senddagidx2nodeidlist: dagidx -> sendto nodeidlist
    unordered_map<int, vector<int>> senddagidx2nodeidlist;

    // for recv
    // recvnodeid2dagidxlist: nodeid -> dagidxlist that recv in this nodeid
    unordered_map<int, vector<int>> recvnodeid2dagidxlist;

    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap();

    // 1. prepare
    // 1.0 figure out realidx and zeroidx (color with -1)
    for (auto dagidx: leaves) {
        int mycolor = _coloring[dagidx];
        if (mycolor != -1)
            realidx.push_back(dagidx);
        else
            zeroidx.push_back(dagidx);
    }
    // 1.1 prepare for realidx
    for (auto dagidx: realidx) {
        int mycolor = _coloring[dagidx];
        ECNode* curnode = ecNodeMap[dagidx];

        //cout << "dagidx: " << dagidx << ", mycolor: " << mycolor << endl;

        // 1.2 figure out dagidx that are generated in the same nodeid
        if (nodeid2dagidxlist.find(mycolor) == nodeid2dagidxlist.end()) {
            vector<int> list = {dagidx};
            nodeid2dagidxlist.insert(make_pair(mycolor, list));
        } else {
            nodeid2dagidxlist[mycolor].push_back(dagidx);
        }

        // 1.3 figure out usage for each dagidx
        // <same, diff>:
        //      same = true: it will be used in the same color
        //      diff = true: it will be used in a different color
        bool same=false;
        bool diff=false;

        vector<int> nodeidlist;
        vector<ECNode*> parentvertices = curnode->getParentNodes();
        for (auto pnode: parentvertices) {
            int p_dagidx = pnode->getNodeId();
            int p_color = _coloring[p_dagidx];

            //cout <<"    p_color: " << p_color << endl;

            if (find(nodeidlist.begin(), nodeidlist.end(), p_color) == nodeidlist.end())
                nodeidlist.push_back(p_color);
        }

        // 1.4 figure out usage
        for (auto p_color: nodeidlist) {
            if (p_color == mycolor)
                same = true;
            else
                diff = true;

            if (same && diff)
                break;
        }
        pair<bool, bool> usage = make_pair(same, diff);
        dagidx2usage.insert(make_pair(dagidx, usage));

        // 1.5 prepare for send
        // remove same node from nodeidlist, we do not need to send to the same nodeid
        if (find(nodeidlist.begin(), nodeidlist.end(), mycolor) != nodeidlist.end())
            nodeidlist.erase(find(nodeidlist.begin(), nodeidlist.end(), mycolor));

        if (nodeidlist.size() > 0) {
            // mark we send dagidx from mycolor
            if (sendfrom2dagidxlist.find(mycolor) == sendfrom2dagidxlist.end()) {
                vector<int> list = {dagidx};
                sendfrom2dagidxlist.insert(make_pair(mycolor, list));
            } else {
                sendfrom2dagidxlist[mycolor].push_back(dagidx);
            }
            
            // mark where we send dagidx
            senddagidx2nodeidlist.insert(make_pair(dagidx, nodeidlist));
            
            //cout << "    sendto: ";
            //for (auto p_color: nodeidlist)
            //    cout << p_color << " ";
            //cout << endl;
        }

        // 1.6 prepare for recv
        for (auto p_color: nodeidlist) {
            if (recvnodeid2dagidxlist.find(p_color) == recvnodeid2dagidxlist.end()) {
                vector<int> list = {dagidx};
                recvnodeid2dagidxlist.insert(make_pair(p_color, list));
            } else {
                recvnodeid2dagidxlist[p_color].push_back(dagidx);
            }
        }
    }

    // 2. generate tasks for realidx 
    // 2.1 for compute
    for (auto item: nodeid2dagidxlist) {
        int nodeid = item.first;
        vector<int> dagidxlist = item.second;

        vector<ComputeItem*> clist;
        for (auto dagidx: dagidxlist) {
            ECNode* curnode = ecNodeMap[dagidx];
            vector<int> srclist = curnode->getChildIndices();
            vector<int> coefs = curnode->getCoefs();

            pair<bool, bool> usage = dagidx2usage[dagidx];

            ComputeItem* ci = new ComputeItem(dagidx, srclist, coefs, usage);
            clist.push_back(ci);
        }

        Task* computetask = new Task(batchid, _stripe_id);
        computetask->buildComputeTask(3, clist);

        if (res.find(nodeid) == res.end()) {
            vector<Task*> list = {computetask};
            res.insert(make_pair(nodeid, list));
        } else 
            res[nodeid].push_back(computetask);
    }
    // 2.2 for send
    for (auto item: sendfrom2dagidxlist) {
        int mycolor = item.first;
        vector<int> dagidxlist = item.second;
        vector<vector<int>> dagidxsendtolist;
        for (int i=0; i<dagidxlist.size(); i++) {
            int dagidx = dagidxlist[i];
            vector<int> sendtolist = senddagidx2nodeidlist[dagidx];;
            dagidxsendtolist.push_back(sendtolist);
        }

        Task* sendtask = new Task(batchid, _stripe_id);
        sendtask->buildSendTask(1, dagidxlist, dagidxsendtolist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {sendtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(sendtask);
        }
    }
    // 2.3 for recv
    for (auto item: recvnodeid2dagidxlist) {
        int mycolor = item.first;
        vector<int> dagidxlist = item.second;

        Task* recvtask = new Task(batchid, _stripe_id);
        recvtask->buildRecvTask(2, dagidxlist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {recvtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(recvtask);
        }
    }

    // 3. deal with zero idx
    unordered_map<int, vector<int>> nodeid2zerodagidxlist;
    for (int i=0; i<zeroidx.size(); i++) {
        int dagidx = zeroidx[i];
        //cout << "zero idx: " << dagidx << ", color: " << _coloring[dagidx] << endl;

        // 3.1 figure out nodeids that uses dagidx
        ECNode* curnode = ecNodeMap[dagidx];
        vector<ECNode*> parentvertices = curnode->getParentNodes();
        vector<int> nodeidlist; // record all the parent nodeids
        for (auto pnode: parentvertices) {
            int p_dagidx = pnode->getNodeId();
            int p_color = _coloring[p_dagidx];

            // // debug for -1
            // vector<ECNode*> childvertices = pnode->getChildNodes();
            // vector<int> p_child_color;
            // for (auto cnode: childvertices) {
            //     int p_c_dagidx = cnode->getNodeId();
            //     int p_c_color = _coloring[p_c_dagidx];
            //     p_child_color.push_back(p_c_color);
            // }
            // cout << "    p_color: " << p_color << ", <- ( ";
            // for (auto p_c_color: p_child_color)
            //     cout << p_c_color << " ";
            // cout << " )" << endl;

            // if p_color == -1, means that there is no need to generate it
            // we delay the generation to the next level
            if (p_color == -1)
                continue;

            if (find(nodeidlist.begin(), nodeidlist.end(), p_color) == nodeidlist.end())
                nodeidlist.push_back(p_color);
        }

        for (auto p_color: nodeidlist) {
            // cout << "    p_color = " << p_color << endl;
            if (nodeid2zerodagidxlist.find(p_color) == nodeid2zerodagidxlist.end()) {
                vector<int> list = {dagidx};
                nodeid2zerodagidxlist.insert(make_pair(p_color, list));
            } else {
                nodeid2zerodagidxlist[p_color].push_back(dagidx);
            }
        }
    }

    // cout << "READ VIRT:" << endl;
    // for (auto item: nodeid2zerodagidxlist) {
    //     int nodeid = item.first;
    //     vector<int> dagidxlist = item.second;
    //     cout << "    nodeid: " << nodeid << ", dagidxlist: ";
    //     for (auto idx: dagidxlist)
    //         cout << idx << " ";
    //     cout << endl;
    // }

    // 3.2 generate read virt tasks
    for (auto item: nodeid2zerodagidxlist) {
        int mycolor = item.first;
        string blkname = "ZERO";
        vector<int> dagidxlist = item.second;
        vector<pair<bool, bool>> usagelist;
        
        Task* readtask = new Task(batchid, _stripe_id);
        readtask->buildReadTask(0, blkname, dagidxlist, usagelist);

        if (res.find(mycolor) == res.end()) {
            vector<Task*> list = {readtask};
            res.insert(make_pair(mycolor, list));
        } else {
            res[mycolor].push_back(readtask);
        }
    }
}

void Stripe::genLastLevel(int batchid, int ecn, int eck, int ecw, 
        vector<int> leaves, unordered_map<int, vector<Task*>>& res) {

    unordered_map<int, ECNode*> ecNodeMap = _ecdag->getECNodeMap();

    // now we only deal with single failure
    assert(leaves.size() == 1);

    string repair_blockname = _blklist[_fail_blk_idx];

    int dagidx = leaves[0];
    int color = _coloring[dagidx];

    ECNode* curnode = ecNodeMap[dagidx];
    vector<int> clist = curnode->getChildIndices();

    Task* persisttask = new Task(batchid, _stripe_id);
    persisttask->buildPersistTask(4, clist, repair_blockname);

    if (res.find(color) == res.end()) {
        vector<Task*> list = {persisttask};
        res.insert(make_pair(color, list));
    } else
        res[color].push_back(persisttask);
}

int Stripe::getTaskNumForNodeId(int nodeid) {
    int toret = 0;

    if (_taskmap.find(nodeid) != _taskmap.end()) 
        toret = _taskmap[nodeid].size();

    return toret;
}

vector<Task*> Stripe::getTaskForNodeId(int nodeid) {
     
    vector<Task*> toret;

    if (_taskmap.find(nodeid) != _taskmap.end()) 
        toret = _taskmap[nodeid];

    return toret;
}
vector<int> Stripe::getsolution(){
    vector<pair<int,int>> vec (_coloring.begin(), _coloring.end());
    sort(vec.begin(), vec.end());
    vector<int> ret;
    for(auto it : vec){
        ret.push_back(it.second);
    }
    return ret;
}

int Stripe::getBlockIdxByName(string blkname) {
    int toret = -1;
    for (int i=0; i<_blklist.size(); i++) {
        string blk = _blklist[i];
        if (blkname == blk) {
            toret = i;
            break;
        }
    }
    return toret;
}

int Stripe::getNodeIdByBlock(string blkname) {
    int toret = -1;
    for (int i=0; i<_blklist.size(); i++) {
        string blk = _blklist[i];
        int nodeid = _nodelist[i];
        if (blkname == blk) {
            toret = nodeid;
            break;
        }
    }
    return toret;
}

void Stripe::setBandwidth(Bandwidth* bdwt) {
    _bandwidth = bdwt;
}

void Stripe::evaluateColorLoad(vector<int> idxs, vector<int>* Load, int newColor){

    ECNode *node = _ecdag->getECNodeMap()[idxs[0]];
    int old_color = _coloring[node->getNodeId()];

    vector<ECNode*> children = node->getChildNodes();
    vector<int> childColors = node->getChildColors(_coloring);
    if (old_color == -1) {
        for (auto child : children) {
            int child_color = _coloring[child->getNodeId()];
            // cout << "child_color: " << child_color << endl;
            if (child_color == newColor) (*Load)[0]--;
            else if (child_color != -1) (*Load)[1]++;
        }
    }
    
    for (auto idx : idxs) {
        ECNode *node = _ecdag->getECNodeMap()[idx];
        vector<ECNode*> parents = node->getParentNodes();
        set<int> parent_colors;
        for (auto parent : parents) parent_colors.insert(_coloring[parent->getNodeId()]);

        for (auto parent_color : parent_colors) {
            if (parent_color != newColor) {
                (*Load)[0]++;
            } if (parent_color == newColor) {
                (*Load)[1]--;
            }
        }
    }

}

void Stripe::dumpBottleneck() {
    double bottleneck = DBL_MAX;
    for (int i = 0; i < _in.size(); i++) {
        double newbottleneck = _bandwidth->getBottleneck(i, vector<int>{_out[i], _in[i]});
        if (newbottleneck < bottleneck) bottleneck = newbottleneck;
    }
    cout << "bottleneck: " << bottleneck << endl;
}

double Stripe::getBottleneck() {
    double bottleneck = DBL_MAX;
    for (int i = 0; i < _in.size(); i++) {
        double newbottleneck = _bandwidth->getBottleneck(i, vector<int>{_out[i], _in[i]});
        if (newbottleneck < bottleneck) bottleneck = newbottleneck;
    }
    return bottleneck;
}


void Stripe::evaluateBottleneck(int& bottlenode , int& port, double& bottleneck) {
    double curbottleneck = DBL_MAX;
    for (int i = 0; i < _in.size(); i++) {
        double newbottleneck = _bandwidth->getBottleneck(i, vector<int>{_out[i], _in[i]});
        int newport = _bandwidth->getBottlePort(i, vector<int>{_out[i], _in[i]});
        // cout << "out : " << _out[i] << "in : " << _in[i] << endl;
        if (newbottleneck < curbottleneck) {
            curbottleneck = newbottleneck;
            bottlenode = i;
            port = newport;
        }
    }
    bottleneck = curbottleneck;
}

int Stripe::evaluateIdleColor(const vector<int>& idleColors) {
    int res = -1;
    double bottleneck = 0;
    for (int icolor : idleColors) {
        double newbottleneck = _bandwidth->evaluateSort(icolor, vector<int>{_in[icolor], _out[icolor]});
        if (newbottleneck > bottleneck) {
            bottleneck = newbottleneck;
            res = icolor;
        }
    }
    return res;
}



ECDAG* Stripe::genRepairECDAG(ECBase* ec, vector<int> failidx) {
    // LOG << "Stripe::genRepairECDAG start " << getStripeId()<< endl;
    // LOG << "node  list: ";
    // for(auto it : _nodelist){
    //     LOG << " " << it;
    // }
    // LOG << endl;
    
    vector<int> from;
    vector<int> to;
    int ecw = ec->_w;                                                                                                                                   
    int ecn = ec->_n;

    _fail_blk_idxs.clear();
    // blkidx refers to the idx of a block in this stripe
    for (int blkidx=0; blkidx < ecn; blkidx++){
        int curnodeid = _nodelist[blkidx];
        if(find(failidx.begin(),failidx.end(),curnodeid)!=failidx.end()){
            // if this node fails, we need to repair this block
            // offset refers the offset of sub-packets
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                to.push_back(pktidx);
            }
            _fail_blk_idxs.push_back(blkidx);
        } else {
            for (int offset=0; offset < ecw; offset++) {
                int pktidx = blkidx * ecw + offset;
                from.push_back(pktidx);
            }
        }
    }
    // LOG << "DEBUG TO " << endl;
    // for(auto it : to){
    //     LOG << " " << it;
    // }
    // LOG << endl;
    _ecdag = ec->Decode(from, to);//decode
    _ecdag->Concact(to, ecn, ecw);
    
    vector<int>  headers_ec = _ecdag->getECHeaders();
    // cout << "_fail_blk_idx " << _fail_blk_idx << endl;
    //剪枝

    for (auto req : headers_ec){
        if (req == REQUESTOR -  _fail_blk_idx) continue;
        _ecdag->getECNodeMap()[req]->removeChildNode();
        _ecdag->removeNode(req);
    }
    _ecdag->DeleteUseless();
    if(ECDAG_DEBUG_ENABLE)
        LOG << "Stripe::genRepairECDAG end" << endl;
    return _ecdag;
}


ECDAG* Stripe::genMutiRepairECDAG(ECBase* ec, string blkname, string codename,  int repair_node_id) {
    vector<int> from;
    vector<int> to;

    int ecw = ec->_w;                                                                                                                                   
    int ecn = ec->_n;
    int eck = ec->_k;

    // blkidx refers to the idx of a block in this stripe
    for (int blkidx=0; blkidx<ecn; blkidx++){
        string curblk = _blklist[blkidx];
        if (curblk == blkname) {
            _fail_blk_idx = blkidx;
        }
    }
    vector<int> failnodeidxs;
    vector<int> igb_idx = _bandwidth->sortByUp(_nodelist);
    vector<ECDAG*> selectECDAG;
    vector<double> limiteds;
    failnodeidxs.push_back(_nodelist[_fail_blk_idx]);
    selectECDAG.push_back(genRepairECDAG(ec, failnodeidxs));
    double limitedbn = preEvaluate(repair_node_id, ecn, ecw, selectECDAG[0]);
    limiteds.push_back(limitedbn);
    if(1 == 1) {
        cout << "limitedbn = " << limitedbn << endl;
        // for (int nodeid : igb_idx){
        //     cout << "igb_idx " << " , blockid " << nodeid << " , nodeid " << _nodelist[nodeid] << endl;
        // }
    }
    int ii = 0;
    int len = 1;
    while(len < ecn-eck && codename=="Clay") {
        if (igb_idx[ii] == _fail_blk_idx) {
            ii++;
            continue;
        }
        failnodeidxs.push_back(_nodelist[igb_idx[ii]]);

        selectECDAG.push_back(genRepairECDAG(ec, failnodeidxs));
        limitedbn = preEvaluate(repair_node_id, ecn, ecw, selectECDAG[selectECDAG.size()-1]);
        if(1 == 1) {
            for (int nodeid : failnodeidxs){
                cout << "failidxs " << nodeid << endl;
            }
            cout << "limitedbn = " << limitedbn << endl;
        }
        limiteds.push_back(limitedbn);
        ii++;
        len++;
    }

    int selectidx = 0;
    limitedbn = 0;
    for (int i = 0; i < limiteds.size(); i++) 
    {
        // cout << "selectidx : " << selectidx << " , limiteds[i] : " << limiteds[i] << " , limitedbn : " << limitedbn << endl;
        if (limiteds[i] - limitedbn > 0.1 * limiteds[i]) {
            
            selectidx = i;
            limitedbn = limiteds[i];
        }
    }

    cout << "selectidx : " << selectidx << endl;
    for (int i = 0; i <= selectidx; i++){
        int nodeid = failnodeidxs[i];
        cout << "failidxs " << nodeid << endl;
    }
    _ecdag = selectECDAG[selectidx];
    return _ecdag;
}

double Stripe::preEvaluate(int repair_node_id, int ecn, 
        int ecw, ECDAG* selectECDAG) {
    vector<vector<int>> interLoadTable;
    for (int i = 0; i < ecn; i++) interLoadTable.push_back(vector<int>(2, 0));
    vector<int> ecLeaves = selectECDAG->getECLeaves();
    int realLeaves = 0;
    for (auto dagidx: ecLeaves) {
        int blkidx = dagidx / ecw;
        // cout << "blkidx : " << blkidx << ", dagidx : " << dagidx << endl;
        int nodeid = -1;

        if (blkidx < ecn) {
            nodeid = blkidx;
            realLeaves++;
            interLoadTable[nodeid][0]++;
        }
    }
    double requestorNeck = _bandwidth->getBottleneck(repair_node_id, vector<int>{0, ecw});
    vector<vector<int>> table = interLoadTable;
    if (1==1) {
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
    double limitedbn = requestorNeck;
    for (int i = 0; i < ecn; i++) {
        double newbottleneck = _bandwidth->getBottleneck(_nodelist[i], vector<int>{interLoadTable[i][0], interLoadTable[i][1]});
        // cout << "newbottleneck : " << newbottleneck << endl;
        if (newbottleneck < limitedbn) limitedbn = newbottleneck;
    }
    return limitedbn;
}