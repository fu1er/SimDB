#ifndef __B_PLUS_TREE_H__
#define __B_PLUS_TREE_H__
#include <vector>
#include "IndexPage.h"
#include "IndexComponent.h"
#include "../filesystem/bufmanager/BufPageManager.h"

class BPlusTree{
private:
    int fileId;
    int rootIndex, tableIndex;
    uint16_t indexLen;
    uint8_t colType;
    BufPageManager* bufPageManager;
    IndexHeader* indexHeader;

    void writeIndexTable();

    int getNextFreePage(); // alloc one empty page

    void _transform(int& val, int pageId, int slotId);

    void _transformR(int val, int& pageId, int& slotId);

    int searchUpperBound(void* data);

    void recycle(std::vector<IndexPage*> &rec, std::vector<int> &pageIndex, bool writeback = false);

    void dealOverFlow(IndexPage* indexPage, std::vector<IndexPage*> &rec, std::vector<int> &pageIndex);

    void dealUnderFlow(IndexPage* indexPage, std::vector<IndexPage*> &rec, std::vector<int> &pageIndex);

    void update(IndexPage* indexPage, void* updateData, std::vector<IndexPage*> &rec, std::vector<int> &pageIndex);

public:
    IndexPage* root;

    BPlusTree(int fileId_, BufPageManager* bufPageManager_, uint16_t indexLen_, uint8_t colType_);

    ~BPlusTree();

    void search(void* data, std::vector<int> &res);
    // attention that res will be cleared first
    // return value of those slots whose data_ is equla to data

    void searchBetween(void* ldata, void* rdata, std::vector<int> &res, bool lIn, bool rIn);
    // bound [ldata, rdata]
    // set ldata or rdata to nullptr if want a one-way search

    void insert(void* data, const int val);

    void remove(void* data, int val = -1);

    void update(void* data, int oldVal, int newVal);

    void transform(int& val, int pageId, int slotId);

    void transformR(int val, int& pageId, int& slotId);
};

#endif