//
//  main.cpp
//  zskiplist
//
//  Created by ice on 5/4/15.
//  Copyright (c) 2015 ice. All rights reserved.
//

#include <iostream>
#include "SkipList.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    SkipList skipList;
    for(int i=0;i<100;++i){
        skipList.insert(i, i);
    }
    
    Rangespec<double> range(10, 1, 40, 1);
    skipList.deleteRangeByScore(&range);
    printf("%d rank is %d\n", 50, skipList.getRank(50, 50));
    
    for (SkipNode *it = skipList.first(); it != NULL; it = it->next()) {
        printf(" %d %f\n", it->obj, it->score);
    }
    
    return 0;
}
