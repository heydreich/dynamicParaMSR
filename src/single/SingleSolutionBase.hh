#ifndef _SINGLESOLUTIONBASE_HH_
#define _SINGLESOLUTIONBASE_HH_

#include "../common/Config.hh"
#include "../common/Stripe.hh"
#include "../inc/include.hh"
#include "../ec/ECBase.hh"
#include "../ec/NodeBatchTask.hh"
#include "../util/DistUtil.hh"

using namespace std;

class SingleSolutionBase {

    public:

        Config* _conf;
        Stripe* _stripe;
        string _blkname; // the blk name to repair

        // targeting ec
        ECBase* _ec;
        string _codename;

        // corresponding Solution constructor will apply
        SingleSolutionBase();
        SingleSolutionBase(string param);

        void init(Stripe* stripe, ECBase* ec, string codename, Config* conf);
        //void genRepairTasks(int ecn, int eck, int ecw, Config* conf, unordered_map<int, int> fail2repair, unsigned int coorIp);
        void genRepairTasks(int ecn, int eck, int ecw);
        virtual int genRepairSolution(string blkname) = 0;
};

#endif
