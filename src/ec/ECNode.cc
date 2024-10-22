#include "ECNode.hh"

ECNode::ECNode(int id) {
  _nodeId = id;
//  _hasConstraint = false;
//  _consId = -1;
}

ECNode::~ECNode() {
//  for (auto item: _oecTasks) {
//    delete item.second;
//  }
//  _oecTasks.clear();
}

void ECNode::setType(string type) {
  if (type == "leaf")
    _type = 0;
  else if (type == "root")
    _type = 1;
  else
    _type = 2;
}

int ECNode::getType() {
  return _type;
}

int ECNode::getNodeId() {
  return _nodeId;
}

void ECNode::setChilds(vector<ECNode*> childs) {
  _childNodes = childs; 
}

void ECNode::setCoefs(vector<int> coefs) {
  _coefs = coefs;
}

void ECNode::addParentNode(ECNode* pnode) {
  _parentNodes.push_back(pnode);
}

int ECNode::getNumChilds() {
  return _childNodes.size();
}

vector<ECNode*> ECNode::getChildNodes() {
  return _childNodes;
}

vector<ECNode*> ECNode::getParentNodes() {
  return _parentNodes;
}

vector<int> ECNode::getCoefs() {
  return _coefs;
}

vector<int> ECNode::getChildIndices() {
  vector<int> toret;
  for (int i=0; i<_childNodes.size(); i++) {
    int idx = _childNodes[i]->getNodeId();
    toret.push_back(idx);
  }
  return toret;
}

vector<int> ECNode::getParentIndices() {
  vector<int> toret;
  for (int i=0; i<_parentNodes.size(); i++) {
    int idx = _parentNodes[i]->getNodeId();
    toret.push_back(idx);
  }
  return toret;
}


void ECNode::dump(int parent) {
  if (parent == -1) parent = _nodeId;
  cout << "(data" << _nodeId;
  if (_childNodes.size() > 0) {
    cout << " = ";
  }
  vector<int> curCoef = _coefs;
//  if (_coefMap.size() > 1) {
//    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(parent);
//    assert (c!=_coefMap.end());
//    curCoef = _coefMap[parent];
//  } else if (_coefMap.size() == 1) {
//    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(_nodeId);
//    assert (c!=_coefMap.end());
//    curCoef = _coefMap[_nodeId];
//  }
  for (int i=0; i<_childNodes.size(); i++) {
    cout << curCoef[i] << " ";
    _childNodes[i]->dump(_nodeId);
    if (i < _childNodes.size() - 1) {
      cout << " + ";
    }
  }
  cout << ")";
}

vector<int> ECNode::getChildColors(const unordered_map<int,int> & coloring) {
  vector<int> ret;
  for (auto child : _childNodes) {
      int childId = child->getNodeId();
      int color = coloring.at(childId);
      if(color != -1){
          if(find(ret.begin(), ret.end(), color) == ret.end())
          {
              ret.push_back(color);
          }
      }
  }
  return ret;
}
void ECNode::removeChildNode() {
  //remove childnodes
  // for (auto it: _childNodes){
  //   cout << it -> getNodeId() << " " << endl;
  // }
  // cout << endl;
  // cout << "ECNode::removeChildNode() " << _nodeId << " " << endl;
  for (auto it:_childNodes){
    // cout << it -> getNodeId() << " " << endl;
    vector<ECNode*>::iterator iter_child=find(_childNodes.begin(), _childNodes.end(),it);
    //erase child's parent's child
    if (iter_child != _childNodes.end()) {
      ECNode* c = *iter_child;
      
      c -> removeParent(this);
      // cout << "node: " << c->getNodeId() << "ParentSizes" << c->getParentsNum() << endl ;
      if (c->getParentsNum() == 0) {
        // cout << "remove " << (*iter_child)->_nodeId << endl;
        c->removeChildNode();
      }
      // _childNodes.erase(iter_child);
    }
  }
  _toDelete = true;
}


void ECNode::removeParent(ECNode* parentPtr) {
  auto it = find(_parentNodes.begin(), _parentNodes.end(), parentPtr);
  if (it != _parentNodes.end()) _parentNodes.erase(it);
}
// void ECNode::removeChild(ECNode* childPtr) {
//   auto it = find(_parentNodes.begin(), _parentNodes.end(), childPtr);
//   if (it != _parentNodes.end()) _parentNodes.erase(it);
// }

int ECNode::getParentsNum(){
  return _parentNodes.size();
}