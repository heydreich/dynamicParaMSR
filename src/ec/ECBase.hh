#ifndef _ECBASE_HH_
#define _ECBASE_HH_

#include "../inc/include.hh"

#include "ECDAG.hh"

using namespace std;

class ECBase {
  public:
    int _n, _k, _w;

    ECBase();
    ECBase(int n, int k, int w, vector<string> param);
    
    virtual ECDAG* Encode() = 0;
    virtual ECDAG* Decode(vector<int> from, vector<int> to) = 0;
};

#endif
