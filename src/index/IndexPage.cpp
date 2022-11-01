#include "IndexPage.h"

void IndexPageHeader::initialize(uint16_t indexLen_, uint8_t colType_) {
    firstIndex = -1;
    firstEmptyIndex = -1;
    nextPage = -1;
    lastPage = -1;
    totalIndex = 0;
    indexLen = indexLen_;
    isInitialized = 1;
    colType = colType_;
    pageType = INDEX_PAGE_LEAF;
}

inline uint16_t IndexPageHeader::getTotalLen() {
    // nextIndex + lastIndex + childIndex + sizeof(in16_t) + content + val
    // nextDataIndex(uint32_t) + lastDataIndex(uint32_t)
    return 4 * sizeof(int16_t) + indexLen + sizeof(int);
}

IndexPage::IndexPage(uint8_t* pageData, uint16_t indexLen, uint8_t colType, uint16_t pageId_) {
    data = pageData;
    indexPageHeader = (IndexPageHeader*)data;
    if (indexPageHeader->isInitialized == 0) { // uninitialized but not zero?
        indexPageHeader->initialize(indexLen, colType);
    }
    capacity = ((PAGE_SIZE - sizeof(IndexPageHeader)) / indexPageHeader->getTotalLen()) - 1;
    pageId = pageId_;
    switch (colType) {
        case COL_INT:
            compare = new IntCompare();
            break;
        case COL_FLOAT:
            compare = new FloatCompare();
            break;
        case COL_VARCHAR:
            compare = new CharCompare();
            break;
        default:
            compare = nullptr;
            break;
    }
}

IndexPage::~IndexPage() {
    if (compare) {
        delete compare;
    }
}

uint16_t IndexPage::getCapacity() {
    return capacity;
}

Compare* IndexPage::getCompare() {
    return compare;
}

void IndexPage::changePageType(uint8_t newPageType) {
    indexPageHeader->pageType = newPageType;
}

uint8_t IndexPage::getPageType() {
    return indexPageHeader->pageType;
}

uint16_t IndexPage::getPageId() {
    return pageId;
}

bool IndexPage::overflow() {
    return indexPageHeader->totalIndex > capacity;
}

bool IndexPage::underflow() {
    return indexPageHeader->totalIndex < (capacity + 1) / 2;
}

uint8_t* IndexPage::accessData(int id) {
    if (id < 0 || id > capacity) {
        // id == capacity will result in overflow
        printf("[ERROR] fetch slot with wrong id.\n");
        return nullptr;
    }
    uint16_t totalLen = indexPageHeader->getTotalLen();
    return data + sizeof(indexPageHeader) + id * totalLen;
}

void* IndexPage::getData(int id) {
    uint8_t* data = accessData(id);
    return data + 4 * sizeof(int16_t);
}

uint32_t IndexPage::getNextDataIndex(int id) {
    uint8_t* data = accessData(id);
    return ((uint32_t*)data)[0];
}

uint32_t IndexPage::getLastDataIndex(int id) {
    uint8_t* data = accessData(id);
    return ((uint32_t*)data)[1];
}

int IndexPage::getVal(int id) {
    uint8_t* data = accessData(id);
    return *((int*)(data + 4 * sizeof(int16_t) + indexPageHeader->indexLen));
}

int16_t IndexPage::getNextIndex(int id) {
    uint8_t* data = accessData(id);
    return *((int16_t*)data);
}

int16_t IndexPage::getLastIndex(int id) {
    uint8_t* data = accessData(id);
    return *((int16_t*)(data + sizeof(int16_t)));
}

int16_t IndexPage::getChildIndex(int id) {
    uint8_t* data = accessData(id);
    return *((int16_t*)(data + 2 * sizeof(int16_t)));
}

int IndexPage::insert(void* data, const int val, const int16_t childIndex_) {
    int16_t head = indexPageHeader->firstIndex, last = -1;
    int16_t emptyIndex = indexPageHeader->firstEmptyIndex;
    indexPageHeader->totalIndex++;
    if (emptyIndex < 0) {
        // new empty slot, create and initialize
        emptyIndex = indexPageHeader->totalIndex;
        uint8_t* newEmptySlot = accessData(emptyIndex);
        ((int16_t*)newEmptySlot)[0] = -1;
    }
    uint8_t* emptySlot = accessData(emptyIndex);
    indexPageHeader->firstEmptyIndex = ((int16_t*)emptySlot)[0];

    if (head < 0) {
        indexPageHeader->firstIndex = emptyIndex;
    } else {
        while (head >= 0) {
            if (compare->lte(data, getData(head))) {
                break;
            }
            last = head;
            head = getNextIndex(head);
        }
    }

    int16_t* nextIndex = (int16_t*)emptySlot;
    int16_t* lastIndex = ((int16_t*)emptySlot) + 1;
    int16_t* childIndex = ((int16_t*)emptySlot) + 2;
    void* emptyData = emptySlot + 4 * sizeof(int16_t);
    int* emptyVal = (int*)(emptySlot + 4 * sizeof(int16_t) + indexPageHeader->indexLen);
    *nextIndex = head;
    *lastIndex = last;
    if (last >= 0) {
        uint8_t* lastSlot = accessData(last);
        ((int16_t*)lastSlot)[0] = emptyIndex;
    }
    if (head >= 0) {
        uint8_t* headSlot = accessData(head);
        ((int16_t*)headSlot)[1] = emptyIndex;
    }
    *childIndex = childIndex_;
    *emptyVal = val;
    memcpy(emptyData, data, indexPageHeader->indexLen);
    return emptyIndex;
}

void IndexPage::searchEQ(void* data, std::vector<int> &res) {
    int16_t head = indexPageHeader->firstIndex;
    while (head >= 0) {
        if (compare->equ(data, getData(head))) {
            res.push_back(head);
        } else if (compare->lt(data, getData(head))) {
            break;
        }
    }
}

int IndexPage::searchLowerBound(void* data) {
    int16_t head = indexPageHeader->firstIndex, last = -1;
    while (head >= 0) {
        if (compare->lt(data, getData(head))) {
            break;
        }
        last = head;
        head = getNextIndex(head);
    }
    return last;
}

int IndexPage::searchUpperBound(void* data) {
    int16_t head = indexPageHeader->firstIndex;
    while (head >= 0) {
        if (compare->lte(data, getData(head))) {
            break;
        }
    }
    return head;
}

void IndexPage::remove(void* data, std::vector<int> &res) {
    int16_t head = indexPageHeader->firstIndex, last = -1;
    while (head >= 0) {
        int next = getNextIndex(head);
        if (compare->equ(data, getData(head))) {
            res.push_back(head);
            if (last >= 0) {
                uint8_t* lastData = accessData(last);
                ((int16_t*)lastData)[0] = next;
            }
            if (next >= 0) {
                uint8_t* nextData = accessData(next);
                ((int16_t*)(nextData + sizeof(int16_t)))[0] = last;
            }
            uint8_t* emptyData = accessData(head);
            // set empty index
            ((int16_t*)emptyData)[0] = indexPageHeader->firstEmptyIndex;
            indexPageHeader->firstEmptyIndex = head;

            indexPageHeader->totalIndex--;
            head = next;
        } else {
            last = head;
            head = next;
        }
    }
}
