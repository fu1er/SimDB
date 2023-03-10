#include "MyParser.h"
#include <fstream>
#include <iostream>

int main() {
    DatabaseManager* databaseManager = new DatabaseManager();
    MyParser* myParser = new MyParser(databaseManager);

    ifstream input;
    input.open("test.sql", ios::in);

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string content(buffer.str());
    myParser->parse(content);
}
