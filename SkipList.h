//
//  SkipList.h
//  zskiplist
//
//  Created by ice on 5/4/15.
//  Copyright (c) 2015 ice. All rights reserved.
//

#ifndef __zskiplist__SkipList__
#define __zskiplist__SkipList__

#include <stdio.h>
#include <cstdlib>
#include <unordered_map>
using namespace std;

typedef int SkipObj;

const int SKIPLIST_MAXLEVEL = 32;

const double SKIP_LIST_P = 0.25;

struct SkipNode{
    SkipObj obj;
    double score;
    SkipNode* backward;
    struct SkipNodeLevel {
        struct SkipNode* forward;
        unsigned int span;
    }level[];
    
    SkipNode* next(){
        return level[0].forward;
    }
};


template<class T>
struct Rangespec{
    T min, max;
    bool minex, maxex;
    Rangespec(T _min, bool _minex, T _max, bool _maxex):
    min(_min), minex(_minex), max(_max), maxex(_maxex){
        
    }
};

class SkipList {
    SkipNode *header, *tail;
    
    unordered_map<SkipObj, double> dict;
    
    unsigned int length;
    int level;
    
    
    //幂次定律 求出随机数
    int randomLevel(){
        int level = 1;
        while ((random()&0xffff) < (SKIP_LIST_P*0xffff) ) {
            ++level;
        }
        return level > SKIPLIST_MAXLEVEL ? SKIPLIST_MAXLEVEL : level;
    }
    
    void deleteNode(SkipNode *x, SkipNode **update);
    
    template<class V>
    int valueGteMin(double value, Rangespec<V> *spec) {
        return spec->minex ? (value > spec->min) : (value >= spec->min);
    }
    template<class V>
    int valueLteMax(double value, Rangespec<V> *spec) {
        return spec->maxex ? (value < spec->max) : (value <= spec->max);
    }
    
    void freeNode(SkipNode *node) {
        free(node);
    }
    
    void dictDelete(SkipObj obj){
        dict.erase(obj);
    }
    
public:
    SkipList();
    
    SkipNode* first(){
        return this->header->level[0].forward;
    }
    
    void insert(double score, SkipObj obj);
    
    bool pop(double score, SkipObj obj);
    
    
    bool isInRange(Rangespec<double> *spec);
    
    SkipNode *firstInRange(Rangespec<double> *range);
    
    SkipNode *lastInRange(Rangespec<double> *range);
    
    unsigned int deleteRangeByScore(Rangespec<double> *range);
    
    unsigned int deleteRangeByRank(unsigned int start, unsigned int end);
    
    unsigned int getRank(double score, SkipObj o);
    
    SkipNode* getElementByRank(unsigned int rank);
    
    ~SkipList();
};

#endif /* defined(__zskiplist__SkipList__) */
