#include "StripeStore.hh"

StripeStore::StripeStore(Config* conf) {
    _conf = conf;

    string sspath = conf->_ssDir + "/placement";
    cout << sspath << endl;

    // 0. read block placement
    ifstream infile(sspath);
    string line;

    int stripeid=0;
    while (getline(infile, line)) {
        vector<string> items = DistUtil::splitStr(line.substr(0, line.length()-1), " ");

        string stripename = items[0];
        vector<string> blklist;
        vector<unsigned int> iplist;
        vector<int> nodeidlist;

        for (int i=1; i<items.size(); i++) {
            vector<string> tmpitems = DistUtil::splitStr(items[i], ":");
            string blockname = tmpitems[0];
            string locationstr = tmpitems[1];
            unsigned int location = inet_addr(locationstr.c_str());
            int nodeid = _conf->_ip2agentid[location];

            blklist.push_back(blockname);
            iplist.push_back(location);
            cout << "blockname:" << blockname << ", location: " << locationstr << ", nodeid:" << nodeid << endl; 
            nodeidlist.push_back(nodeid);
        }
        
        Stripe* curstripe = new Stripe(stripeid++, stripename, blklist, iplist, nodeidlist);
        _stripe_list.push_back(curstripe);

        for (string blk: blklist) {
            _blk2stripe.insert(make_pair(blk, curstripe));
        }
    }

    cout << "number of stripes: " << _stripe_list.size() << endl;

}

StripeStore::~StripeStore() {
    // note that we generate stripes inside stripe store
    // thus, we free stripes inside stripe store
    for (int i=0; i<_stripe_list.size(); i++) {
        Stripe* curstripe = _stripe_list[i];
        if (curstripe)
            delete curstripe;
    }
}

vector<Stripe*> StripeStore::getStripeList() {
    return _stripe_list;
}

Stripe* StripeStore::getStripeFromBlock(string blkname) {
    assert(_blk2stripe.find(blkname) != _blk2stripe.end());
    return _blk2stripe[blkname];
}

void StripeStore::setBandwidth(Bandwidth* bdwt) {
    _bdwt = bdwt;
}

