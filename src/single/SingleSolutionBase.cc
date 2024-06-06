#include "SingleSolutionBase.hh"

SingleSolutionBase::SingleSolutionBase() {

}


SingleSolutionBase::SingleSolutionBase(string param) {

}

void SingleSolutionBase::init(Stripe* stripe, ECBase* ec, string codename, Config* conf) {
    _stripe = stripe;
    _ec = ec;
    _codename = codename;
    _conf = conf;
}

void SingleSolutionBase::genRepairTasks(int ecn, int eck, int ecw, Config* conf, unordered_map<int, int> fail2repair, unsigned int coorIp) {

    struct timeval time1, time2, time3, time4, time5, time6;
    gettimeofday(&time1, NULL);

    // 0. generate tasks for each stripe
    unordered_map<int, vector<int>> nodeid2internalidx;

    int _batch_id = 0;
    unordered_map<int, vector<Task*>> curtaskmap = _stripe->genRepairTasks(_batch_id, ecn, eck, ecw, fail2repair);

    // mark that nodeid needs to deal with tasks for the i-th stripe in this batch
    int i=0; // stripeid = 0;
    for (auto item: curtaskmap) {
        int nodeid = item.first;
        if (nodeid2internalidx.find(nodeid) == nodeid2internalidx.end()) {
            vector<int> list = {i};
            nodeid2internalidx.insert(make_pair(nodeid, list));
        } else
            nodeid2internalidx[nodeid].push_back(i); 
    }
    gettimeofday(&time2, NULL);

    // 1. for each node, send command to agent to tell the number of stripes that the agent need to deal with
    unordered_map<int, NodeBatchTask*> batchtaskmap;
    for (auto item: nodeid2internalidx) {
        int nodeid = item.first;
        vector<int> internalidx = item.second;

        // 1.0 nodeid needs to deal with tasks from 1 stripes
        int nstripes = internalidx.size();
        vector<int> stripelist;
        vector<int> numlist;
        unordered_map<int, vector<Task*>> stripetaskmap;

        assert (internalidx.size() == 1);

        int tasknum = _stripe->getTaskNumForNodeId(nodeid);
        vector<Task*> tasklist = _stripe->getTaskForNodeId(nodeid);

        int stripeid = 0;
        stripelist.push_back(stripeid);
        numlist.push_back(tasknum);
        stripetaskmap.insert(make_pair(stripeid, tasklist));

        // 1.1 get ip for nodeid
        unsigned int ip;
        if (nodeid < conf->_agentsIPs.size()) {
            ip = conf->_agentsIPs[nodeid];
        } else {
            int idx = nodeid - conf->_agentsIPs.size();
            ip = conf->_repairIPs[idx];
        }

        // 1.2 prepare NodeBatchTask
        NodeBatchTask* nbtask = new NodeBatchTask(0, stripelist, numlist, stripetaskmap, ip);

        batchtaskmap.insert(make_pair(nodeid, nbtask));
    }

    gettimeofday(&time3, NULL);
    int threadnum = batchtaskmap.size(); 
    thread thrds[threadnum];
    int tid=0;
    for (auto item: batchtaskmap) {
        int nodeid = item.first;   
        NodeBatchTask* nbtask = item.second; 
        thrds[tid++] = thread([=]{nbtask->sendCommands();});
    } 

    for (int i=0; i<tid; i++) {
        thrds[i].join();
    }

    gettimeofday(&time4, NULL);

    // 2. for each node, we wait for a response that make sure corresponding node finishes assigned tasks for this batch
    tid=0;
    for (auto item: nodeid2internalidx) {
        int nodeid = item.first;
        NodeBatchTask* nbtask = batchtaskmap[nodeid];
        thrds[tid++] = thread([=]{nbtask->waitFinishFlag(nodeid, coorIp);});
    }
    for (int i=0; i<tid; i++) {
        thrds[i].join();
    }
    gettimeofday(&time5, NULL);

    // 3. clean
    for (auto item: batchtaskmap) {
        NodeBatchTask* nbtask = item.second;
        if (nbtask)
            delete nbtask;
    }
    gettimeofday(&time6, NULL);
    LOG << "RepairBatch::genRepairTasks.time: " << DistUtil::duration(time1, time6) << endl;
}
