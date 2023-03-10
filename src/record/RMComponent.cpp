#include "RMComponent.h"
#include "../filesystem/utils/pagedef.h"
#include "cstring"

TableEntry::TableEntry() {}

TableEntry::TableEntry(const char* colName_, uint8_t colType_, bool checkConstraint_, bool primaryKeyConstraint_, \
               uint8_t foreignKeyConstraint_, uint32_t colLen_, bool hasDefault_, \
               bool notNullConstraint_, bool uniqueConstraint_, bool isNull_) {
    colType = colType_;
    strcpy(colName, colName_);
    // strncpy(colName, colName_, strlen(colName_));
    checkConstraint = checkConstraint_;
    primaryKeyConstraint = primaryKeyConstraint_;
    foreignKeyConstraint = foreignKeyConstraint_;
    colLen = colLen_;
    hasDefault = hasDefault_;
    notNullConstraint = notNullConstraint_;
    uniqueConstraint = uniqueConstraint_;
    isNull = isNull_;
    switch (colType) {
        case COL_INT:
            colLen = sizeof(int);
            break;
        case COL_FLOAT:
            colLen = sizeof(float);
            break;
        case COL_NULL:
            colLen = 0;
            break;
        default:
            break;
    }
    next = -1;
}

bool TableEntry::verifyConstraint(RecordDataNode* recordDataNode) {
    if (notNullConstraint) {
        if (recordDataNode->nodeType == COL_NULL)
            return false;
    } else if (checkConstraint) {
        switch (recordDataNode->nodeType) {
            case COL_INT: {
                int intContent = *(recordDataNode->content.intContent);
                for (uint32_t i = 0; i < checkKeyNum; i++) {
                    int checkInt = *(checkContent.checkInt + i);
                    switch (*(checkType + i)) {
                        case EQUAL:
                            if (intContent != checkInt) return false;
                            break;
                        case LESS:
                            if (intContent >= checkInt) return false;
                            break;
                        case LESS_EQUAL:
                            if (intContent > checkInt) return false;
                            break;
                        case GREATER:
                            if (intContent <= checkInt) return false;
                            break;
                        case GREATER_EQUAL:
                            if (intContent < checkInt) return false;
                            break;
                        case NOT_EQUAL:
                            if (intContent == checkInt) return false;
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
            case COL_FLOAT: {
                float floatContent = *(recordDataNode->content.floatContent);
                for (uint32_t i = 0; i < checkKeyNum; i++) {
                    int checkFloat = *(checkContent.checkFloat + i);
                    switch (*(checkType + i)) {
                        case EQUAL:
                            if (floatContent != checkFloat) return false;
                            break;
                        case LESS:
                            if (floatContent >= checkFloat) return false;
                            break;
                        case LESS_EQUAL:
                            if (floatContent > checkFloat) return false;
                            break;
                        case GREATER:
                            if (floatContent <= checkFloat) return false;
                            break;
                        case GREATER_EQUAL:
                            if (floatContent < checkFloat) return false;
                            break;
                        case NOT_EQUAL:
                            if (floatContent == checkFloat) return false;
                            break;
                        default:
                            break;
                    }
                }
                break;
            }     
            case COL_VARCHAR: {
                // char *charContent, *checkVarchar;
                // strcpy(charContent, recordDataNode->content.charContent);
                // strcpy(checkVarchar, checkContent.checkVarchar);
                break;
            }
            default:
                break;
        }   
    }
    return true;
}

void TableEntry::fillDefault(RecordDataNode* data) {
    if (hasDefault && data->nodeType == COL_NULL) {
        switch (colType) {
            case COL_INT:
                data->len = sizeof(int);
                data->nodeType = COL_INT;
                *(data->content.intContent) = defaultVal.defaultValInt;
                break;
            case COL_FLOAT:
                data->len = sizeof(float);
                data->nodeType = COL_FLOAT;
                *(data->content.floatContent) = defaultVal.defaultValFloat;
                break;
            case COL_VARCHAR:
                data->len = colLen;
                data->nodeType = COL_VARCHAR;
                memcpy(data->content.charContent, defaultVal.defaultValVarchar, data->len);
        }
    }
}

TableHeader::TableHeader() {
    valid = 0;
    colNum = 0;
    entryHead = -1;
    firstNotFullPage = -1;
    totalPageNumber = 1;
    recordSize = 0;
    recordLen = 0;
    recordNum = 0;
}

bool TableHeader::existCol(char* colName) {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (colName == nullptr) {
            printf("Error1\n");
            return false;
        }
        if (entry.colName == nullptr) {
            printf("Error2\n");
            return false;
        }
        if (strcmp(colName, entry.colName) == 0) {
            return true;
        }
        head = entry.next;
    }
    return false;
}

void TableHeader::printInfo() {

    printf("Table name: %s;\n", tableName);
    printf("Column size: %d;\n", colNum);
    printf("Record Length: %d;\n", recordLen);
    printf("Record size: %d;\n", recordSize);

    int8_t head = entryHead;
    TableEntry entry;
    int cnt = 0;
    int uniqueKeyCnt = 0;
    int primaryKeyCnt = 0;
    while (head >= 0) {
        entry = entrys[head];
        printf("[column %d] name = %s, type = ", ++cnt, entry.colName);
        if (entry.uniqueConstraint) {
            uniqueKeyCnt++;
        }
        if (entry.primaryKeyConstraint) {
            primaryKeyCnt++;
        }
        switch (entry.colType) {
            case COL_INT:
                printf("INT");
                if (entry.hasDefault) {
                    printf(", Default value = %d;", entry.defaultVal.defaultValInt);
                } else {
                    printf(", Default value = Null;");
                }
                break;
            case COL_VARCHAR:
                printf("VARCHAR(%d)", entry.colLen - 1);
                if (entry.hasDefault) {
                    printf(", Default value = %s;", entry.defaultVal.defaultValVarchar);
                } else {
                    printf(", Default value = Null;");
                }
                break;
            case COL_FLOAT:
                printf("FLOAT");
                if (entry.hasDefault) {
                    printf(", Default value = %f;", entry.defaultVal.defaultValFloat);
                } else {
                    printf(", Default value = Null;");
                }
                break;
            case COL_NULL:
                printf("NULL;");
                break;
            default:
                fprintf(stderr, "Error Type");
                break;
        }
        // if (entry.notNullConstraint) {
        //     printf(", not NULL");
        // }
        printf("\n");
        head = entry.next;
    }
    if (primaryKeyCnt > 0) {
        printf("PRIMARY KEY (");
        head = entryHead;
        while (head >= 0) {
            entry = entrys[head];
            if (entry.primaryKeyConstraint) {
                printf("%s", entry.colName);
                primaryKeyCnt--;
                if (primaryKeyCnt > 0) {
                    printf(", ");
                }
            }
            head = entry.next;
        }
        printf(");\n");
    }
    if (uniqueKeyCnt > 0) {
        printf("UNIQUE KEY (");
        head = entryHead;
        while (head >= 0) {
            entry = entrys[head];
            if (entry.uniqueConstraint) {
                printf("%s", entry.colName);
                uniqueKeyCnt--;
                if (uniqueKeyCnt > 0) {
                    printf(", ");
                }
            }
            head = entry.next;
        }
        printf(");\n");
    }
}

int TableHeader::getCol(const char* colName, TableEntry& tableEntry) {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (strcmp(entry.colName, colName) == 0) {
            tableEntry = entrys[head];
            return 0;
        }
        head = entry.next;
    }
    return -1;
}

int TableHeader::getCol(const char* colName) {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (strcmp(entry.colName, colName) == 0) {
            return head;
        }
        head = entry.next;
    }
    return -1;
}

int TableHeader::getCol(int colId, char* colName) {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (colId == 0) {
            strcpy(colName, entry.colName);
            return 0;
        }
        colId--;
        head = entry.next;
    }
    return -1;
}

bool TableHeader::hasPrimaryKey() {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (entry.primaryKeyConstraint) {
            return true;
        }
        head = entry.next;
    }
    return false;
}

int TableHeader::alterCol(TableEntry* tableEntry) {
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (strcmp(entry.colName, tableEntry->colName) == 0) {
            int8_t tmpNext = entry.next;
            memcpy(&(entrys[head]), tableEntry, sizeof(TableEntry));
            entrys[head].next = tmpNext;
            calcRecordSizeAndLen();
            return 0;
        }
        head = entry.next;
    }
    printf("[ERROR] column with specified name dose not exist.\n");
    return -1;
}

int TableHeader::removeCol(char* colName) {
    int8_t head = entryHead, prev = -1;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (strcmp(entry.colName, colName) == 0) {
            if (prev >= 0) {
                entrys[prev].next = entrys[head].next;
            } else {
                entryHead = entrys[head].next;
            }
            calcRecordSizeAndLen();
            colNum--;
            return 0;
        }
        prev = head;
        head = entry.next;
    }
    printf("[ERROR] column with specified name dose not exist.\n");
    return -1;
}

int TableHeader::findFreeCol() {
    bool check[TAB_MAX_COL_NUM] = {false};
    int8_t head = entryHead;
    while (head >= 0) {
        check[head] = true;
        head = entrys[head].next;
    }
    for (int i = 0; i < TAB_MAX_COL_NUM; i++) {
        if (!check[i])
            return i;
    }
    return -1;
}

int TableHeader::addCol(TableEntry* tableEntry) {
    if (existCol(tableEntry->colName)) {
        printf("[ERROR] Column with same name already exists.\n");
        return -1;
    }
    int8_t head = entryHead;
    TableEntry entry;

    int freeCol = findFreeCol();
    if (freeCol < 0) {
        printf("[ERROR] no free column.\n");
        return -1;
    }
    memcpy(&entrys[freeCol], tableEntry, sizeof(TableEntry));
    entrys[freeCol].next = -1;
    if (head < 0) {
        entryHead = freeCol;
    } else {
        entry = entrys[head];
        while (entry.next >= 0) {
            head = entry.next;
            entry = entrys[head];
        }
        entrys[head].next = freeCol;
    }
    colNum++;
    calcRecordSizeAndLen();
    return 0;
}

void TableHeader::init(TableEntry* entryHead_, int num) {
    entryHead = 0;
    for (int i = 0; i < num; i++) {
        entrys[i] = entryHead_[i];
        entrys[i].next = (i + 1) == num ? -1 : (i + 1);
    }
    colNum = num;
    calcRecordSizeAndLen();
    valid = 1;
}

void TableHeader::calcRecordSizeAndLen() {
    int8_t head = entryHead;
    TableEntry entry;
    recordLen = sizeof(uint16_t) + sizeof(uint16_t); // record if slot is free + bitmap
    while (head >= 0) {
        entry = entrys[head];
        recordLen += entry.colLen;
        head = entry.next;
    }
    if (recordLen > 0) {
        recordSize = (PAGE_SIZE - sizeof(PageHeader)) / recordLen;
    }
}

TableEntryDesc::~TableEntryDesc() {
    TableEntryDescNode* cur = head;
    while (cur) {
        TableEntryDescNode* next = cur->next;
        delete cur;
        cur = next;
    }
}

TableEntryDesc TableHeader::getTableEntryDesc() {
    TableEntryDesc res;
    TableEntryDescNode* cur;
    TableEntryDescNode* prev = nullptr;
    int8_t head = entryHead;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        cur = new TableEntryDescNode();
        cur->colType = entry.colType;
        cur->colLen = entry.colLen;
        cur->next = nullptr;
        if (prev) {
            prev->next = cur;
        } else {
            res.head = cur;
        }
        prev = cur;
        head = entry.next;
    }
    return res;
}

bool TableHeader::verifyConstraint(const RecordData& recordData) {
    int8_t head = entryHead;
    RecordDataNode* cur = recordData.head;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        if (!entry.verifyConstraint(cur)) {
            return false;
        }
        head = entry.next;
        cur = cur->next;
    }
    if (cur != nullptr) {
        printf("[ERROR] unmatched size between record and TableHeader.\n");
        return false;
    }
    return true;
}

bool TableHeader::verifyConstraint(Record& record) {
    RecordData recordData;
    TableEntryDesc tableEntryDesc = getTableEntryDesc();
    if (record.deserialize(recordData, tableEntryDesc)) {
        return verifyConstraint(recordData);
    }
    return false;
}

bool TableHeader::fillDefault(RecordData& recordData) {
    int8_t head = entryHead;
    RecordDataNode* cur = recordData.head;
    TableEntry entry;
    while (head >= 0) {
        entry = entrys[head];
        entry.fillDefault(cur);
        head = entry.next;
        cur = cur->next;
    }
    if (cur != nullptr) {
        printf("[ERROR] unmatched size between record and TableHeader.\n");
        return false;
    }
    return true;
}

bool TableHeader::fillDefault(Record& record) {
    RecordData recordData;
    TableEntryDesc tableEntryDesc = getTableEntryDesc();
    if (record.deserialize(recordData, tableEntryDesc)) {
        fillDefault(recordData);
        recordData.serialize(record);
        return true;
    } else {
        printf("[ERROR] fail to deserialize record.\n");
    }
    return false;
}

size_t TableEntryDesc::getLen() {
    if (len > 0)
        return len;
    size_t len_ = sizeof(uint16_t); // bitmap
    TableEntryDescNode* cur = head;
    while (cur) {
        switch (cur->colType) {
            case COL_INT:
                len_ += sizeof(int);
                break;
            case COL_FLOAT:
                len_ += sizeof(float);
                break;
            case COL_VARCHAR:
                len_ += cur->colLen;
                break;
            default:
                break;
        }
        cur = cur->next;
    }
    return len = len_;
}

TableEntryDescNode* TableEntryDesc::getCol(unsigned int i) {
    TableEntryDescNode* cur = head;
    while (cur) {
        if (i == 0) {
            return cur;
        }
        cur = cur->next;
        i--;
    }
    return nullptr;
}

bool Record::deserialize(RecordData& rData, TableEntryDesc& tableEntryDesc) {
    if (len != tableEntryDesc.getLen()) {
        printf("[ERROR] unmatched length between record and recordData.\n");
        return false;
    }
    
    RecordDataNode* cur;
    RecordDataNode* prev = nullptr;
    TableEntryDescNode* now = tableEntryDesc.head;
    uint8_t* buf = data + sizeof(uint16_t); // bitmap
    size_t offset;
    int index = 0;
    
    while (now) {
        cur = new RecordDataNode();
        switch(now->colType) {
            case COL_INT:
                offset = sizeof(int);
                cur->nodeType = COL_INT;
                cur->len = offset;
                cur->content.intContent = new int;
                memcpy(cur->content.intContent, buf, offset);
                break;
            case COL_FLOAT:
                offset = sizeof(float);
                cur->nodeType = COL_FLOAT;
                cur->len = offset;
                cur->content.floatContent = new float;
                memcpy(cur->content.floatContent, buf, offset);
                break;
            case COL_VARCHAR:
                offset = now->colLen;
                cur->nodeType = COL_VARCHAR;
                cur->len = offset;
                cur->content.charContent = new char[offset];
                memcpy(cur->content.charContent, buf, offset);
                break;
            default:
                return false;
                break;
        }
        if (isNull(index++)) {
            cur->nodeType = COL_NULL;
        }
        if (prev) {
            prev->next = cur;
        } else {
            rData.head = cur;
        }
        prev = cur;
        buf += offset;
        now = now->next;
    }
    return true;
}

RecordData::~RecordData() {
    RecordDataNode* cur = head;
    while (cur) {
        RecordDataNode* next = cur->next;
        delete cur;
        cur = next;
    }
}

RecordData::RecordData(const RecordData &other) {
    RecordDataNode* temp = other.head;
    if (temp) {
        head = new RecordDataNode();
    } else {
        return;
    }
    RecordDataNode* cur = head;
    while (1) {
        cur->len = temp->len;
        cur->nodeType = temp->nodeType;
        switch (cur->nodeType) {
            case COL_INT:
                cur->content.intContent = new int;
                *(cur->content.intContent) = *(temp->content.intContent);
                break;
            case COL_FLOAT:
                cur->content.floatContent = new float;
                *(cur->content.floatContent) = *(temp->content.floatContent);
                break;
            case COL_VARCHAR:
                cur->content.charContent = new char[cur->len];
                strcpy(cur->content.charContent, temp->content.charContent);
                break; 
        }
        if (temp->next) {
            cur->next = new RecordDataNode();
        } else {
            break;
        }
        temp = temp->next;
        cur = cur->next;
    }
}

RecordData::RecordData(int len) {
    if (len < 0) {
        fprintf(stderr, "cannot construct RecordData with negative length.\n");
        assert(false);
    } else if (len == 0) {
        return;
    }
    head = nullptr;
    RecordDataNode* cur = new RecordDataNode();
    head = cur;
    while (1) {
        cur->len = 0;
        len--;
        if (len > 0) {
            cur->next = new RecordDataNode();
            cur = cur->next;
        } else {
            break;
        }
    }
}

bool RecordData::serialize(Record& record) {
    if (getLen() != record.len) {
        printf("[ERROR] unmatched length between recordData and record.\n");
        return false;
    }

    record.clearBitmap();
    RecordDataNode* cur = head;
    uint8_t* buf = record.data + sizeof(uint16_t);
    size_t offset;
    int index = 0;
    while (cur) {
        switch (cur->nodeType) {
            case COL_INT:
                offset = sizeof(int);
                memcpy(buf, (uint8_t*)(cur->content.intContent), offset);
                break;
            case COL_FLOAT:
                offset = sizeof(float);
                memcpy(buf, (uint8_t*)(cur->content.floatContent), offset);
                break;
            case COL_VARCHAR:
                offset = cur->len;
                memcpy(buf, (uint8_t*)(cur->content.charContent), offset);
                break;
            case COL_NULL:
                offset = cur->len;
                record.setItemNull(index);
                break;
            default:
                return false;
                break;
        }
        buf += offset;
        cur = cur->next;
        index++;
    }
    return true;
}

size_t RecordData::getLen() {
    if (recordLen > 0)
        return recordLen;
    size_t len = sizeof(uint16_t); // bitmap
    RecordDataNode* cur = head;
    while (cur) {
        switch (cur->nodeType) {
            case COL_INT:
                len += sizeof(int);
                break;
            case COL_FLOAT:
                len += sizeof(float);
                break;
            case COL_VARCHAR:
            case COL_NULL:
                len += cur->len;
                break;
            default:
                break;
        }
        cur = cur->next;
    }
    return recordLen = len;
}

RecordDataNode* RecordData::getData(unsigned int i) {
    RecordDataNode* cur = head;
    while (cur) {
        if (i == 0) {
            return cur;
        }
        cur = cur->next;
        i--;
    }
    return nullptr;
}
