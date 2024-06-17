#pragma once
#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include "bitset.h"

template<class T>
struct Hash {
    const T& operator()(const T& key) {
        return key;
    }
};

// BKDRHash
struct BKDRHash {
    size_t operator()(const string& str) {
        register size_t hash = 0;
        for (size_t i = 0; i < str.length(); ++i) {
            hash = hash * 131 + str[i];
        }
        return hash;
    }
};

// SDBHash
struct SDBHash {
	size_t operator()(const string& str) {
		register size_t hash = 0;
		for (size_t i = 0; i < str.length(); ++i)
		{
			hash = 65599 * hash + str[i];
			//hash = (size_t)ch + (hash << 6) + (hash << 16) - hash;  
		}
		return hash;
	}
};

// RSHash
struct RSHash {
	size_t operator()(const string& str) {
		register size_t hash = 0;
		size_t magic = 63689;
		for (size_t i = 0; i < str.length(); ++i)
		{
			hash = hash * magic + str[i];
			magic *= 378551;
		}
		return hash;
	}
};

template<class T = string, class Hash1 = BKDRHash, class Hash2 = SDBHash, class Hash3 = RSHash>
class BloomFilter {
public:

    BloomFilter(size_t size);
	void Insert(const T&);
	void Insert(const ifstream&);
    bool IsInBloomFilter(const T&);

private:
    bitset _bs;
    size_t num;
};

void Insert(const T& x)
	{
		size_t index1 = Hash1()(x) % num;
		size_t index2 = Hash2()(x) % num;
		size_t index3 = Hash3()(x) % num;

		_bs.set(index1);
		_bs.set(index2);
		_bs.set(index3);
	}

	void Insert(const ifstream &fd) {
		string str;
		while (getline(fd, str))
		{
			Insert(str);
		}
		
	}

	// 不支持删除，删除某个元素需要计数器影响性能
	bool IsInBloomFilter(const T& x)
	{
		size_t index1 = Hash1()(x) % num;
		size_t index2 = Hash2()(x) % num;
		size_t index3 = Hash3()(x) % num;

		return _bs.test(index1)
			&& _bs.test(index2)
			&& _bs.test(index3);// 可能会误报，判断在是不准确的，判断不在是准确的
	}

#endif