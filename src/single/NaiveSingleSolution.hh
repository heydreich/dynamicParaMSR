#ifndef _NAIVESINGLESOLUTION_HH_
#define _NAIVESINGLESOLUTION_HH_

#include "SingleSolutionBase.hh"
#include "../ec/ECBase.hh"
#include "PRSolution.hh"

using namespace std;

//#define CLUSTERSIZE 32768
class NaiveSingleSolution : public SingleSolutionBase {
    private:
        void genNaiveColoringForSingleFailure(Stripe* stripe, unordered_map<int, int>& res, int failblkidx);
        void evaluateColoring(Stripe* stripe, vector<int> curres, unordered_map<int ,int> coloring, vector<int> itm_idx);
        void genPareto(unordered_map<long long, long long>& pareto_curve,
            unordered_map<long long, PRSolution*>& sol_map,
            vector<int> itm_idx, vector<int> candidates,
            Stripe* stripe, unordered_map<int ,int> coloring);
        bool dominate(PRSolution* a, PRSolution* b);

    public:
        NaiveSingleSolution();
        int genRepairSolution(string blkname);

};



#endif