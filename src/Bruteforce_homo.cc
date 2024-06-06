#include "common/Config.hh"
#include "common/Stripe.hh"
#include "ec/ECDAG.hh"
#include "util/DistUtil.hh"

#include "ec/Clay.hh"
#include "ec/BUTTERFLY.hh"

using namespace std;

/*
 * The brute force algorithm
 * We suppose the original blocks are stored in 0, 1, 2, ..., n-1
 * We repair the block in node with index = n
 */

void usage() {
  cout << "Usage: ./Bruteforce_homo" << endl;
  cout << "    1. code" << endl;
  cout << "    2. n" << endl;
  cout << "    3. k" << endl;
  cout << "    4. w" << endl;
  cout << "    5. failNodeId" << endl;
}  

double percentage(double found, double total) {  
    return (double)found/(double)total;
}

void simple_search(vector<int>& curres, int curidx, vector<int> itm_idx, unordered_map<int, int> coloring, vector<int> candidates, 
        int fail_node_id, Stripe* stripe, double* found, double total, 
        unordered_map<int, vector<int>>& max2bwlist, unordered_map<int, double>& process, 
        struct timeval starttime) {

    if (curidx == itm_idx.size()) {
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

        int load = stripe->getLoad();
        int bdwt = stripe->getBdwt();

        if (max2bwlist.find(load) == max2bwlist.end()) {
            vector<int> curlist = {bdwt};
            max2bwlist.insert(make_pair(load, curlist));
        } else {
            vector<int> curlist = max2bwlist[load];              
            if (find(curlist.begin(), curlist.end(), bdwt) == curlist.end())
                max2bwlist[load].push_back(bdwt);
        }

        *found += 1;
        double perc = percentage(*found, total);

        int count = 100 * perc;
        if (count % 2 == 0 && process.find(count) == process.end()) {
            struct timeval t;
            gettimeofday(&t, NULL);
            double ts = DistUtil::duration(starttime, t);
            cout << "percentage: " << count << "%" << " at " << ts << endl;
            process.insert(make_pair(count, ts));
        }

        return;
    }

    for (int i=0; i<candidates.size(); i++) {
        int curcolor = candidates[i];
        curres[curidx] = curcolor;
        simple_search(curres, curidx+1, itm_idx, coloring, candidates, fail_node_id, stripe, found, total, max2bwlist, process, starttime);
        curres[curidx] = -1;
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
  // blk0: node0
  // blk1: node1
  // blk2: node2
  // blk3: node3
  vector<int> nodeidlist;
  for (int i=0; i<n; i++)
      nodeidlist.push_back(i);
  int stripeid = 0;

  // 1. generate stripe
  Stripe* stripe = new Stripe(stripeid, nodeidlist); 

  // 2. generate ec
  ECBase* ec;
  vector<string> param;
  if (code == "Clay")
    ec = new Clay(n, k, w, {to_string(n-1)});
  else if (code == "Butterfly")
    ec = new BUTTERFLY(n, k, w, param);
  else {
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
  
  // The size of the solution space
  double spacesize = pow(candidates.size(), itm_idx.size());
  cout << "Spacesize: " << spacesize << endl;

  // simple search
  vector<int> curres;
  for (int i=0; i<itm_idx.size(); i++)
    curres.push_back(-1);

  double found=0;
  unordered_map<int, vector<int>> max2bwlist;
  unordered_map<int, double> process;

  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  simple_search(curres, 0, itm_idx, coloring, candidates, failNodeId, stripe, &found, spacesize, max2bwlist, process, time1);
//  simple_search(curres, 0, itm_idx, candidates, &found, spacesize, sidx2ip, ecdag, max2bwlist, process, time1, clustersize);
  gettimeofday(&time2, NULL);
  double latency = DistUtil::duration(time1, time2);
  cout << "Runtime: " << latency << endl;

  //for (auto item: max2bwlist) {
  //  int max = item.first;
  //  for (auto bw: max2bwlist[max])
  //    cout << max <<  "\t" << bw << endl;
  //}

  int min_load = -1;
  for (auto item: max2bwlist) {
      int load = item.first;
      if (min_load == -1)
          min_load = load;
      else if (load < min_load)
          min_load = load;
  }

  int min_bdwt = -1;
  for (auto item: max2bwlist[min_load]) {
      if (min_bdwt == -1)
          min_bdwt = item;
      else if (item < min_bdwt)
          min_bdwt = item;
  }

  cout << "MLP: (" << min_load << ", " << min_bdwt << ")" << endl;
  return 0;
}
