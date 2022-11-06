#include "IndexManager.h"
#include "unistd.h"

IndexManager::IndexManager(BufPageManager* bufPageManager_, char* databaseName_) {
    bufPageManager = bufPageManager_;
    strcpy(databaseName, databaseName_);
    for (int i = 0; i < DB_MAX_TABLE_NUM; i++) {
        for (int j = 0; j < TAB_MAX_COL_NUM; j++) {
            bPlusTree[i][j] = nullptr;
        }
    }
}

IndexManager::~IndexManager() {
    for (int i = 0; i < DB_MAX_TABLE_NUM; i++) {
        for (int j = 0; j < TAB_MAX_COL_NUM; j++) {
            if (bPlusTree[i][j]) {
                bufPageManager->fileManager->closeFile(fileIds[i][j]);
                delete bPlusTree[i][j];
                bPlusTree[i][j] = nullptr;
            }
        }
    }
}

int IndexManager::findEmptyIndex(int &emptyI, int &emptyJ) {
    for (int i = 0; i < DB_MAX_TABLE_NUM; i++) {
        for (int j = 0; j < TAB_MAX_COL_NUM; j++) {
            if (bPlusTree[i][j] == nullptr) {
                emptyI = i;
                emptyJ = j;
                return 0;
            }
        }
    }
    return -1;
}

BPlusTree* IndexManager::findIndex(const char* tableName, const char* indexName) {
    for (int i = 0; i < DB_MAX_TABLE_NUM; i++) {
        if (strcmp(tableNames[i], tableName) != 0) {
            continue;
        }
        for (int j = 0; j < TAB_MAX_COL_NUM; j++) {
            if (bPlusTree[i][j] && strcmp(indexNames[i][j], indexName) == 0) {
                return bPlusTree[i][j];
            }
        }
    }
    int emptyI, emptyJ;
    if (findEmptyIndex(emptyI, emptyJ) == -1) {
        printf("[ERROR] indexManager can accept no more index.\n");
        return nullptr;
    }
    strcpy(tableNames[emptyI], tableName);
    strcpy(indexNames[emptyI][emptyJ], indexName);
    char fileName[DB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + 30];
    sprintf(fileName, "database/%s_%s_%s.index", databaseName, tableName, indexName);
    if (bufPageManager->fileManager->openFile(fileName, fileIds[emptyI][emptyJ])) {
        bPlusTree[emptyI][emptyJ] = new BPlusTree(fileIds[emptyI][emptyJ], bufPageManager, -1, -1);
        return bPlusTree[emptyI][emptyJ];
    }
    return nullptr;
}

bool IndexManager::hasIndex(const char* tableName, const char* indexName) {
    char fileName[DB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + 30];
    sprintf(fileName, "database/%s_%s_%s.index", databaseName, tableName, indexName);
    return (access(fileName, 0) != -1);
}

int IndexManager::createIndex(const char* tableName, const char* indexName, uint16_t indexLen, uint8_t colType) {
    if (hasIndex(tableName, indexName)) {
        printf("[INFO] index %s already created.\n", indexName);
        return 0;
    }
    char fileName[DB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + 30];
    sprintf(fileName, "database/%s_%s_%s.index", databaseName, tableName, indexName);
    if (bufPageManager->fileManager->createFile(fileName)) {
        int fileId, emptyI, emptyJ;
        if (bufPageManager->fileManager->openFile(fileName, fileId)) {
            if (findEmptyIndex(emptyI, emptyJ) == -1) {
                printf("[ERROR] indexManager can accept no more index.\n");
                return -1;
            }
            fileIds[emptyI][emptyJ] = fileId;
            strcpy(tableNames[emptyI], tableName);
            strcpy(indexNames[emptyI][emptyJ], indexName);
            bPlusTree[emptyI][emptyJ] = new BPlusTree(fileId, bufPageManager, indexLen, colType);
            return 0;
        }
    }
    return -1;
}

int IndexManager::removeIndex(const char* tableName, const char* indexName) {
    if (!hasIndex(tableName, indexName)) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    char fileName[DB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + TAB_MAX_NAME_LEN + 30];
    sprintf(fileName, "database/%s_%s_%s.index", databaseName, tableName, indexName);
    if (bufPageManager->fileManager->removeFile(fileName)) {
        for (int i = 0; i < DB_MAX_TABLE_NUM; i++) {
            if (strcmp(tableNames[i], tableName) != 0) {
                continue;
            }
            for (int j = 0; j < TAB_MAX_COL_NUM; j++) {
                if (bPlusTree[i][j] && strcmp(indexNames[i][j], indexName) == 0) {
                    delete bPlusTree[i][j];
                    bPlusTree[i][j] = nullptr;
                }
            }
        }
        return 0;
    }
    return -1;
}

int IndexManager::insert(const char* tableName, const char* indexName, void* data, const int val) {
    BPlusTree* cur = findIndex(tableName, indexName);
    if (cur == nullptr) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    cur->insert(data, val);
    return 0;
}

int IndexManager::insert(const char* tableName, const char* indexName, std::vector<void*> data, std::vector<int> val) {
    BPlusTree* cur = findIndex(tableName, indexName);
    if (cur == nullptr) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    if (data.size() != val.size()) {
        printf("[ERROR] size of data and val don't match.\n");
        return -1;
    }
    int siz = data.size();
    for (int i = 0; i < siz; i++) {
        cur->insert(data[i], val[i]);
    }
    return 0;
}

int IndexManager::search(const char* tableName, const char* indexName, void* data, std::vector<int> &res) {
    BPlusTree* cur = findIndex(tableName, indexName);
    if (cur == nullptr) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    cur->search(data, res);
    return 0;
}

int IndexManager::searchBetween(const char* tableName, const char* indexName, void* lData, void* rData, std::vector<int> &res) {
    BPlusTree* cur = findIndex(tableName, indexName);
    if (cur == nullptr) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    cur->searchBetween(lData, rData, res);
    return 0;
}

int IndexManager::remove(const char* tableName, const char* indexName, void* data) {
    BPlusTree* cur = findIndex(tableName, indexName);
    if (cur == nullptr) {
        printf("[ERROR] index %s has not been created.\n", indexName);
        return -1;
    }
    cur->remove(data);
    return 0;
}