#pragma once
#ifndef BITSET_H
#define BITSET_H

// #include <string>
// using std::string;

#include <fstream>
#include <vector>

using std::vector;
using std::ifstream;

class bitset {
public:
    bitset(const size_t bitCount):_bitcount(bitCount),
        _bitset(bitCount / 32 + 1) {}

    // 每个字节用0和1记录某个数字存在状态
	// 把x所在的数据在位图的那一位变成1
    void set(const int x) {
        if(x > _bitCount) return;

        int index = x / 32; // x是第index个数
        int pos = x % 32;   // 的第pos位
        _bitset[index] |= (1 << pos);
    }

    void reset(const int x) {
        if(x > _bitCount) return;

        int index = x / 32;
        int pos = x % 32;
        _bitset[index] &= ~(1 << pos);
    }

    // void Insert(const ifstream &fd) {
    //     string str;
    //     while (getline(fd, str)) {
    //         unsigned int ipcnt = 1;
    //         unsigned int num = 0;
    //         for(int i = 0; i < str.length(); ++i) {
    //             if(str[i] == '.') {
    //                 ipcnt *= num;
    //                 num = 0;
    //             }//mistake
    //         }
    //         set()
    //     }
    // }

    // 判断x是否存在
	bool test(int x)
	{
		if (x > _bitCount) return false;

		int index = x / 32;// x是第index个整数
		int pos = x % 32;// x是滴index个整数的第pos个位

		return _bitset[index] & (1 << pos);
	}

private:
    vector<int> _bitset;
    size_t _bitCount;
}

#endif