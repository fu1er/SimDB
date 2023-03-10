#ifndef __MYSQLVisitor_H__
#define __MYSQLVisitor_H__

#include "antlr4/SQLBaseVisitor.h"
#include "SQLOptimizer.h"
#include <cstdio>
#include <chrono>
#include <ctime>


class MySQLVisitor : public SQLBaseVisitor {
private:
    DatabaseManager* databaseManager;
public:
    MySQLVisitor(DatabaseManager* databaseManager_) {
        databaseManager = databaseManager_;
    }

    ~MySQLVisitor() {}

    std::any visitProgram(SQLParser::ProgramContext *ctx) override {
        fprintf(stderr, "Visit Program.\n");
        return visitChildren(ctx);
    }

    std::any visitStatement(SQLParser::StatementContext *ctx) override {
        fprintf(stderr, "Visit Statement.\n");
        return visitChildren(ctx);
    }

    std::any visitCreate_db(SQLParser::Create_dbContext *ctx) override {
        fprintf(stderr, "Visit Create DB.\n");
        fprintf(stderr, "Database name = %s.\n", ctx->Identifier()->getText().c_str());
        
        return databaseManager->createDatabase(ctx->Identifier()->getText());
    }

    std::any visitDrop_db(SQLParser::Drop_dbContext *ctx) override {
        fprintf(stderr, "Visit Drop DB.\n");
        fprintf(stderr, "Database name = %s.\n", ctx->Identifier()->getText().c_str());
        
        return databaseManager->dropDatabase(ctx->Identifier()->getText());
    }

    std::any visitShow_dbs(SQLParser::Show_dbsContext *ctx) override {
        fprintf(stderr, "Visit Show DB.\n");
        return databaseManager->showDatabases();
    }

    std::any visitUse_db(SQLParser::Use_dbContext *ctx) override {
        fprintf(stderr, "Visit Use DB.\n");
        return databaseManager->switchDatabase(ctx->Identifier()->getText());
    }

    std::any visitShow_tables(SQLParser::Show_tablesContext *ctx) override {
        fprintf(stderr, "Visit Show DB tables.\n");
        return databaseManager->listTablesOfDatabase();
    }

    std::any visitShow_indexes(SQLParser::Show_indexesContext *ctx) override {
        fprintf(stderr, "Visit Show DB indexes.\n");
        return databaseManager->showIndex();
    }

    std::any visitLoad_data(SQLParser::Load_dataContext *ctx) override {
        // TODO
        return visitChildren(ctx);
    }

    std::any visitDump_data(SQLParser::Dump_dataContext *ctx) override {
        // TODO
        return visitChildren(ctx);
    }

    std::any visitCreate_table(SQLParser::Create_tableContext *ctx) override {
        fprintf(stderr, "Visit Create Table.\n");
        fprintf(stderr, "Table name = %s.\n", ctx->Identifier()->getText().c_str());
        fprintf(stderr, "Number of field = %ld.\n", ctx->field_list()->field().size());

        std::string tableName = ctx->Identifier()->getText();
        std::vector<FieldItem> fieldList;
        std::vector<FieldItem> normalFieldList = std::vector<FieldItem>();
        std::vector<FieldItem> pkFieldList = std::vector<FieldItem>();
        std::vector<FieldItem> fkFieldList = std::vector<FieldItem>();
        
        fieldList = std::any_cast<std::vector<FieldItem>>(ctx->field_list()->accept(this));
        for(int i = 0; i < fieldList.size(); i++) {
            if(fieldList[i].isNormalField)
                normalFieldList.push_back(fieldList[i]);
            else if(fieldList[i].isPkField)
                pkFieldList.push_back(fieldList[i]);
            else if(fieldList[i].isFkField)
                fkFieldList.push_back(fieldList[i]);
            else
                printf("report error when telling different fields in visit create table\n");
        }
        databaseManager->createTable(tableName, normalFieldList);

        if(pkFieldList.size() > 0) {
            if(pkFieldList.size() > 1)
                printf("multiple primary key"); // actually report error
            databaseManager->createPrimaryKey(tableName, pkFieldList[0].colNames, pkFieldList[0].colNames.size());
        }

        for(int i = 0; i < fkFieldList.size(); i++) {
            databaseManager->createForeignKey(tableName, fkFieldList[i].fkName, fkFieldList[i].colName, \
                                              fkFieldList[i].refTableName, fkFieldList[i].refColName);
        }

        return 0;
    }

    std::any visitDrop_table(SQLParser::Drop_tableContext *ctx) override {
        fprintf(stderr, "Visit Drop Table.\n");
        fprintf(stderr, "Table name = %s.\n", ctx->Identifier()->getText().c_str());
        
        databaseManager->dropTable(ctx->Identifier()->getText());
        return visitChildren(ctx);
    }

    std::any visitDescribe_table(SQLParser::Describe_tableContext *ctx) override {
        fprintf(stderr, "Visit Describe Table.\n");
        
        databaseManager->listTableInfo(ctx->Identifier()->getText());
        return visitChildren(ctx);
    }

    std::any visitInsert_into_table(SQLParser::Insert_into_tableContext *ctx) override {
        fprintf(stderr, "Visit Insert Into Table.\n");

        DBInsert* dbInsert = new DBInsert;
        std::vector<std::vector<void*>> valueLists = std::any_cast<std::vector<std::vector<void*>>>(ctx->value_lists()->accept(this));
        dbInsert->valueLists = valueLists;

        auto lists = ctx->value_lists()->value_list(); // a vector
        for(int i = 0; i < lists.size(); i++) {
            // for each value list
            std::vector<DB_LIST_TYPE> listType;
            auto values = lists[i]->value();
            for(int j = 0; j < values.size(); j++) {
                if(values[j]->Integer() != nullptr) {
                    listType.push_back(DB_LIST_INT);
                } else if(values[j]->String() != nullptr) {
                    listType.push_back(DB_LIST_CHAR);
                } else if(values[j]->Float() != nullptr) {
                    listType.push_back(DB_LIST_FLOAT);
                } else {
                    listType.push_back(DB_LIST_NULL);
                }
            }
            dbInsert->valueListsType.push_back(listType);
        }
        chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
        int ret = databaseManager->insertRecords(ctx->Identifier()->getText(), dbInsert);
        chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        if(ret != -1)
            printf("Insert %d rows in %f seconds\n", ret, time_span.count());

        // delete pointer with struct destructor
        delete dbInsert;
        return 0;
    }

    std::any visitDelete_from_table(SQLParser::Delete_from_tableContext *ctx) override {
        fprintf(stderr, "Visit Delete From Table.\n");

        DBDelete* dbDelete = new DBDelete;
        std::vector<DBExpression> expression;
        expression = std::any_cast<std::vector<DBExpression>>(ctx->where_and_clause()->accept(this));
        dbDelete->expression = expression;
        chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
        int ret = databaseManager->dropRecords(ctx->Identifier()->getText(), dbDelete);
        
        chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        if(ret != -1)
            printf("Delete %d rows in %f seconds\n", ret, time_span.count());
        // delete pointer in DBExpression using struct destructor
        delete dbDelete;
        return 0;
    }

    std::any visitUpdate_table(SQLParser::Update_tableContext *ctx) override {
        fprintf(stderr, "Visit Update Table.\n");

        DBUpdate* dbUpdate = new DBUpdate;
        std::vector<DBExpression> expItem = std::any_cast<std::vector<DBExpression>>(ctx->set_clause()->accept(this));
        std::vector<DBExpression> expressions = std::any_cast<std::vector<DBExpression>>(ctx->where_and_clause()->accept(this));
        dbUpdate->expItem = expItem;
        dbUpdate->expressions = expressions;
        chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
        int ret = databaseManager->updateRecords(ctx->Identifier()->getText(), dbUpdate);
        
        chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        if(ret != -1)
            printf("Update %d rows in %f seconds\n", ret, time_span.count());
        // delete pointer in DBExpression using struct destructor
        delete dbUpdate;
        return 0;
    }

    std::any visitSelect_table_(SQLParser::Select_table_Context *ctx) override {
        fprintf(stderr, "Visit Select Table_.\n");

        DBSelect* dbSelect;
        dbSelect = std::any_cast<DBSelect*>(ctx->select_table()->accept(this));
        chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
        int ret = databaseManager->selectRecords(dbSelect);
        chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        if(ret != -1)
            printf("Select %d rows in %f seconds\n", ret, time_span.count());
        // delete pointer in DBExpression with struct destructor
        delete dbSelect;
        return 0;
    }
    
    std::any visitSelect_table(SQLParser::Select_tableContext *ctx) override {
        fprintf(stderr, "Visit Select Table.\n");
        
        DBSelect* dbSelect = new DBSelect;

        std::vector<DBSelItem> selectItems;

        selectItems = std::any_cast<std::vector<DBSelItem>>(ctx->selectors()->accept(this));
        dbSelect->selectItems = selectItems;

        std::vector<std::string> selectTables;
        selectTables = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        dbSelect->selectTables = selectTables;

        if(ctx->where_and_clause() != nullptr) {
            std::vector<DBExpression> expressions;
            expressions = std::any_cast<std::vector<DBExpression>>(ctx->where_and_clause()->accept(this));
            dbSelect->expressions = expressions;
        }

        if(ctx->column() != nullptr) { // 'GROUP' 'BY' column
            dbSelect->groupByEn = true;
            DBExpItem* groupByCol;
            groupByCol = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
            dbSelect->groupByCol = *groupByCol;
            delete groupByCol;
        }

        size_t intSize = ctx->Integer().size();
        if(intSize == 1) {
            dbSelect->limitEn = true;
            dbSelect->limitNum = stoi(ctx->Integer(0)->getText());
        } else if(intSize == 2) {
            dbSelect->limitEn = true;
            dbSelect->limitNum = stoi(ctx->Integer(0)->getText());
            dbSelect->offsetEn = true;
            dbSelect->offsetNum = stoi(ctx->Integer(1)->getText());
        } else {
            // it will never use this branch in normal
        }
        return dbSelect;
    }

    std::any visitAlter_add_index(SQLParser::Alter_add_indexContext *ctx) override {
        fprintf(stderr, "Visit Add Index.\n");
        
        std::string tableName = ctx->Identifier()->getText();
        std::vector<std::string> colName = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        for(int i = 0; i < colName.size(); i++)
            databaseManager->createIndex(tableName, colName[i]);
        return 0;
    }

    std::any visitAlter_drop_index(SQLParser::Alter_drop_indexContext *ctx) override {
        fprintf(stderr, "Visit Drop Index.\n");
        
        std::string tableName = ctx->Identifier()->getText();
        std::vector<std::string> colName = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        for(int i = 0; i < colName.size(); i++)
            databaseManager->dropIndex(tableName, colName[i]);
        return 0;
    }

    std::any visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *ctx) override {
        fprintf(stderr, "Visit Drop Primary Key.\n");
        
        std::string tableName;
        tableName = ctx->Identifier(0)->getText();
        // size_t identSize = ctx->Identifier().size();
        // std::vector<std::string> colName = std::vector<std::string>();
        // if(identSize == 1) {
        //     tableName = ctx->Identifier(0)->getText();
        // } else if(identSize == 2) {
        //     tableName = ctx->Identifier(0)->getText();
        //     colName.push_back(ctx->Identifier(1)->getText());
        // } else {
        //     // error
        // }
        databaseManager->dropPrimaryKey(tableName);
        return 0;
    }

    std::any visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *ctx) override {
        fprintf(stderr, "Visit Drop Foreign Key.\n");
        
        std::string tableName;
        std::string fkName;

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            fkName = ctx->Identifier(1)->getText();
        } else {
            // it will never use this branch in normal
        }

        databaseManager->dropForeignKey(tableName, fkName);
        return 0;
    }

    std::any visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *ctx) override {
        fprintf(stderr, "Visit Add Primary Key.\n");
        
        std::string tableName;
        std::string pkName;

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            pkName = ctx->Identifier(1)->getText();
        } else {
            // it will never use this branch in normal
        }
        std::vector<std::string> colName;
        colName = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        databaseManager->createPrimaryKey(tableName, colName, colName.size());
        return 0;
    }

    std::any visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *ctx) override {
        fprintf(stderr, "Visit Add Foreign Key.\n");
        
        std::string tableName = "";
        std::string fkName = "";
        std::vector<std::string> colNames = std::vector<std::string>();
        std::string refTableName = "";
        std::vector<std::string> refColNames = std::vector<std::string>();

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            refTableName = ctx->Identifier(2)->getText();
        } else if(identSize == 3){
            tableName = ctx->Identifier(0)->getText();
            fkName = ctx->Identifier(1)->getText();
            refTableName = ctx->Identifier(2)->getText();
        } else {
            // it will never use this branch in normal
        }
        colNames = std::any_cast<std::vector<std::string>>(ctx->identifiers(0)->accept(this));
        refColNames = std::any_cast<std::vector<std::string>>(ctx->identifiers(1)->accept(this));

        // if(colNames.size() != 1 || refColNames.size() != 1) {
        //     printf("report error when add foreign key because it is not 1 on 1\n");
        //     exit(0);
        // }

        databaseManager->createForeignKey(tableName, fkName, colNames[0], refTableName, refColNames[0]);
        return 0;
    }

    std::any visitAlter_table_add_unique(SQLParser::Alter_table_add_uniqueContext *ctx) override {
        fprintf(stderr, "Visit Add Unique.\n");
        
        std::string tableName;
        std::vector<std::string> colNames;
        tableName = ctx->Identifier()->getText();
        colNames = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        
        databaseManager->createUniqueKey(tableName, colNames, colNames.size());
        return 0;
    }

    // for drop unique::
    std::any visitAlter_table_drop_unique(SQLParser::Alter_table_drop_uniqueContext *ctx) override {
        fprintf(stderr, "Visit Drop Unique.\n");
        std::string tableName;
        std::vector<std::string> colNames;
        tableName = ctx->Identifier()->getText();
        colNames = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
    
        databaseManager->dropUniqueKey(tableName, colNames, colNames.size());
        return 0;
    }
   

    std::any visitField_list(SQLParser::Field_listContext *ctx) override {
        fprintf(stderr, "Visit Field List.\n");

        std::vector<FieldItem> fieldList;
        FieldItem item;
        for(int i = 0; i < ctx->field().size(); i++) {
            item = std::any_cast<FieldItem>(ctx->field(i)->accept(this));
            fieldList.push_back(item);
        }
        return fieldList;
    }

    std::any visitNormal_field(SQLParser::Normal_fieldContext *ctx) override {
        fprintf(stderr, "Visit Normal Field.\n");
        FieldItem item;
        item.isNormalField = true;
        item.fieldName = ctx->Identifier()->getText();
        item.type = std::any_cast<Type>(ctx->type_()->accept(this));
        if(ctx->Null() != nullptr) {
            item.isNotNull = true;
        }
        if(ctx->value() != nullptr) {
            item.hasDefault = true;
            if(ctx->value()->Integer() != nullptr) {
                int* pInt = std::any_cast<int*>(ctx->value()->accept(this));
                item.dValueInt = *pInt;
            } else if(ctx->value()->String() != nullptr) {
                char* pChar = std::any_cast<char*>(ctx->value()->accept(this));
                item.dValueString = std::string(pChar);
            } else if(ctx->value()->Float() != nullptr) {
                float* pFloat = std::any_cast<float*>(ctx->value()->accept(this));
                item.dValueFloat = *pFloat;
            }
        }
        return item;
    }

    std::any visitPrimary_key_field(SQLParser::Primary_key_fieldContext *ctx) override {
        fprintf(stderr, "Visit Primary Key Field.\n");

        FieldItem item;
        item.isPkField = true;
        item.colNames = std::any_cast<std::vector<std::string>>(ctx->identifiers()->accept(this));
        return item;
    }

    std::any visitForeign_key_field(SQLParser::Foreign_key_fieldContext *ctx) override {
        fprintf(stderr, "Visit Foreign Key Field.\n");

        FieldItem item;
        item.isFkField = true;

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            item.refTableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            item.fkName = ctx->Identifier(0)->getText();
            item.refTableName = ctx->Identifier(1)->getText();
        } else {
            // it will never use this branch in normal
        }

        std::vector<std::string> colNames;
        std::vector<std::string> refColNames;
        colNames = std::any_cast<std::vector<std::string>>(ctx->identifiers(0)->accept(this));
        refColNames = std::any_cast<std::vector<std::string>>(ctx->identifiers(1)->accept(this));
        if(colNames.size() != 1 || refColNames.size() != 1) {
            printf("report error when create foreign key field because it is not 1 on 1\n");
            exit(0);
        }
        item.colName = colNames[0];
        item.refColName = refColNames[0];
        return item;
    }

    std::any visitType_(SQLParser::Type_Context *ctx) override {
        // fprintf(stderr, "Visit Type.\n");

        Type type;
        if(ctx->getStart()->getText() == "INT") {
            type.typeName = COL_INT;
            type.len = 4;
        } else if(ctx->getStart()->getText() == "VARCHAR") {
            type.typeName = COL_VARCHAR;
            type.len = stoi(ctx->Integer()->getText());
        } else if(ctx->getStart()->getText() == "FLOAT") {
            type.typeName = COL_FLOAT;
            type.len = 4;
        } else {
            // it will never use this branch in normal
        }
        return type;
    }

    std::any visitValue_lists(SQLParser::Value_listsContext *ctx) override {
        fprintf(stderr, "Visit Value Lists.\n");

        std::vector<std::vector<void*>> valueLists;
        auto lists = ctx->value_list();
        for(int i = 0; i < lists.size(); i++) {
            valueLists.push_back(std::any_cast<std::vector<void*>>(lists[i]->accept(this)));
        }   
        return valueLists;
    }

    std::any visitValue_list(SQLParser::Value_listContext *ctx) override {
        // fprintf(stderr, "Visit Value List.\n");
        std::vector<void*> list;
        auto values = ctx->value();
        for(int i = 0; i < values.size(); i++) {
            if(values[i]->Integer() != nullptr) {
                int* pInt = std::any_cast<int*>(values[i]->accept(this));
                list.push_back((void*)pInt);
            } else if(values[i]->String() != nullptr) {
                char* pString = std::any_cast<char*>(values[i]->accept(this));
                list.push_back((void*)pString);
            } else if(values[i]->Float() != nullptr) {
                float* pFloat = std::any_cast<float*>(values[i]->accept(this));
                list.push_back((void*)pFloat);
            } else if(values[i]->Null() != nullptr){
                list.push_back(nullptr);
            } else {
                // it will never use this branch in normal
            }
        }
        return list;
    }

    std::any visitValue(SQLParser::ValueContext *ctx) override {
        // fprintf(stderr, "Visit Value.\n");

        // NOTICE: create pointer here, need to be deleted after using
        if(ctx->Integer() != nullptr) {
            int* pInt = new int(stoi(ctx->Integer()->getText()));
            return pInt;
        }
        else if(ctx->String() != nullptr) {
            std::string subString = ctx->String()->getText();
            size_t startPos = subString.find("'");
            size_t endPos   = subString.rfind("'");
            subString = subString.substr(startPos+1, endPos-startPos-1);
            char* pString = new char[subString.length()+1];
            strcpy(pString, subString.c_str());
            return pString;
        }
        else if(ctx->Float() != nullptr) {
            float* pFloat = new float(stof(ctx->Float()->getText()));
            return pFloat;
        }
        else if(ctx->Null() != nullptr)
            return nullptr;
        else
            // it will never use this branch in normal
        return 0;
    }
    
    std::any visitWhere_and_clause(SQLParser::Where_and_clauseContext *ctx) override {
        fprintf(stderr, "Visit Where And Clause.\n");

#ifndef NO_OPTIM
        optimizeWhereClause(ctx, databaseManager);
#endif
        std::vector<DBExpression> expressions;

        auto clauses = ctx->where_clause();
        for(int i = 0; i < clauses.size(); i++) {
            DBExpression expr;
            expr = std::any_cast<DBExpression>(clauses[i]->accept(this));
            expressions.push_back(expr);
        }
        return expressions;
    }

    std::any visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *ctx) override {
        fprintf(stderr, "Visit WOE.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;

        DB_EXP_OP_TYPE op = std::any_cast<DB_EXP_OP_TYPE>(ctx->operator_()->accept(this));
        expr.op = op;

        if(ctx->expression()->value() != nullptr) {
            if(ctx->expression()->value()->Integer() != nullptr) {
                int* itemInt = std::any_cast<int*>(ctx->expression()->accept(this));
                expr.rVal = itemInt;
                expr.rType = DB_INT;
            } else if(ctx->expression()->value()->String() != nullptr) {
                char* itemString = std::any_cast<char*>(ctx->expression()->accept(this));
                expr.rVal = itemString;
                expr.rType = DB_CHAR;
            } else if(ctx->expression()->value()->Float() != nullptr) {
                float* itemFloat = std::any_cast<float*>(ctx->expression()->accept(this));
                expr.rVal = itemFloat;
                expr.rType = DB_FLOAT;
            } else if(ctx->expression()->value()->Null() != nullptr){
                expr.rVal = expr.lVal;
                expr.rType = DB_NULL;
            } else {
                // it will never use this branch in normal
            }
        } else if(ctx->expression()->column() != nullptr) {
            DBExpItem* pItem = std::any_cast<DBExpItem*>(ctx->expression()->accept(this));
            expr.rVal = pItem;
            expr.rType = DB_ITEM;
        }
        
        return expr;
    }

    std::any visitWhere_operator_select(SQLParser::Where_operator_selectContext *ctx) override {
        fprintf(stderr, "Visit WOS.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;
        
        DB_EXP_OP_TYPE op = std::any_cast<DB_EXP_OP_TYPE>(ctx->operator_()->accept(this));
        expr.op = op;

        DBSelect* dbselect;
        dbselect = std::any_cast<DBSelect*>(ctx->select_table()->accept(this));
        expr.rVal = dbselect;
        expr.rType = DB_NST;

        return expr;
    }

    std::any visitWhere_null(SQLParser::Where_nullContext *ctx) override {
        fprintf(stderr, "Visit WN.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;

        expr.op = IS_TYPE;
        std::string nullInst = ctx->getText();
        if(nullInst.rfind("NOT") != std::string::npos) {
            expr.op = ISN_TYPE;
        }

        expr.rVal = nullptr;
        expr.rType = DB_NULL;
        return expr;
    }

    std::any visitWhere_in_list(SQLParser::Where_in_listContext *ctx) override {
        fprintf(stderr, "Visit WIL.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;
        expr.op = IN_TYPE;
        
        std::vector<void*> valueList;
        std::vector<DB_LIST_TYPE> valueListType;

        valueList = std::any_cast<std::vector<void*>>(ctx->value_list()->accept(this));
        std::vector<void*>* pList = new std::vector<void*>(valueList);

        auto values = ctx->value_list()->value();
        for(int i = 0; i < ctx->value_list()->value().size(); i++) {
            if(values[i]->Integer() != nullptr) {
                valueListType.push_back(DB_LIST_INT);
            } else if(values[i]->String() != nullptr) {
                valueListType.push_back(DB_LIST_CHAR);
            } else if(values[i]->Float() != nullptr) {
                valueListType.push_back(DB_LIST_FLOAT);
            } else if(values[i]->Null() != nullptr){
                valueListType.push_back(DB_LIST_NULL);
            } else {
                // it will never use this branch in normal
            }
        }
        expr.rVal = pList;
        expr.rType = DB_LIST;
        expr.valueListType = valueListType;
        return expr;
    }

    std::any visitWhere_in_select(SQLParser::Where_in_selectContext *ctx) override {
        fprintf(stderr, "Visit WIS.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;
        expr.op = IN_TYPE;

        DBSelect* dbselect;
        dbselect = std::any_cast<DBSelect*>(ctx->select_table()->accept(this));
        expr.rVal = dbselect;
        expr.rType = DB_NST;

        return expr;
    }

    std::any visitWhere_like_string(SQLParser::Where_like_stringContext *ctx) override {
        fprintf(stderr, "Visit WLS.\n");

        DBExpression expr;
        DBExpItem* item1 = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
        expr.lVal = item1;
        expr.lType = DB_ITEM;
        expr.op = LIKE_TYPE;

        // std::string* item2 = new std::string(ctx->String()->getText());
        std::string subString = ctx->String()->getText();
        size_t startPos = subString.find("'");
        size_t endPos   = subString.rfind("'");
        subString = subString.substr(startPos+1, endPos-startPos-1);
        char* pchar = new char[subString.length()];
        strcpy(pchar, subString.c_str());
        expr.rVal = pchar;
        expr.rType = DB_CHAR;
        return expr;
    }

    std::any visitColumn(SQLParser::ColumnContext *ctx) override {
        // fprintf(stderr, "Visit Column.\n");

        DBExpItem* pItem;
        if(ctx->Identifier().size() == 1) {
            pItem = new DBExpItem("", ctx->Identifier(0)->getText());
            return pItem;
        }
        else if(ctx->Identifier().size() == 2) {
            pItem = new DBExpItem(ctx->Identifier(0)->getText(), ctx->Identifier(1)->getText());
            return pItem;
        }
        else    // error
            return nullptr;
        
    }

    std::any visitExpression(SQLParser::ExpressionContext *ctx) override {
        // fprintf(stderr, "Visit Expression.\n");

        if(ctx->value() != nullptr) {
            return ctx->value()->accept(this);
        } else if(ctx->column() != nullptr) {
            return ctx->column()->accept(this);
        } else {
            return nullptr;
            // it will never use this branch in normal
        }
    }

    std::any visitSet_clause(SQLParser::Set_clauseContext *ctx) override {
        fprintf(stderr, "Visit Set Clause.\n");

        std::vector<DBExpression> expItem;
        DBExpression expr;
        auto idents = ctx->Identifier();
        auto values = ctx->value();
        for(int i = 0; i < idents.size(); i++) {
            DBExpItem *item = new DBExpItem;
            item->expCol = idents[i]->getText();
            expr.lVal = item;
            expr.lType = DB_ITEM;
            expr.op = EQU_TYPE;

            int* intValue;
            char* stringValue;
            float* floatValue;
            if(values[i]->Integer() != nullptr) {
                intValue = std::any_cast<int*>(values[i]->accept(this));
                // printf("%d\n", *intValue);
                expr.rVal = intValue;
                expr.rType = DB_INT;
            } else if(values[i]->String() != nullptr) {
                stringValue = std::any_cast<char*>(values[i]->accept(this));
                expr.rVal = stringValue;
                expr.rType = DB_CHAR;
            } else if(values[i]->Float() != nullptr) {
                floatValue = std::any_cast<float*>(values[i]->accept(this));
                expr.rVal = floatValue;
                expr.rType = DB_FLOAT;
            } else if(values[i]->Null() != nullptr) {
                expr.rVal = nullptr;
                expr.rType = DB_NULL;
            } else {

            }
            expItem.push_back(expr);
        }
        return expItem;
    }

    std::any visitSelectors(SQLParser::SelectorsContext *ctx) override {
        fprintf(stderr, "Visit Selectors.\n");

        std::vector<DBSelItem> selectItems;
        DBSelItem item;
        auto selectors = ctx->selector();
        if(selectors.size() == 0) {
            item.star = true;
            item.selectType = ORD_TYPE;
            selectItems.push_back(item);
        }
        for(int i = 0; i < selectors.size(); i++) {
            item = std::any_cast<DBSelItem>(selectors[i]->accept(this));
            selectItems.push_back(item);
        }
        return selectItems;
    }

    std::any visitSelector(SQLParser::SelectorContext *ctx) override {
        // fprintf(stderr, "Visit Selector.\n");

        DBSelItem selItem;
        selItem.star = false;
         if(ctx->aggregator() != nullptr) {
            DBExpItem* pItem = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
            selItem.item = *pItem;
            selItem.selectType = std::any_cast<DB_SELECT_TYPE>(ctx->aggregator()->accept(this));
            delete pItem;
        } else if(ctx->column() != nullptr) {
            DBExpItem* pItem = std::any_cast<DBExpItem*>(ctx->column()->accept(this));
            selItem.item =  *pItem;
            selItem.selectType = ORD_TYPE;
            delete pItem;
        } else if(ctx->Count() != nullptr) { // no star condition in aggregator 
            selItem.star = true;
            selItem.selectType = COUNT_TYPE;
        } else {
            // it will never use this branch in normal
        }
        return selItem;
    }

    std::any visitIdentifiers(SQLParser::IdentifiersContext *ctx) override {
        fprintf(stderr, "Visit Identifiers.\n");

        std::vector<std::string> selectTables;
        auto idents = ctx->Identifier();
        for(int i = 0; i < ctx->Identifier().size(); i++)
            selectTables.push_back(idents[i]->getText());
        return selectTables;
    }

    std::any visitOperator_(SQLParser::Operator_Context *ctx) override {
        // fprintf(stderr, "Visit Operator.\n");
        bool flag = ctx->children.size() == 1;
        if(ctx->EqualOrAssign() != nullptr)
            return EQU_TYPE;
        else if(ctx->Less() != nullptr)
            return flag ? LT_TYPE : GT_TYPE;
        else if(ctx->LessEqual() != nullptr)
            return flag ? LTE_TYPE : GTE_TYPE;
        else if(ctx->Greater() != nullptr)
            return flag ? GT_TYPE : LT_TYPE;
        else if(ctx->GreaterEqual() != nullptr)
            return flag ? GTE_TYPE: LTE_TYPE;
        else if(ctx->NotEqual() != nullptr)
            return NEQ_TYPE;
        else
            return 0;// it will never use this branch in normal
    }

    std::any visitAggregator(SQLParser::AggregatorContext *ctx) override {
        // fprintf(stderr, "Visit Aggregator.\n");

        DB_SELECT_TYPE type;
        if(ctx->Count() != nullptr) {
            type = COUNT_TYPE;
        } else if(ctx->Average() != nullptr) {
            type = AVERAGE_TYPE;
        } else if(ctx->Max() != nullptr) {
            type = MAX_TYPE;
        } else if(ctx->Min() != nullptr) {
            type = MIN_TYPE;
        } else if(ctx->Sum() != nullptr) {
            type = SUM_TYPE;
        } else {
            // it will never use this branch in normal
        }
        return type;
    }

};

#endif