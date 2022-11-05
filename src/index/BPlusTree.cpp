#include "BPlusTree.h"
#include "assert.h"

BPlusTree::BPlusTree(int fileId_, BufPageManager* bufPageManager_, uint16_t indexLen_, uint8_t colType_) {
    fileId = fileId_;
    indexLen = indexLen_;
    colType = colType_;
    bufPageManager = bufPageManager_;
    indexHeader = new IndexHeader((uint8_t*)bufPageManager->getPage(fileId, 0, tableIndex));
    if (indexHeader->valid == 0) {
        indexHeader->init();
        writeIndexTable();
    }
    BufType data = bufPageManager->getPage(fileId, indexHeader->rootPageId, rootIndex);
    root = new IndexPage((uint8_t*)data, indexLen_, colType_, indexHeader->rootPageId);
    bufPageManager->markDirty(rootIndex);
}

BPlusTree::~BPlusTree() {
    delete root;
}

void BPlusTree::writeIndexTable() {
    int index;
    BufType data = bufPageManager->getPage(fileId, 0, index);
    memcpy(data, indexHeader, sizeof(indexHeader));
    bufPageManager->markDirty(index);
}

int BPlusTree::getNextFreePage() {
    int res;
    if (indexHeader->firstEmptyPage >= 0) {
        res = indexHeader->firstEmptyPage;
        int index;
        IndexPageHeader* header = (IndexPageHeader*) bufPageManager->getPage(fileId, indexHeader->firstEmptyPage, index);
        indexHeader->firstEmptyPage = header->nextFreePage;
    } else {
        res = indexHeader->totalPageNumber;
    }
    indexHeader->totalPageNumber++;
    writeIndexTable();
    return res;
}

inline void BPlusTree::transform(int& val, int pageId, int slotId) {
    val = pageId * (root->getCapacity() + 2) + slotId;
}

inline void BPlusTree::transformR(int val, int& pageId, int& slotId) {
    pageId = val / (root->getCapacity() + 2);
    slotId = val % (root->getCapacity() + 2);
}

void BPlusTree::recycle(std::vector<IndexPage*> &rec, std::vector<int> &pageIndex, bool writeback) {
    int siz = rec.size();
    for (int i = 0; i < siz; i++) {
        if (writeback) {
            bufPageManager->markDirty(pageIndex[i]);
        }
        delete rec[i];
    }
}

int BPlusTree::searchUpperBound(void* data) {
    IndexPage* cur = root;
    std::vector<IndexPage*> rec;
    std::vector<int> pageIndex;
    while (cur->getPageType() == INDEX_PAGE_INTERIOR) {
        int nextSlot = cur->searchUpperBound(data);
        int16_t childIndex;
        if (nextSlot >= 0) {
            childIndex = *cur->getChildIndex(nextSlot);
        } else {
            childIndex = *cur->getChildIndex(*cur->getLastIndex());
        }
        int index;
        cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, childIndex, index)), indexLen, colType, childIndex);
        rec.push_back(cur);
        pageIndex.push_back(index);
    }
    int slotId = cur->searchUpperBound(data);
    int val;
    if (slotId >= 0) {
        transform(val, cur->getPageId(), slotId);
    } else { // data searched is greater than all the other data
        val = -cur->getPageId();
    }
    recycle(rec, pageIndex);
    return val;
}

void BPlusTree::dealOverFlow(IndexPage* indexPage, std::vector<IndexPage*> &rec, std::vector<int> &pageIndex) {
    assert(indexPage->getTotalIndex() == indexPage->getCapacity() + 1);
    int lNum = (indexPage->getCapacity() + 1) / 2;
    int rNum = (indexPage->getCapacity() + 1) - lNum;
    int head = indexPage->cut(lNum);
    int newPageId = getNextFreePage(), index;
    IndexPage* newIndexPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, newPageId, index)), indexLen, colType, newPageId);
    newIndexPage->initialize(indexLen, colType);
    bufPageManager->markDirty(index);
    rec.push_back(newIndexPage);
    pageIndex.push_back(index);

    std::vector<void*> removeData;
    std::vector<int> removeVal;
    std::vector<int16_t> removeChildIndex;
    indexPage->removeFrom(head, removeData, removeVal, removeChildIndex);
    newIndexPage->insertFrom(removeData, removeVal, removeChildIndex);
    int siz = removeChildIndex.size();
    for (int i = 0; i < siz; i++) {
        if (removeChildIndex[i] < 0)
            continue;
        IndexPage* childPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, removeChildIndex[i], index)), indexLen, colType, removeChildIndex[i]);
        int fatherPos_;
        transform(fatherPos_, newPageId, i);
        *childPage->getFatherIndex() = fatherPos_;
        rec.push_back(childPage);
        pageIndex.push_back(index);
    }
    *newIndexPage->getNextPage() = *indexPage->getNextPage();
    if (*indexPage->getNextPage() >= 0) {
        IndexPage* indexPageNextPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, *indexPage->getNextPage(), index)), indexLen, colType, *indexPage->getNextPage());
        *indexPageNextPage->getLastPage() = newPageId;
        bufPageManager->markDirty(index);
        rec.push_back(indexPageNextPage);
        pageIndex.push_back(index);
    }
    *indexPage->getNextPage() = newPageId;
    *newIndexPage->getLastPage() = indexPage->getPageId();
    newIndexPage->changePageType(indexPage->getPageType());
    int fatherPos = *indexPage->getFatherIndex(), fatherPageId, fatherSlot;
    IndexPage* fatherPage = nullptr;
    if (fatherPos >= 0) {
        transformR(fatherPos, fatherPageId, fatherSlot);
        fatherPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, fatherPageId, index)), indexLen, colType, fatherPageId);
        fatherPage->changePageType(INDEX_PAGE_INTERIOR);
        bufPageManager->markDirty(index);
        rec.push_back(fatherPage);
        pageIndex.push_back(index);

        memcpy(fatherPage->getData(fatherSlot), indexPage->getData(*indexPage->getLastIndex()), indexLen);
        int insertIndex = fatherPage->insert(newIndexPage->getData(*newIndexPage->getLastIndex()), 0, newPageId);
        int newIndexPageFatherIndex;
        transform(newIndexPageFatherIndex, fatherPageId, insertIndex);
        *newIndexPage->getFatherIndex() = newIndexPageFatherIndex; 
    } else { // new root
        fatherPageId = getNextFreePage();
        fatherPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, fatherPageId, index)), indexLen, colType, fatherPageId);
        fatherPage->initialize(indexLen, colType);
        fatherPage->changePageType(INDEX_PAGE_INTERIOR);
        bufPageManager->markDirty(index);
        int lInsertIndex = fatherPage->insert(indexPage->getData(*indexPage->getLastIndex()), 0, indexPage->getPageId());
        int rInsertIndex = fatherPage->insert(newIndexPage->getData(*newIndexPage->getLastIndex()), 0, newIndexPage->getPageId());
        int lFatherPos, rFatherPos;
        transform(lFatherPos, fatherPageId, lInsertIndex);
        transform(rFatherPos, fatherPageId, rInsertIndex);
        *indexPage->getFatherIndex() = lFatherPos;
        *newIndexPage->getFatherIndex() = rFatherPos;
        root = fatherPage;
        indexHeader->rootPageId = fatherPageId;
        writeIndexTable();
    }
    if (fatherPage->overflow()) {
        dealOverFlow(fatherPage, rec, pageIndex);
    }
}

void BPlusTree::update(IndexPage* indexPage, void* updateData, std::vector<IndexPage*> &rec, std::vector<int> &pageIndex) {
    int fatherIndex = *(indexPage->getFatherIndex());
    if (fatherIndex < 0) {
        return;
    }
    int pageId, slotId, index;
    transformR(fatherIndex, pageId, slotId);
    IndexPage* fatherIndexPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
    void* data = fatherIndexPage->getData(slotId);
    memcpy(data, updateData, indexLen);
    bufPageManager->markDirty(index);
    rec.push_back(fatherIndexPage);
    pageIndex.push_back(index);
    update(fatherIndexPage, updateData, rec, pageIndex);
}

void BPlusTree::search(void* data, std::vector<int> &res) {
    res.clear();
    int pos = searchUpperBound(data), pageId, slotId;
    int curPageId = -1, index;
    IndexPage* cur = nullptr;
    std::vector<IndexPage*> rec;
    std::vector<int> pageIndex;
    while (pos >= 0) {
        transformR(pos, pageId, slotId);
        if (pageId != curPageId) {
            cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
            curPageId = pageId;
            rec.push_back(cur);
            pageIndex.push_back(index);
        }
        if (!cur->getCompare()->equ(data, cur->getData(slotId))) {
            break;
        }
        res.push_back(*cur->getVal(slotId));
        int nextSlot = *(cur->getNextIndex(slotId));
        if (nextSlot < 0 && *(cur->getNextPage()) >= 0) {
            curPageId = *(cur->getNextPage());
            cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, curPageId, index)), indexLen, colType, curPageId);
            rec.push_back(cur);
            pageIndex.push_back(index);
            transform(pos, curPageId, *(cur->getFirstIndex()));
        } else if (nextSlot >= 0){
            transform(pos, curPageId, nextSlot);
        } else {
            pos = -1;
        }
    }
    recycle(rec, pageIndex);
}

void IndexVectorReverse(std::vector<int> &res) {
    int siz = res.size();
    for (int i = 0; i < siz / 2; i++) {
        std::swap(res[i], res[siz - 1 - i]);
    }
}

void BPlusTree::searchBetween(void* ldata, void* rdata, std::vector<int> &res) {
    res.clear();
    if (ldata == nullptr && rdata == nullptr) {
        return;
    }
    int pos, pageId, slotId;
    int curPageId = -1, index;
    IndexPage* cur = nullptr;
    std::vector<IndexPage*> rec;
    std::vector<int> pageIndex;
    if (ldata != nullptr) {
        pos = searchUpperBound(ldata);
        while (pos >= 0) {
            transformR(pos, pageId, slotId);
            if (pageId != curPageId) {
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
                curPageId = pageId;
                rec.push_back(cur);
                pageIndex.push_back(index);
            }
            if (rdata && cur->getCompare()->gt(cur->getData(slotId), rdata)) {
                break;
            }
            res.push_back(*cur->getVal(slotId));
            int nextSlot = *(cur->getNextIndex(slotId));
            if (nextSlot < 0 && *(cur->getNextPage()) >= 0) {
                curPageId = *(cur->getNextPage());
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, curPageId, index)), indexLen, colType, curPageId);
                rec.push_back(cur);
                pageIndex.push_back(index);
                transform(pos, curPageId, *(cur->getFirstIndex()));
            } else if (nextSlot >= 0) {
                transform(pos, curPageId, nextSlot);
            } else {
                pos = -1;
            }
        }
    } else { // rdata != nullptr && ldata == nullptr
        pos = searchUpperBound(rdata);
        if (pos < 0) { // rdata is greater than all the data
                      // under this condition, pos = -pageId
            pageId = -pos;
        } else {
            transformR(pos, pageId, slotId);
        }
        cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
        curPageId = pageId;
        if (pos < 0) {
            slotId = *cur->getLastIndex();
        }
        transform(pos, pageId, slotId);
        rec.push_back(cur);
        pageIndex.push_back(index);
        int lastSlot = *(cur->getLastIndex(slotId)), lastPage, lastPos;
        if (lastSlot < 0 && *(cur->getLastPage()) >= 0) {
            lastPage = *(cur->getLastPage());
            IndexPage* lastIndexPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, lastPage, index)), indexLen, colType, lastPage);
            rec.push_back(lastIndexPage);
            pageIndex.push_back(index);
            transform(lastPos, lastPage, *lastIndexPage->getLastIndex());
        } else if (lastSlot >= 0) {
            lastPage = pageId;
            transform(lastPos, lastPage, lastSlot);
        } else {
            lastPos = -1;
        }
        while (pos >= 0) {
            transformR(pos, pageId, slotId);
            if (pageId != curPageId) {
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
                curPageId = pageId;
                rec.push_back(cur);
                pageIndex.push_back(index);
            }
            if (cur->getCompare()->gt(cur->getData(slotId), rdata)) {
                break;
            }
            res.push_back(*cur->getVal(slotId));
            int nextSlot = *(cur->getNextIndex(slotId));
            if (nextSlot < 0 && *(cur->getNextPage()) >= 0) {
                curPageId = *(cur->getNextPage());
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, curPageId, index)), indexLen, colType, curPageId);
                rec.push_back(cur);
                pageIndex.push_back(index);
                transform(pos, curPageId, *(cur->getFirstIndex()));
            } else if (nextSlot >= 0){
                transform(pos, curPageId, nextSlot);
            } else {
                pos = -1;
            }
        }
        IndexVectorReverse(res);
        pos = lastPos;
        while (pos >= 0) {
            transformR(pos, pageId, slotId);
            if (pageId != curPageId) {
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
                curPageId = pageId;
                rec.push_back(cur);
                pageIndex.push_back(index);
            }
            res.push_back(*cur->getVal(slotId));
            int lastSlot = *(cur->getLastIndex(slotId));
            if (lastSlot < 0 && *(cur->getLastPage()) >= 0) {
                curPageId = *(cur->getLastPage());
                cur = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, curPageId, index)), indexLen, colType, curPageId);
                rec.push_back(cur);
                pageIndex.push_back(index);
                transform(pos, curPageId, *(cur->getLastIndex()));
            } else if (lastSlot >= 0) {
                transform(pos, curPageId, lastSlot);
            } else {
                pos = -1;
            }
        }
        IndexVectorReverse(res);
    }
    recycle(rec, pageIndex);
}

void BPlusTree::insert(void* data, const int val) {
    std::vector<IndexPage*> rec;
    std::vector<int> pageIndex;
    int pos = searchUpperBound(data), pageId, slotId, index;
    IndexPage* indexPage;
    if (pos < 0) {
        pageId = -pos;   
    } else {
        transformR(pos, pageId, slotId);
    }
    indexPage = new IndexPage((uint8_t*)(bufPageManager->getPage(fileId, pageId, index)), indexLen, colType, pageId);
    slotId = indexPage->insert(data, val);
    bufPageManager->markDirty(index);
    rec.push_back(indexPage);
    pageIndex.push_back(index);
    if (slotId == *(indexPage->getLastIndex())) {
        update(indexPage, data, rec, pageIndex);
    }
    if (indexPage->overflow()) {
        dealOverFlow(indexPage, rec, pageIndex);
    }
    recycle(rec, pageIndex, true);
}
