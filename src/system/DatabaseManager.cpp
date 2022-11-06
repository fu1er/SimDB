#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "DatabaseManager.h"

DatabaseManager::DatabaseManager() {
    BASE_PATH = "./DATABASE";
    tableManager = nullptr;
}

inline bool DatabaseManager::checkName(string name) {
    size_t length = name.length();
    if(length == 0 || length > DB_MAX_NAME_LEN) {
        printf("[Error] valid database name !");
        return false;
    }
    return true;
}

int DatabaseManager::createDatabase(string name) {
    if(!checkName(name))
        return -1;

    string path = BASE_PATH + name + '/';
    if(!mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH)) // 774
        return 0;
    
    printf("[Error] fail to create the database %s\n", name);
    return -1;
}

int DatabaseManager::dropDatabase(string name) {
    if(!checkName(name))
        return -1;

    string path = BASE_PATH + name + '/';
    system(("rm -rf " + path).c_str());
    return 0;
}

int DatabaseManager::switchDatabase(string name) {
    if(!checkName(name))
        return -1;

    string path = BASE_PATH + name + '/';
    tableManager = new TableManager(path);
    return 0;
}