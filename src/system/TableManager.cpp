#include "TableManager.h"
#include <unistd.h>

TableManager::TableManager(string databaseName_,  BufPageManager* bufPageManager_) {
    databaseName = databaseName_;
    recordManager = new RecordManager(bufPageManager_, databaseName.c_str());
}

inline bool TableManager::checkTableName(string name) {
    size_t length = name.length();
    if(length == 0 || length > TAB_MAX_NAME_LEN) {
        printf("[Error] invalid table name ! \n");
        return false;
    }
    return true;
}

inline bool TableManager::checkTableExist(string path) {
    if (!access(path.c_str(), F_OK))
        return true; // already exit
    return false;
}

int TableManager::creatTable(string tableName, TableEntry* tableEntrys, int colNum) {
    if(!checkTableName(tableName))
        return -1;
    string path = "database/" + databaseName + '/' + tableName +".db";
    if(checkTableExist(path)) {
        printf("[Error] table named %s already exist!\n", tableName.c_str());
        return -1;
    }
    if(recordManager->createFile(tableName.c_str()) != 0){
        printf("report error when create file in table manager\n");
        return -1;
    }
    if(recordManager->openFile(tableName.c_str(), fileHandler) != 0) {
        printf("report error when open file in tablemanager\n");
        return -1;
    }

    fileHandler->operateTable(TB_INIT, nullptr, tableEntrys, colNum);

}

int TableManager::openTable(string name) {
    if(!checkTableName(name))
        return -1;
    string path = "database/" + databaseName + '/' + name +".db";
    if(!checkTableExist(path)) {
        printf("[Error] table dose not exist!\n");
        return -1;
    }

    if(recordManager->openFile(name.c_str(), fileHandler) != 0) {
        printf("[Error] table %s has already been opened. \n", name.c_str());
        return -1;
    }

    return 0;
}

int TableManager::dropTable(string name) {
    if(!checkTableName(name))
        return -1;
    string path = "database/" + databaseName + '/' + name +".db";
    if(!checkTableExist(path)) {
        printf("[Error] table dose not exist!\n");
        return -1;
    }
        
    if(recordManager->removeFile(name.c_str()) != 0) {
        printf("[Error] fail to drop the table named %s\n", name.c_str());
        return -1;
    }

    return 0;

}

int TableManager::listTableInfo(string name) {
    if(!checkTableName(name))
        return -1;
    string path = "database/" + databaseName + '/' + name +".db";
    if(!checkTableExist(path))
        return -1;
    fileHandler = recordManager->findTable(name.c_str());
    if(fileHandler == nullptr)
        return -1;
    TableHeader* tableHeader = fileHandler->getTableHeader();
    printf("======================begin======================\n");
    for(int i = 0; i < tableHeader->colNum; i++) {
        printf("%64s|", tableHeader->entrys[i].colName);
    }
    printf("\n===============================================\n");
    for(int i = 0; i < tableHeader->colNum; i++) {
        switch (tableHeader->entrys[i].colType)
        {
        case 0:
            printf("NULL|");
            break;
        case 1:
            printf("INT|");
        case 2:
            printf("VARCHAR(%d)|", tableHeader->entrys[i].colLen);
        case 3:
            printf("FLOAT|");
        default:
            break;
        }
    }
    printf("\n=====================end======================\n");
    return 0;
}

int TableManager::renameTable(string oldName, string newName) {
    if(!checkTableName(newName))
        return -1;

    string oldPath = "database/" + databaseName + '/' + oldName +".db";
    if(!checkTableExist(oldPath)) {
        printf("[Error] table named %s does not exist !\n", oldName.c_str());
        return -1;
    }

    fileHandler = recordManager->findTable(oldName.c_str());
    if(fileHandler == nullptr) {
        printf("[Error] can not find the table named %s !\n", oldName.c_str());
        return -1;
    }
    if(recordManager->closeFile(fileHandler) != 0) {
        printf("[Error] can not close the file before rename it !\n");
        return-1;
    }
    fileHandler = nullptr;

    string newPath = "database/" + databaseName + '/' + newName +".db";
    int ret = rename(oldPath.c_str(), newPath.c_str());
    if(ret != 0) {
        printf("[Error] can not rename the table !\n");
        return -1;
    }
    
    if(recordManager->openFile(newName.c_str(), fileHandler) != 0) {
        printf("[Error] can not open the file after rename it !\n");
        return -1;
    }
    return 0;
}

int TableManager::saveChangeToFile(const char* tableName) {
    fileHandler = recordManager->findTable(tableName);
    recordManager->closeFile(fileHandler);
}
