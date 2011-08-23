#ifndef concuno_kdtree_h
#define concuno_kdtree_h


#include "stats.h"
#include "tree.h"


cnCBegin;


typedef struct cnKdSplitter {

  cnRandom random;

}* cnKdSplitter;


cnKdSplitter cnKdSplitterCreate(cnRandom random);


void cnKdSplitterDestroy(cnKdSplitter splitter);


cnCEnd;


#endif
