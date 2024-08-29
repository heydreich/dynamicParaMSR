#include "Bandwidth.hh"

Bandwidth::Bandwidth(std::string& filepath)     {
    _bwf = std::fstream(filepath, std::ios::in);
    if (!_bwf.is_open()) {
        std::cerr << "no bandwidth file:" << filepath << std::endl;
        exit(-1);    
    }

    _bwf >> _bwNum >> _nodeNum;
    std::unordered_map<int,double> UploadMap;
    std::unordered_map<int,double> DownloadMap;
    _cur = 0;
    
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
            i++;
        } else if (flag == 2) {
            DownloadArray[i] = stod(content);
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

Bandwidth::~Bandwidth() {
    Close();
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
    double UpPerSubBlock = testTable[0] == 0 ? DBL_MAX : _idx2bdwt[index].first/testTable[1];
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