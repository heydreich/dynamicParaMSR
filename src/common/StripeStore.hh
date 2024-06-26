#ifndef _STRIPESTORE_HH_
#define _STRIPESTORE_HH_

#include "../inc/include.hh"
#include "../util/DistUtil.hh"
#include "Config.hh"
#include "Bandwidth.hh"
#include "Stripe.hh"

class StripeStore{

    private:
        Config* _conf;
        vector<Stripe*> _stripe_list;
        unordered_map<string, Stripe*> _blk2stripe;
        

    public:
        Bandwidth* _bdwt;

        StripeStore(Config* conf);
        ~StripeStore();
        vector<Stripe*> getStripeList();
        Stripe* getStripeFromBlock(string blkname);
        void setBandwidth(Bandwidth* bdwt);
};

#endif
