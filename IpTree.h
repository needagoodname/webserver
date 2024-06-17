#ifndef IPTREE_H
#define IPTREE_H

#include <string>
#include <fstream>

using std::string;

class IpNode {
public:
    IpNode(const bool &l):end(l) {}
    IpNode(){ end = false; }

    void Insert(const string &);
    int Delete(const string &);
    bool Search(const string &);
private:
    IpNode* next[10];
    bool end;
    string npIp(const string &);
};

class IpTree {
public:
    void Insert(const string &str) {
        root.Insert(str);
    }

    int Delete(const string &str) {
        return root.Delete(str);
    }

    bool Search(const string &str) {
        return root.Search(str);
    }
    
    void Insert(const std::ifstream &);
private:
    IpNode root;
};

#endif