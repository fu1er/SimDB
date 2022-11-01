#ifndef __B_PLUS_TREE_H__
#define __B_PLUS_TREE_H__
#include <vector>
#include "IndexPage.h"
#include "../filesystem/bufmanager/BufPageManager.h"

class BPlusTree{
private:
    int fileId;
    int rootIndex;
    uint16_t indexLen;
    uint8_t colType;
    BufPageManager* bufPageManger;

    inline void transform(int& val, int pageId, int slotId);

    inline void transformR(int val, int& pageId, int& slotId);

    int searchUpperBound(void* data);

    void recycle(std::vector<IndexPage*> &rec, std::vector<int> &pageIndex, bool writeback = false);

public:
    IndexPage* root;

    BPlusTree(int fileId_, BufPageManager* bufPageManager_, int pageId, uint16_t indexLen_, uint8_t colType_);

    ~BPlusTree();

    void search(void* data, std::vector<int> &res);
    // attention res will be cleared first

    void searchBetween(void* ldata, void* rdata, std::vector<int> &res);
    // bound [ldata, rdata]
    // set ldata or rdata to nullptr if want a one-way search
};

#endif