//
//  SkipList.cpp
//  zskiplist
//
//  Created by ice on 5/4/15.
//  Copyright (c) 2015 ice. All rights reserved.
//

#include "SkipList.h"
#include <cstdlib>


static SkipNode*  CreateSkipNode(int level, double score, SkipObj obj){
    SkipNode* zn =(SkipNode*) malloc(sizeof(SkipNode) + level*(sizeof(SkipNode::SkipNodeLevel)));
    zn->score = score;
    zn->obj = obj;
    return zn;
}

SkipList::SkipList():
level(1), length(0), header(CreateSkipNode(SKIPLIST_MAXLEVEL, 0, NULL))
{
    for (int i=0; i < SKIPLIST_MAXLEVEL; ++i) {
        header->level[i].forward = NULL ;
        header->level[i].span = 0;
    }
    header->backward = NULL;
    tail = NULL;
}

SkipList::~SkipList(){
    SkipNode* node = this->header->level[0].forward, *next;
    free(this->header);
    while (node) {
        next = node->level[0].forward;
        freeNode(node);
        node = next;
    }
}


void SkipList::insert(double score, SkipObj obj){
    SkipNode* update[SKIPLIST_MAXLEVEL], *x;
    unsigned int rank[SKIPLIST_MAXLEVEL];
    int i, level;
    
    x = header;
    for (i = this->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (this->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward &&
               (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj < obj))) {
                    rank[i] += x->level[i].span;
                    x = x->level[i].forward;
                }
        update[i] = x;
    }
    /* we assume the key is not already inside, since we allow duplicated
     * scores, and the re-insertion of score and redis object should never
     * happen since the caller of zslInsert() should test in the hash table
     * if the element is already inside or not. */
    level = randomLevel();
    if (level > this->level) {
        for (i = this->level; i < level; i++) {
            rank[i] = 0;
            update[i] = this->header;
            update[i]->level[i].span = this->length;
        }
        this->level = level;
    }
    x = CreateSkipNode(level,score,obj);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;
        
        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }
    
    /* increment span for untouched levels */
    for (i = level; i < this->level; i++) {
        update[i]->level[i].span++;
    }
    
    x->backward = (update[0] == this->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        this->tail = x;
    this->length++;
}

void SkipList::deleteNode(SkipNode *x, SkipNode **update){
    int i;
    for (i = 0; i < this->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        this->tail = x->backward;
    }
    while(this->level > 1 && this->header->level[this->level-1].forward == NULL)
        this->level--;
    this->length--;
}


bool SkipList::pop(double score, SkipObj obj){
    SkipNode *update[SKIPLIST_MAXLEVEL], *x;
    int i;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
               (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj < obj )))
            x = x->level[i].forward;
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score && x->obj == obj ) {
        deleteNode(x, update);
        freeNode(x);
        return 1;
    }
    return 0; /* not found */
}

bool SkipList::isInRange(Rangespec<double> *range){
    SkipNode* x;
    /* Test for ranges that will always be empty. */
    if (range->min > range->max ||
        (range->min == range->max && (range->minex || range->maxex)))
        return 0;
    x = this->tail;
    if (x == NULL || !valueGteMin(x->score,range))
        return 0;
    x = this->header->level[0].forward;
    if (x == NULL || !valueLteMax(x->score,range))
        return 0;
    return 1;
}


SkipNode* SkipList::firstInRange(Rangespec<double> *range){
    SkipNode *x;
    int i;
    
    /* If everything is out of range, return early. */
    if (!isInRange(range)) return NULL;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward &&
               !valueGteMin(x->level[i].forward->score,range))
            x = x->level[i].forward;
    }
    
    /* This is an inner range, so the next node cannot be NULL. */
    x = x->level[0].forward;
//    redisAssert(x != NULL);
    
    /* Check if score <= max. */
    if (!valueLteMax(x->score,range)) return NULL;
    return x;
    
}

SkipNode *SkipList::lastInRange(Rangespec<double> *range) {
    SkipNode *x;
    int i;
    
    /* If everything is out of range, return early. */
    if (!isInRange(range)) return NULL;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward &&
               valueLteMax(x->level[i].forward->score,range))
            x = x->level[i].forward;
    }
    
    /* This is an inner range, so this node cannot be NULL. */
//    redisAssert(x != NULL);
    
    /* Check if score >= min. */
    if (!valueGteMin(x->score,range)) return NULL;
    return x;
}


unsigned int SkipList::deleteRangeByScore(Rangespec<double> *range){
    SkipNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int removed = 0;
    int i;
    
    x = this->header;
    for (i =this->level-1; i >= 0; i--) {
        while (x->level[i].forward && (range->minex ?
                                       x->level[i].forward->score <= range->min :
                                       x->level[i].forward->score < range->min))
            x = x->level[i].forward;
        update[i] = x;
    }
    
    /* Current node is the last with score < or <= min. */
    x = x->level[0].forward;
    
    /* Delete nodes while in range. */
    while (x &&
           (range->maxex ? x->score < range->max : x->score <= range->max))
    {
        SkipNode *next = x->level[0].forward;
        deleteNode(x,update);
        dictDelete(x->obj);
        freeNode(x);
        removed++;
        x = next;
    }
    return removed;
}

unsigned int SkipList::deleteRangeByRank(unsigned int start, unsigned int end) {
    SkipNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int traversed = 0, removed = 0;
    int i;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) < start) {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    
    traversed++;
    x = x->level[0].forward;
    while (x && traversed <= end) {
        SkipNode *next = x->level[0].forward;
        deleteNode(x,update);
        dictDelete(x->obj);
        freeNode(x);
        removed++;
        traversed++;
        x = next;
    }
    return removed;
}


unsigned int SkipList::getRank(double score, SkipObj o) {
    SkipNode *x;
    unsigned int rank = 0;
    int i;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
               (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                 x->level[i].forward->obj <= o))) {
                    rank += x->level[i].span;
                    x = x->level[i].forward;
                }
        
        /* x might be equal to this->header, so test if obj is non-NULL */
        if (x->obj && x->obj == o) {
            return rank;
        }
    }
    return 0;
}

/* Finds an element by its rank. The rank argument needs to be 1-based. */
SkipNode* SkipList::getElementByRank(unsigned int rank) {
    SkipNode *x;
    unsigned long traversed = 0;
    int i;
    
    x = this->header;
    for (i = this->level-1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
        {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        if (traversed == rank) {
            return x;
        }
    }
    return NULL;
}


