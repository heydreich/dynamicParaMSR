#include "Bandwidth.hh"

Bandwidth::Bandwidth(std::string& filepath, bool print)     {
    _bwf = std::fstream(filepath, std::ios::in);
    if (!_bwf.is_open()) {
        std::cerr << "no bandwidth file:" << filepath << std::endl;
        exit(-1);    
    }

    _bwf >> _bwNum >> _nodeNum >> _eth;
    std::unordered_map<int,double> UploadMap;
    std::unordered_map<int,double> DownloadMap;
    _cur = 0;
    _if_print = print;
}

bool Bandwidth::LoadNext() {
    //Check whether the file is ended   
    if (++_cur > _bwNum) return false;
    std::cout << "\n cur : " << _cur << std::endl;

    //Load
    double UploadArray[_nodeNum];
    double DownloadArray[_nodeNum];
    std::string content;
    int flag;
    int i = 0;
    int lineNum = _nodeNum * 2 + 2;
    int readNum = 0;
    int min = 0, max = 1000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    while (readNum < lineNum) {
        readNum++;
        _bwf >> content;
        if (content == "u") {
            flag = 1;
            i = 0;
            continue;
        }
        if (content == "d") {
            flag = 2;
            i = 0;
            continue;
        }
        if (flag == 1) {
            UploadArray[i] = stod(content);
            // if (i > 14) UploadArray[i] = double(distrib(gen));
            // UploadArray[i] = stod(content);
            // if (UploadArray[i] < 500) {
            //     UploadArray[i] = 500;
            // }
            // UploadArray[i] = 500;
            i++;
        } else if (flag == 2) {
            DownloadArray[i] = stod(content);
            // if (i > 14) DownloadArray[i] = double(distrib(gen));
            i++;
        }
    }
    if (_cur == 1) {
        for (int i = 0; i < _nodeNum; i++) {
            _idx2bdwt.insert(std::make_pair(i, std::make_pair(UploadArray[i], DownloadArray[i])));
            if(BANDWIDTHDEBUG) std::cout << i << ": upload " << UploadArray[i] << ", download " << DownloadArray[i] << std::endl;
        }
    } else {
        for (int i = 0; i < _nodeNum; i++) {
            _idx2bdwt[i].first = UploadArray[i];
            _idx2bdwt[i].second = DownloadArray[i];
            if(BANDWIDTHDEBUG) std::cout << i << ": upload " << UploadArray[i] << ", download " << DownloadArray[i] << std::endl;
        }
    }

    return true;
}

void Bandwidth::setBandwidth(const Config* conf) {
  for (auto node : _idx2bdwt) {
    double coefficient = 1000;
    double uploaddb = node.second.first * coefficient;
    double downloaddb = node.second.second * coefficient;
    if (uploaddb < minBw*coefficient) uploaddb = minBw*coefficient;
    if (downloaddb < minBw*coefficient) downloaddb = minBw*coefficient;
    string upload = to_string(uploaddb);
    string download = to_string(downloaddb);
    upload = upload.substr(0, upload.find("."));
    download = download.substr(0, download.find("."));

    std::stringstream fmt;
    if (node.first < conf -> _agentsIPs.size()) fmt << "ssh " << 
    RedisUtil::ip2Str(conf -> _agentsIPs[node.first]) <<  
    " \"" << ksetCmd << _eth << " -u " << upload << " -d " << download << "\"";
    if (node.first >= conf -> _agentsIPs.size()) fmt << "ssh " <<
    RedisUtil::ip2Str(conf -> _repairIPs[node.first-conf -> _agentsIPs.size()]) << 
    " \"" << ksetCmd << _eth << " -u " << upload << " -d " << download << "\"";
    // auto _ = system(fmt.str().c_str());
    if (_if_print) {
        std::cout << fmt.str() << std::endl;
    } else {
        std::cout << fmt.str() << std::endl;
        auto _ = system(fmt.str().c_str());
    }
  }
}

void Bandwidth::setFull(int node_id) {
    _idx2bdwt[node_id].first = 0;
    _idx2bdwt[node_id].second = 1000;
}

void Bandwidth::ResetBandwidth(const Config* conf) {
  for (auto node : _idx2bdwt) {
    std::stringstream fmt;
    if (node.first < conf -> _agentsIPs.size()) fmt << "ssh " << 
    RedisUtil::ip2Str(conf -> _agentsIPs[node.first]) <<  
    " \"" << ResetCmd << _eth << "\"";
    if (node.first >= conf -> _agentsIPs.size()) fmt << "ssh " <<
    RedisUtil::ip2Str(conf -> _repairIPs[node.first-conf -> _agentsIPs.size()]) << 
    " \"" << ResetCmd << _eth << "\"";
    // auto _ = system(fmt.str().c_str());
    if (_if_print) {
        std::cout << fmt.str() << std::endl;
    } else {
        std::cout << fmt.str() << std::endl;
        auto _ = system(fmt.str().c_str());
    } 
  }
}

Bandwidth::~Bandwidth() {
    Close();
}

int Bandwidth::getCurId() {
    return _cur;
}

void Bandwidth::clearCache(const Config* conf){
    for (auto node : _idx2bdwt) {
    std::stringstream fmt;
    if (node.first < conf -> _agentsIPs.size()) fmt << "ssh " << 
    RedisUtil::ip2Str(conf -> _agentsIPs[node.first]) <<  
    " \"" << "redis-cli flushall" << "\"";
    if (node.first >= conf -> _agentsIPs.size()) fmt << "ssh " <<
    RedisUtil::ip2Str(conf -> _repairIPs[node.first-conf -> _agentsIPs.size()]) << 
    " \"" << "redis-cli flushall" << "\"";
    // auto _ = system(fmt.str().c_str());
    if (_if_print) {
        std::cout << fmt.str() << std::endl;
    } else {
        std::cout << fmt.str() << std::endl;
        auto _ = system(fmt.str().c_str());
    } 
  }
} 

void Bandwidth::Close() {
    if (_bwf.is_open()) _bwf.close();
}

double Bandwidth::getBottleneck(int index, std::vector<int> testTable) {
    //0 out, 1 in
    double UpPerSubBlock = testTable[0] == 0 ? DBL_MAX : _idx2bdwt[index].first/testTable[0];
    double DownPerSubBlock = testTable[1] == 0 ? DBL_MAX : _idx2bdwt[index].second/testTable[1];
    return UpPerSubBlock < DownPerSubBlock ? UpPerSubBlock:DownPerSubBlock;
}

int Bandwidth::getBottlePort(int index, std::vector<int> testTable) {
    //0 means bottle port is upPort, 1 means DownPort
    double UpPerSubBlock = testTable[0] == 0 ? DBL_MAX : _idx2bdwt[index].first/testTable[0];
    double DownPerSubBlock = testTable[1] == 0 ? DBL_MAX : _idx2bdwt[index].second/testTable[1];
    return UpPerSubBlock < DownPerSubBlock ? 0:1;
}

double Bandwidth::evaluateSort(int index, std::vector<int> testTable) {
    //0 out, 1 in
    double UpPerSubBlock = testTable[0] == 0 ? _idx2bdwt[index].first : _idx2bdwt[index].first/testTable[0];
    double DownPerSubBlock = testTable[1] == 0 ? _idx2bdwt[index].second : _idx2bdwt[index].second/testTable[1];
    return UpPerSubBlock < DownPerSubBlock ? UpPerSubBlock:DownPerSubBlock;
}

double Bandwidth::getglobalBottleneck(std::vector<std::vector<int>> testtable) {
    double bottleneck = DBL_MAX;
    for (int i = 0; i < testtable.size(); i++) {
        double newbottleneck = getBottleneck(i, std::vector<int>{testtable[i][0], testtable[i][1]});
        if (newbottleneck < bottleneck) bottleneck = newbottleneck;
    }
    return bottleneck;
}

std::vector<int> Bandwidth::getIdealLoad(int index, double limitedbn) {
    //0 out, 1 in
    int outLoad = _idx2bdwt[index].first / limitedbn;
    int inLoad = _idx2bdwt[index].second / limitedbn;
    
    return std::vector<int>{outLoad, inLoad};
}

std::vector<int> Bandwidth::sortByUp(vector<int> nodeidxs){
    vector<int> res;
    for (int i = 0; i < nodeidxs.size(); i++) res.push_back(i);
    for (int i = 0; i < nodeidxs.size(); i++) {
        for (int j = i + 1; j < nodeidxs.size(); j++) {
            if (_idx2bdwt[nodeidxs[res[i]]].first > _idx2bdwt[nodeidxs[res[j]]].first) {
                int swap = res[i];
                res[i] = res[j];
                res[j] = swap;
            }
        }
    }
    return res;
}