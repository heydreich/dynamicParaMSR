#include "common/Config.hh"
#include "common/Stripe.hh"
//#include "common/StripeMeta.hh"
//#include "ec/ECDAG.hh"
//#include "common/StripeStore.hh"
//#include "protocol/AGCommand.hh"
//#include "util/DistUtil.hh"
//
#include "ec/Clay.hh"
#include "ec/BUTTERFLY.hh"

#include "single/Solution.hh"
#include "single/PRSolution.hh"

using namespace std;

//#define CLUSTERSIZE 32768

void usage() {
  cout << "Usage: ./Naive_homo" << endl;
  cout << "    1. code" << endl;
  cout << "    2. n" << endl;
  cout << "    3. k" << endl;
  cout << "    4. w" << endl;
  cout << "    5. failNodeId" << endl;
}  

bool dominate(PRSolution* a, PRSolution* b) {
    int bdwt_a = a->getBdwt();
    int load_a = a->getLoad();
    int bdwt_b = b->getBdwt();
    int load_b = b->getLoad();

    if (load_a < load_b) {
        if (bdwt_a <= bdwt_b) {
            return true;
        } else {
            return false;
        }
    } else if (load_a > load_b) {
        return false;
    } else if (load_a == load_b) {
        if (bdwt_a < bdwt_b) {
            return true;
        } else 
            return false;
    }
}

//void stat(unordered_map<int, int> sidx2ip, vector<int> curres, vector<int> itm_idx, ECDAG* ecdag, int* bdwt, int* maxload) {
//    
//    struct timeval time1, time2;
//    gettimeofday(&time1, NULL);
//    
//    unordered_map<int, int> coloring_res;
//    for (auto item: sidx2ip) {
//        coloring_res.insert(make_pair(item.first, item.second));
//    }
//    for (int ii=0; ii<curres.size(); ii++) {
//        int idx = itm_idx[ii];
//        int color = curres[ii];
//        coloring_res[idx] = color;
//    }
//    //cout << "coloring_res.size: " << coloring_res.size() << endl;
//    //
//    //cout << "current coloring:  " << endl;
//    //for (auto item: coloring_res) {
//    //    cout << "  " << item.first << ", " << item.second << endl;
//    //}
//
//    // gen ECClusters
//    ecdag->clearECCluster();
//    ecdag->genECCluster(coloring_res, CLUSTERSIZE);
//    
//    // gen stat
//    unordered_map<int, int> inmap;
//    unordered_map<int, int> outmap;
//    ecdag->genStat(coloring_res, inmap, outmap);
//
//    int bw=0, max=0;
//    for (auto item: inmap) {
//        bw+= item.second;
//        if (item.second > max)
//            max = item.second;
//    }
//    for (auto item: outmap) {
//        if (item.second > max)
//            max = item.second;
//    }
//    
//    *bdwt = bw;
//    *maxload = max;
//    
//    gettimeofday(&time2, NULL);
//    double latency = DistUtil::duration(time1, time2);
//}

void evaluateColoring(Stripe* stripe, vector<int> curres, unordered_map<int ,int> coloring, vector<int> itm_idx) {
    unordered_map<int, int> coloring_res;
    for (auto item: coloring) {
        coloring_res.insert(make_pair(item.first, item.second));
    }
    for (int ii=0; ii<curres.size(); ii++) {
        int idx = itm_idx[ii];
        int color = curres[ii];
        coloring_res[idx] = color;
    }
    stripe->setColoring(coloring_res);

    // evaluate coloring
    stripe->evaluateColoring();
}

void genPareto(unordered_map<long long, long long>& pareto_curve,
        unordered_map<long long, PRSolution*>& sol_map,
        vector<int> itm_idx, vector<int> candidates,
        Stripe* stripe, unordered_map<int ,int> coloring) {

    // 0. prepare an unsearched pool
    unordered_map<long long, PRSolution*> unsearched;

    // 1. generate a solution with random colors
    int SOL_ID = 0;
    PRSolution* init_sol = new PRSolution(SOL_ID++, itm_idx.size(), candidates.size());
    vector<int> curres = init_sol->getSolution();
    evaluateColoring(stripe, curres, coloring, itm_idx);
    int init_load = stripe->getLoad();
    int init_bdwt = stripe->getBdwt();

    init_sol->setLoad(init_load);
    init_sol->setBdwt(init_bdwt);
    cout << "initsol: load = " << init_sol->getLoad() << ", bdwt = " << init_sol->getBdwt() << endl;

    // 1.2 add the solution into the pareto curve, which start with -1, and end
    // with -2
    pareto_curve.insert(make_pair(-1, init_sol->getId()));
    pareto_curve.insert(make_pair(init_sol->getId(), -2));
    sol_map.insert(make_pair(init_sol->getId(), init_sol));

    // 1.3 we also create a prev_curve, which start with -2, and end with -1
    unordered_map<long long, long long> prev_curve;
    prev_curve.insert(make_pair(-2, init_sol->getId()));
    prev_curve.insert(make_pair(init_sol->getId(), -1));

    // 1.4 add the solution into the unsearched pool
    unsearched.insert(make_pair(init_sol->getId(), init_sol));

    // 2. each time we get an unsearched solution from unsearched pool
    while(unsearched.size() > 0) {
        //cout << "=============================================================" << endl;
        //cout << "unsearched: " << unsearched.size() << endl;
        // 2.1 get an unsearched solution from the unsearched pool
        long long sol_id = -1;
        PRSolution* sol;
        // there is a priority, we first find unsearched solution that are
        // already in the pareto curve
        bool find = false;
        while (pareto_curve[sol_id] != -2) {
            sol_id = pareto_curve[sol_id];
            sol = sol_map[sol_id];
            
            if (sol->getSearched())
                continue;
            else {
                find = true;
                break;
            }
        }
        if (!find) {
            for (auto item: unsearched) {
                sol_id = item.first;
                sol = item.second;
                break;
            }
        }

        long long head_id = pareto_curve[-1];
        PRSolution* head_sol = sol_map[head_id];

        cout << "search for sol id: " << sol_id << ", load: " << sol->getLoad() << ", bdwt: " << sol->getBdwt() << ", headload: " << head_sol->getLoad() << ", headbdwt: " << head_sol->getBdwt() << endl;

        vector<int> itm_color = sol->getSolution();
        int v = itm_idx.size();
        int m = candidates.size();

        // 2.2 enumerate all the neighbors
        unordered_map<long long, PRSolution*> neighbors;
        for (int i=0; i<v; i++) {
            int old_color = itm_color[i];
            for (int j=0; j<m; j++) {
                int new_color = j;

                if (new_color == old_color)
                    continue;

                // exchange the old_color to the new_color
                itm_color[i] = new_color;

                // generate a neighbor solution
                PRSolution* neighbor_sol = new PRSolution(SOL_ID++, itm_idx.size(), candidates.size(), itm_color);
                vector<int> curres = neighbor_sol->getSolution();
                evaluateColoring(stripe, curres, coloring, itm_idx);
                int init_load = stripe->getLoad();
                int init_bdwt = stripe->getBdwt();

                neighbor_sol->setLoad(init_load);
                neighbor_sol->setBdwt(init_bdwt);

                neighbors.insert(make_pair(neighbor_sol->getId(), neighbor_sol));
            }

            // revert the color back to old_color
            itm_color[i] = old_color;
        }
        //cout << "neighbors.size = " << neighbors.size() << endl;

        // 2.3 set sol as searched and remove from unsearched
        sol->setSearched();
        unsearched.erase(unsearched.find(sol_id));

        // 2.4 pruning neighbors with pareto curve
        // we compare each solution in neighbors with pareto curve

        // we prepare a dropped data structure to record the solution to be
        // dropped, including solutions in pareto curve, and solutions we
        // generate
        vector<long long> dropped;
        int debugnum = 0;
        //cout << "-------------------------------" << endl;
        for (auto item: neighbors) {
            long long alpha_id = item.first;
            PRSolution* alpha_sol = item.second;
            //cout << "alpha_id: " << alpha_id << ", load: " << alpha_sol->getLoad() << ", bdwt: " << alpha_sol->getBdwt() << ", pareto size: " << pareto_curve.size() << endl;


            long long beta_id = -1;
            long long prev_id, next_id = pareto_curve[beta_id];

            while (next_id != -2) {

                beta_id = next_id;
                next_id = pareto_curve[beta_id];
                prev_id = prev_curve[beta_id];

                //cout << "  beta_id = " << beta_id <<", prev_id = " << prev_id << ", next_id = " << next_id << endl;
                PRSolution* beta_sol = sol_map[beta_id];
                //cout << "    beta load: " << beta_sol->getLoad() << ", bdwt: " << beta_sol->getBdwt() << " ";

                if (dominate(beta_sol, alpha_sol)) {
                    // existing solution beta in the pareto curve dominates the
                    // generated neighbor alpha, we drop the neighbor
                    //cout << "    case 1: beta dominate alpha, drop alpha" << endl;
                    dropped.push_back(alpha_id);
                    // there is no need to compare alpha with more beta in
                    // pareto curve
                    break;
                } else if (dominate(alpha_sol, beta_sol)) {
                    // the generated neighbor alpha dominate the solution beta
                    // in the pareto curve
                    //cout << "    case 2: alpha dominate beta, update pareto curve" << endl;
                    
                    // deal with prev, beta, and alpha
                    if (prev_curve.find(alpha_id) == prev_curve.end()) {
                        pareto_curve[prev_id] = alpha_id;
                        prev_curve.insert(make_pair(alpha_id, prev_id));
                        if (sol_map.find(alpha_id) == sol_map.end())
                            sol_map.insert(make_pair(alpha_id, alpha_sol));
                    }

                    // deal with alpha, next
                    pareto_curve[alpha_id] = next_id;
                    prev_curve[next_id] = alpha_id;

                    // clear metadata for beta
                    pareto_curve.erase(pareto_curve.find(beta_id));
                    prev_curve.erase(prev_curve.find(beta_id));
                    dropped.push_back(beta_id);
                } else if (alpha_sol->getLoad() < beta_sol->getLoad()) {
                    if (prev_curve.find(alpha_id) == prev_curve.end()) {
                        //cout << "    case 3: alpha is exactly on the left of beta" << endl;
                        pareto_curve[prev_id] = alpha_id;
                        prev_curve[alpha_id] = prev_id;
                        pareto_curve[alpha_id] = beta_id;
                        prev_curve[beta_id] = alpha_id;
                        if (sol_map.find(alpha_id) == sol_map.end())
                            sol_map.insert(make_pair(alpha_id, alpha_sol));
                    }
                } else if (alpha_sol->getLoad() > beta_sol->getLoad()) {
                    if (next_id == -2) {
                        //cout << "    case 4: alpha is exactly before the tail" << endl;
                        pareto_curve[beta_id] = alpha_id;
                        prev_curve[alpha_id] = beta_id;
                        pareto_curve[alpha_id] = -2;
                        prev_curve[-2] = alpha_id;
                        if (sol_map.find(alpha_id) == sol_map.end()) {
                            sol_map.insert(make_pair(alpha_id, alpha_sol));
                        }
                    }
                } else {
                    dropped.push_back(alpha_id);
                    break;
                }
            }

            debugnum++;
            //cout << "checked " << debugnum << " neighbors" << endl;
        }

        // 2.5 remove dropped solutions
        // Note that what we add into sol_map includes sol that we add, 
        // and sol that we first add and later be removed
        for (int i=0; i<dropped.size(); i++) {
            long long drop_idx = dropped[i];
            PRSolution* sol;

            //// first check whether drop_idx is in unsearched
            //if (unsearched.find(drop_idx) != unsearched.end())
            //    continue;

            bool erased = false;
            if (neighbors.find(drop_idx) != neighbors.end()) {
                // the generated neighbor should be dropped
                sol = neighbors[drop_idx];
                neighbors.erase(neighbors.find(drop_idx));
                delete sol;
                erased = true;
            }

            if (sol_map.find(drop_idx) != sol_map.end()) {
                // the one in pareto curve should be dropped
                sol = sol_map[drop_idx];
                sol_map.erase(sol_map.find(drop_idx));
                if (!erased) {
                    delete sol;
                    erased = true;
                }
            }

            if (unsearched.find(drop_idx) != unsearched.end()) {
                unsearched.erase(unsearched.find(drop_idx));
            }
        }

        // 2.6 add remaining solutions in neighbors into unsearched pool
        //cout << "Add " << neighbors.size() << " into unsearched pool" << endl;
        for (auto item: neighbors) {
            long long add_idx = item.first;
            PRSolution* add_sol = item.second;
            unsearched.insert(make_pair(add_idx, add_sol));

            if (sol_map.find(add_idx) == sol_map.end()) {
                sol_map.insert(make_pair(add_idx, add_sol));
            }
        }
        cout << "unsearched pool size: " << unsearched.size() << ", pareto size: " << pareto_curve.size() << endl;
    }
}

int main(int argc, char** argv) {

    if (argc != 6) {
        usage();
        return 0;
    }
    
    string code = argv[1];
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int w = atoi(argv[4]);
    int failNodeId = atoi(argv[5]);

    // 0. prepare the block placement for a block
    vector<int> nodeidlist;
    for (int i=0; i<n; i++)
        nodeidlist.push_back(i);
       
    int stripeid = 0; 

    // 1. generate stripe
    Stripe* stripe = new Stripe(stripeid, nodeidlist);

    // 2. generate ec
    ECBase *ec;
    vector<string> param;
    if (code == "Clay") {
        ec = new Clay(n, k, w, {to_string(n-1)});
    } else if (code == "Butterfly") {
        ec = new BUTTERFLY(n, k, w, param);
    } else {
        cout << "wrong ec id!" << endl;
        return -1;
    }
    w = ec->_w;

    // 3. generate ecdag
    ECDAG* ecdag = stripe->genRepairECDAG(ec, failNodeId); 

    // 4. get data structures from ecdag
    unordered_map<int, ECNode*> ecNodeMap = ecdag->getECNodeMap();
    vector<int> ecHeaders = ecdag->getECHeaders();
    vector<int> ecLeaves = ecdag->getECLeaves();
    
    cout << "Total nodes: " << ecNodeMap.size() << endl;
    cout << "Header nodes: " << ecHeaders.size() << endl;
    cout << "Leaf nodes: " << ecLeaves.size() << endl;

    int intermediate_num = ecNodeMap.size() - ecHeaders.size() - ecLeaves.size();
    cout << "Intermediate nodes: " << intermediate_num << endl;

    // 5. generate colors for leaves
    unordered_map<int, int> coloring;
    int realLeaves = 0;
    vector<int> avoid_node_ids;
    for (auto dagidx: ecLeaves) {
        int blkidx = dagidx / w;
        int nodeid = -1;
        if (blkidx < n) {
            nodeid = nodeidlist[blkidx];
            avoid_node_ids.push_back(nodeid);
            realLeaves++;
        } 
        coloring.insert(make_pair(dagidx, nodeid)); 
    }
    avoid_node_ids.push_back(failNodeId);

    // 6. generate colors for header
    coloring.insert(make_pair(ecHeaders[0], n));

    // 7. now we try to color the intermediate node
    vector<int> itm_idx;
    vector<int> candidates;
    for (auto item: ecNodeMap) {
        int sidx = item.first;
        if (find(ecHeaders.begin(), ecHeaders.end(), sidx) != ecHeaders.end())
            continue;
        if (find(ecLeaves.begin(), ecLeaves.end(), sidx) != ecLeaves.end())
            continue;
        itm_idx.push_back(sidx);
        coloring.insert(make_pair(sidx, -1));
    }
    sort(itm_idx.begin(), itm_idx.end());

    for (int i=0; i<=n; i++) {
        if (i == failNodeId)
            continue;
        candidates.push_back(i);
    }

    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    unordered_map<long long, long long> pareto_curve;
    unordered_map<long long, PRSolution*> sol_map;
    genPareto(pareto_curve, sol_map, itm_idx, candidates, stripe, coloring);
  gettimeofday(&time2, NULL);
  double latency = DistUtil::duration(time1, time2);
  cout << "Runtime: " << latency << endl;

  // pareto curve
  long long sol_id = -1;
  PRSolution* sol;
  while (pareto_curve[sol_id] != -2) {
      sol_id = pareto_curve[sol_id];
      sol = sol_map[sol_id];

      int bdwt = sol->getBdwt();
      int load = sol->getLoad();

      cout << "sol_id: " << sol_id << ", load: " << load << ", bdwt: " << bdwt << endl;;
      cout << "string: " << sol->getString() << endl;
  }
  cout << endl;

  return 0;
}
