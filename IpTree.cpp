#include "IpTree.h"

string IpNode::npIp(const string &str)
{
    string res;
    int len = str.length();
    int cur = 0;
    while (cur < len)
    {
        int con = 3;
        string path;
        while (cur < len && str[cur] != '.')
        {
            path += str[cur];
            ++cur;
            --con;
        }
        while (con)
        {
            res += '0';
            --con;
        }
        ++cur;
        res += path;
    }
    len = res.length() - 1;
    while(len >= 0 && res[len] == '0') --len;
    res.resize(len + 1);
    return res;
}

void IpNode::Insert(const string &s)
{
    IpNode *cur = this;
    string str = npIp(s);
    for (int i = 0; i < str.length(); ++i)
    {
        if (cur->next[str[i] - '0'] == nullptr)
        {
            cur->next[str[i] - '0'] = new IpNode();
        }
        cur = cur->next[str[i] - '0'];
    }
    cur->end = true;
}

int IpNode::Delete(const string &s)
{
    IpNode *cur = this;
    string str = npIp(s);
    for (int i = 0; i < str.length(); ++i)
    {
        if (cur->next[str[i] - '0'] == nullptr)
        {
            return -1;
        }
        cur = cur->next[str[i] - '0'];
    }
    cur->end = false;
    return 0;
}

bool IpNode::Search(const string &s) {
    IpNode *cur = this;
    string str = npIp(s);
    for (int i = 0; i < str.length(); ++i)
    {
        if (cur->next[str[i] - '0'] == nullptr)
        {
            return false;
        }
        cur = cur->next[str[i] - '0'];
    }
    return cur->end;
}

IpTree::Insert(const std::ifstream &fd) {
    string str;

    while (getline(fd, str)) {
        root.Insert(str);
    }
}