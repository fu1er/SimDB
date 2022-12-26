#ifndef __MYSQLVisitor_H__
#define __MYSQLVisitor_H__

#include "antlr4/SQLBaseVisitor.h"
#include "SQLOptimizer.h"
#include <cstdio>


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
        fieldList = std::any_cast<std::vector<FieldItem>>(visitField_list(ctx->field_list()));
        // int colNum = ctx->field_list()->field().size();
        // char (*colName)[COL_MAX_NAME_LEN] = new char[colNum][COL_MAX_NAME_LEN];
        // TB_COL_TYPE* colType = new TB_COL_TYPE[colNum];
        // int *colLen = new int[colNum];

        // for(int i = 0; i < colNum; i++) {
        //     auto field = ctx->field_list()->field(i);
        //     if(field->start->getType() == 64) {
        //         // normal field
        //         auto normalField = dynamic_cast<SQLParser::Normal_fieldContext*>(field);
        //         strcpy(colName[i], normalField->getStart()->getText().c_str());
        //         if(normalField->type_()->start->getText() == "INT") {
        //             colType[i] = COL_INT;
        //             colLen[i] = 4;
        //         } else if(normalField->type_()->getStart()->getText() == "VARCHAR") {
        //             colType[i] = COL_VARCHAR;
        //             colLen[i] = stoi(normalField->type_()->Integer()->getText());
        //         } else if(normalField->type_()->getStart()->getText() == "FLOAT") {
        //             colType[i] = COL_FLOAT;
        //             colLen[i] = 4;
        //         } else {
        //             // TODO error
        //         }
        //     }
        //     // TODO PRIMARY KEY & FOREIGN KEY
        // }
        // databaseManager->createTable(tableName, colName, colType, colLen, colNum);
        // delete [] colName;
        // delete [] colType;
        // delete [] colLen;
        // return visitChildren(ctx);
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
        std::vector<std::vector<void*>> valueLists = std::any_cast<std::vector<std::vector<void*>>>(visitValue_lists(ctx->value_lists()));
        dbInsert->valueLists = valueLists;

        auto value_lists = ctx->value_lists()->value_list(); // a vector
        for(int i = 0; i < value_lists.size(); i++) {
            // for each value list
            std::vector<DB_LIST_TYPE> listType;
            auto values = value_lists[i]->value();
            for(int j = 0; j < values.size(); j++) {
                // for each value
                switch (values[j]->getStart()->getType())
                {
                case SQLParser::Integer: {
                    // printf("type is %ld\n", values[j]->getStart()->getType());
                    listType.push_back(DB_LIST_INT);
                    break;
                }

                case SQLParser::String: {
                    // printf("type is %ld\n", values[j]->getStart()->getType());
                    listType.push_back(DB_LIST_CHAR);
                    break;
                }
                case SQLParser::Float: {
                    // printf("type is %ld\n", values[j]->getStart()->getType());
                    listType.push_back(DB_LIST_FLOAT);
                    break;
                }
                default:
                    listType.push_back(DB_LIST_NULL);
                    break;
                }
            }
            dbInsert->valueListsType.push_back(listType);
            
        }
        printf("here\n");
        // TODO segmetation fault here
        databaseManager->insertRecords(ctx->Identifier()->getText(), dbInsert);
        printf("before return\n");
        return 0;
    }

    std::any visitDelete_from_table(SQLParser::Delete_from_tableContext *ctx) override {
        fprintf(stderr, "Visit Delete From Table.\n");
        DBDelete* dbDelete = new DBDelete;
        // maybe optimize before parsing
        std::vector<DBExpression> expression;
        expression = std::any_cast<std::vector<DBExpression>>(visitWhere_and_clause(ctx->where_and_clause()));
        dbDelete->expression = expression;
        return databaseManager->dropRecords(ctx->Identifier()->getText(), dbDelete);
    }

    std::any visitUpdate_table(SQLParser::Update_tableContext *ctx) override {
        fprintf(stderr, "Visit Update Table.\n");
        DBUpdate* dbUpdate = new DBUpdate;
        std::vector<DBExpression> expItem = std::any_cast<std::vector<DBExpression>>(visitSet_clause(ctx->set_clause()));
        std::vector<DBExpression> expressions = std::any_cast<std::vector<DBExpression>>(visitWhere_and_clause(ctx->where_and_clause()));
        dbUpdate->expItem = expItem;
        dbUpdate->expressions = expressions;
        return databaseManager->updateRecords(ctx->Identifier()->getText(), dbUpdate);
    }

    std::any visitSelect_table_(SQLParser::Select_table_Context *ctx) override {
        fprintf(stderr, "Visit Select Table_.\n");
        return visitSelect_table(ctx->select_table());
    }
    
    std::any visitSelect_table(SQLParser::Select_tableContext *ctx) override {
        fprintf(stderr, "Visit Select Table.\n");
        DBSelect* dbSelect = new DBSelect;

        std::vector<DBSelItem> selectItems;
        selectItems = std::any_cast<std::vector<DBSelItem>>(visitSelectors(ctx->selectors()));
        dbSelect->selectItems = selectItems;

        std::vector<std::string> selectTables;
        selectTables = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers()));
        dbSelect->selectTables = selectTables;

        if(ctx->where_and_clause() != nullptr) {
            std::vector<DBExpression> expressions;
            expressions = std::any_cast<std::vector<DBExpression>>(visitWhere_and_clause(ctx->where_and_clause()));
            dbSelect->expressions = expressions;
        }

        if(ctx->column() != nullptr) { // 'GROUP' 'BY' column
            dbSelect->groupByEn = true;
            DBExpItem groupByCol;
            groupByCol = std::any_cast<DBExpItem>(visitColumn(ctx->column()));
            dbSelect->groupByCol = groupByCol;
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
            // TODO error
        }
        
        return databaseManager->selectRecords(dbSelect);
    }

    std::any visitAlter_add_index(SQLParser::Alter_add_indexContext *ctx) override {
        std::string tableName = ctx->Identifier()->getText();
        std::vector<std::string> colName = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers()));
        for(int i = 0; i < colName.size(); i++)
            databaseManager->createIndex(tableName, colName[i]);
        return 0;
    }

    std::any visitAlter_drop_index(SQLParser::Alter_drop_indexContext *ctx) override {
        std::string tableName = ctx->Identifier()->getText();
        std::vector<std::string> colName = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers()));
        for(int i = 0; i < colName.size(); i++)
            databaseManager->dropIndex(tableName, colName[i]);
        return 0;
    }

    std::any visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *ctx) override {
        std::string tableName;
        size_t identSize = ctx->Identifier().size();
        std::vector<std::string> colName = std::vector<std::string>();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            colName.push_back(ctx->Identifier(1)->getText());
        } else {
            // TODO error
        }
        // TODO drop pk with column name
        return databaseManager->dropPrimaryKey(tableName, colName, colName.size());
    }

    std::any visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *ctx) override {
        std::string tableName;
        std::string fkName;

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            fkName = ctx->Identifier(1)->getText();
        } else {
            // TODO error
        }
        return databaseManager->dropForeignKey(tableName, fkName);
    }

    std::any visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *ctx) override {
        std::string tableName;
        std::string pkName;

        size_t identSize = ctx->Identifier().size();
        if(identSize == 1) {
            tableName = ctx->Identifier(0)->getText();
        } else if(identSize == 2) {
            tableName = ctx->Identifier(0)->getText();
            pkName = ctx->Identifier(1)->getText();
        } else {
            // TODO error
        }
        std::vector<std::string> colName;
        colName = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers()));
        return databaseManager->createPrimaryKey(tableName, colName, colName.size());
    }

    std::any visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *ctx) override {
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
            // TODO error
        }
        colNames = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers(0)));
        refColNames = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers(1)));

        if(colNames.size() != 1 || refColNames.size() != 1) {
            printf("report error when add foreign key because it is not 1 on 1\n");
            exit(0);
        }
        return databaseManager->createForeignKey(tableName, fkName, colNames[0], refTableName, refColNames[0]);
    }

    std::any visitAlter_table_add_unique(SQLParser::Alter_table_add_uniqueContext *ctx) override {
        std::string tableName;
        std::vector<std::string> colNames;
        tableName = ctx->Identifier()->getText();
        colNames = std::any_cast<std::vector<std::string>>(visitIdentifiers(ctx->identifiers()));
        return databaseManager->createUniqueKey(tableName, colNames, colNames.size());
    }

    std::any visitField_list(SQLParser::Field_listContext *ctx) override {
        fprintf(stderr, "Visit Field List.\n");
        std::vector<FieldItem> fieldList;
        FieldItem item;
        for(int i = 0; i < ctx->field().size(); i++) {
            // ctx->field(i)->accept(this);
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
                item.dValueInt = std::any_cast<int>(ctx->value()->accept(this));
            } else if(ctx->value()->String() != nullptr) {
                item.dValueString = std::any_cast<string>(ctx->value()->accept(this));
            } else if(ctx->value()->Float() != nullptr) {
                item.dValueFloat = std::any_cast<float>(ctx->value()->accept(this));
            }
        }
        return item;
        // return visitChildren(ctx);
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
            // TODO error
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
        fprintf(stderr, "Visit Type.\n");
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
            // TODO error
        }
        return type;
    }

    std::any visitValue_lists(SQLParser::Value_listsContext *ctx) override {
        fprintf(stderr, "Visit Value Lists.\n");
        std::vector<std::vector<void*>> valueLists;
        for(int i = 0; i < ctx->value_list().size(); i++) {
            valueLists.push_back(std::any_cast<std::vector<void*>>( visitValue_list(ctx->value_list(i))));
        }
        return valueLists;
    }

    std::any visitValue_list(SQLParser::Value_listContext *ctx) override {
        fprintf(stderr, "Visit Value List.\n");
        std::vector<void*> values;
        for(int i = 0; i < ctx->value().size(); i++) {
            if(ctx->value(i)->Integer() != nullptr) {
                // printf("is int\n");
                int intValue = std::any_cast<int>(visitValue(ctx->value(i)));
                values.push_back((void*)&intValue);
            } else if(ctx->value(i)->String() != nullptr) {
                // printf("is string\n");
                std::string stringValue = std::any_cast<std::string>(visitValue(ctx->value(i)));
                values.push_back((void*)stringValue.c_str());
            } else if(ctx->value(i)->Float() != nullptr) {
                // printf("is float\n");
                float floatValue = std::any_cast<float>(visitValue(ctx->value(i)));
                values.push_back((void*)&floatValue);
            } else if(ctx->value(i)->Null() != nullptr){
                // printf("is null\n");
                values.push_back(nullptr);
            } else {
                // TODO error
            }
        }
        return values;
    }

    std::any visitValue(SQLParser::ValueContext *ctx) override {
        fprintf(stderr, "Visit Value.\n");
        if(ctx->Integer() != nullptr)
            return stoi(ctx->Integer()->getText());
        else if(ctx->String() != nullptr)
            return ctx->String()->getText();
        else if(ctx->Float() != nullptr)
            return stof(ctx->Float()->getText());
        else if(ctx->Null() != nullptr)
            return 0;
        else
            // TODO error
        return 0;
    }
    
    std::any visitWhere_and_clause(SQLParser::Where_and_clauseContext *ctx) override {
        // optimizeWhereClause(ctx, databaseManager);
        fprintf(stderr, "Visit Where And Clause.\n");
        std::vector<DBExpression> expressions;
        for(int i = 0; i < ctx->where_clause().size(); i++) {
            DBExpression expr;
            SQLParser::Where_in_listContext* wil = dynamic_cast<SQLParser::Where_in_listContext*>(ctx->where_clause(i));
            if(wil != NULL) {
                expr = std::any_cast<DBExpression>(visitWhere_in_list(wil));
                expressions.push_back(expr);
                continue;
            }
            SQLParser::Where_operator_selectContext* wos = dynamic_cast<SQLParser::Where_operator_selectContext*>(ctx->where_clause(i));
            if(wos != NULL) {
                // TODO
                visitWhere_operator_select(wos);
                continue;
            }
            SQLParser::Where_nullContext* wn = dynamic_cast<SQLParser::Where_nullContext*>(ctx->where_clause(i));
            if(wn != NULL) {
                expr = std::any_cast<DBExpression>(visitWhere_null(wn));
                expressions.push_back(expr);
                continue;
            }
            SQLParser::Where_operator_expressionContext* woe = dynamic_cast<SQLParser::Where_operator_expressionContext*>(ctx->where_clause(i));
            if(woe != NULL) {
                expr = std::any_cast<DBExpression>(visitWhere_operator_expression(woe));
                expressions.push_back(expr);
                continue;
            }
            SQLParser::Where_in_selectContext* wis = dynamic_cast<SQLParser::Where_in_selectContext*>(ctx->where_clause(i));
            if(wis != NULL) {
                // TODO
                visitWhere_in_select(wis);
                continue;
            }
            SQLParser::Where_like_stringContext* wls = dynamic_cast<SQLParser::Where_like_stringContext*>(ctx->where_clause(i));
            if(wls != NULL) {
                expr = std::any_cast<DBExpression>(visitWhere_like_string(wls));
                expressions.push_back(expr);
                continue;
            }
        }
        return expressions;
    }

    std::any visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *ctx) override {
        fprintf(stderr, "Visit WOE.\n");
        DBExpression expr;
        DBExpItem item1 = std::any_cast<DBExpItem>(visitColumn(ctx->column()));
        DB_EXP_OP_TYPE op = std::any_cast<DB_EXP_OP_TYPE>(visitOperator_(ctx->operator_()));
        expr.lVal = &item1;
        expr.lType = DB_ITEM;
        expr.op = op;

        if(ctx->expression()->value() != nullptr) {
            if(ctx->expression()->value()->Integer() != nullptr) {
                int itemInt = std::any_cast<int>(visitExpression(ctx->expression()));
                expr.rVal = &itemInt;
                expr.rType = DB_INT;
            } else if(ctx->expression()->value()->String() != nullptr) {
                std::string itemString = std::any_cast<std::string>(visitExpression(ctx->expression()));
                expr.rVal = &itemString;
                expr.rType = DB_CHAR;
            } else if(ctx->expression()->value()->Float() != nullptr) {
                float itemFloat = std::any_cast<float>(visitExpression(ctx->expression()));
                expr.rVal = &itemFloat;
                expr.rType = DB_FLOAT;
            } else if(ctx->expression()->value()->Null() != nullptr){
                expr.rVal = expr.lVal;
                expr.rType = DB_NULL;
            } else {
                // TODO error
            }
        }
        
        return expr;
    }

    std::any visitWhere_operator_select(SQLParser::Where_operator_selectContext *ctx) override {
        // TODO
        fprintf(stderr, "Visit WOS.\n");
        return visitChildren(ctx);
    }

    std::any visitWhere_null(SQLParser::Where_nullContext *ctx) override {
        fprintf(stderr, "Visit WN.\n");
        DBExpression expr;
        DBExpItem item1 = std::any_cast<DBExpItem>(visitColumn(ctx->column()));
        expr.lVal = &item1;
        expr.lType = DB_ITEM;
        expr.op = IS_TYPE;
        std::string nullInst = ctx->getText();
        if(nullInst.rfind("NOT") != std::string::npos) {
            expr.op = ISN_TYPE;
        }
        return expr;
    }

    std::any visitWhere_in_list(SQLParser::Where_in_listContext *ctx) override {
        fprintf(stderr, "Visit WIL.\n");
        DBExpression expr;
        DBExpItem item1 = std::any_cast<DBExpItem>(visitColumn(ctx->column()));
        expr.lVal = &item1;
        expr.lType = DB_ITEM;
        expr.op = IN_TYPE;
        
        std::vector<void*> valueList;
        std::vector<DB_LIST_TYPE> valueListType;
        valueList = std::any_cast<std::vector<void*>>(visitValue_list(ctx->value_list()));
        for(int i = 0; i < ctx->value_list()->value().size(); i++) {
            if(ctx->value_list()->value(i)->Integer() != nullptr) {
                valueListType.push_back(DB_LIST_INT);
            } else if(ctx->value_list()->value(i)->String() != nullptr) {
                valueListType.push_back(DB_LIST_CHAR);
            } else if(ctx->value_list()->value(i)->Float() != nullptr) {
                valueListType.push_back(DB_LIST_FLOAT);
            } else if(ctx->value_list()->value(i)->Null() != nullptr){
                valueListType.push_back(DB_LIST_NULL);
            } else {
                // TODO error
            }
        }
        expr.rVal = &valueList;
        expr.rType = DB_LIST;
        expr.valueListType = valueListType;
        return expr;
    }

    std::any visitWhere_in_select(SQLParser::Where_in_selectContext *ctx) override {
        // TODO
        fprintf(stderr, "Visit WIS.\n");
        return visitChildren(ctx);
    }

    std::any visitWhere_like_string(SQLParser::Where_like_stringContext *ctx) override {
        fprintf(stderr, "Visit WLS.\n");
        DBExpression expr;
        DBExpItem item1 = std::any_cast<DBExpItem>(visitColumn(ctx->column()));
        expr.lVal = &item1;
        expr.lType = DB_ITEM;
        expr.op = LIKE_TYPE;

        std::string item2 = ctx->String()->getText();
        expr.rVal = &item2;
        expr.rType = DB_CHAR;
        return expr;
    }

    std::any visitColumn(SQLParser::ColumnContext *ctx) override {
        fprintf(stderr, "Visit Column.\n");
        if(ctx->Identifier().size() == 1)
            return DBExpItem("", ctx->Identifier(0)->getText());
        else
            return DBExpItem(ctx->Identifier(0)->getText(), ctx->Identifier(1)->getText());
        
    }

    std::any visitExpression(SQLParser::ExpressionContext *ctx) override {
        fprintf(stderr, "Visit Expression.\n");
        if(ctx->value() != nullptr) {
            return visitValue(ctx->value());
        } else if(ctx->column() != nullptr) {
            return visitColumn(ctx->column());
        } else {
            return 0;
            // TODO error
        }
    }

    std::any visitSet_clause(SQLParser::Set_clauseContext *ctx) override {
        fprintf(stderr, "Visit Set Clause.\n");
        std::vector<DBExpression> expItem;
        DBExpItem item;
        DBExpression expr;
        for(int i = 0; i < ctx->Identifier().size(); i++) {
            item.expCol = ctx->Identifier(i)->getText();
            expr.lVal = &item;
            expr.lType = DB_ITEM;
            expr.op = EQU_TYPE;

            int intValue;
            std::string stringValue;
            float floatValue;
            if(ctx->value(i)->Integer() != nullptr) {
                intValue = std::any_cast<int>(visitValue(ctx->value(i)));
                expr.rVal = &intValue;
                expr.rType = DB_INT;
            } else if(ctx->value(i)->String() != nullptr) {
                stringValue = std::any_cast<std::string>(visitValue(ctx->value(i)));
                expr.rVal = &stringValue;
                expr.rType = DB_CHAR;
            } else if(ctx->value(i)->Float() != nullptr) {
                floatValue = std::any_cast<float>(visitValue(ctx->value(i)));
                expr.rVal = &floatValue;
                expr.rType = DB_FLOAT;
            } else if(ctx->value(i)->Null() != nullptr) {
                expr.rVal = nullptr;
                expr.rType = DB_NULL;
            } else {

            }
            expItem.push_back(expr);
        }
        return expItem;
    }

    std::any visitSelectors(SQLParser::SelectorsContext *ctx) override {
        std::vector<DBSelItem> selectItems;
        DBSelItem item;
        if(ctx->selector().size() == 0) {
            item.star = true;
            item.selectType = ORD_TYPE;
            selectItems.push_back(item);
        }
        for(int i = 0; i < ctx->selector().size(); i++) {
            item = std::any_cast<DBSelItem>(visitSelector(ctx->selector(i)));
            selectItems.push_back(item);
        }
        return selectItems;
    }

    std::any visitSelector(SQLParser::SelectorContext *ctx) override {
        DBSelItem selItem;
        selItem.star = false;
        if(ctx->column() != nullptr) {
            selItem.item =  std::any_cast<DBExpItem>(visitColumn(ctx->column()));
            selItem.selectType = ORD_TYPE;
        } else if(ctx->aggregator() != nullptr) {
            selItem.item =  std::any_cast<DBExpItem>(visitColumn(ctx->column()));
            selItem.selectType = std::any_cast<DB_SELECT_TYPE>(visitAggregator(ctx->aggregator()));
        } else if(ctx->Count() != nullptr) { // no star condition in aggregator 
            selItem.star = true;
            selItem.selectType = COUNT_TYPE;
        } else {
            // TODO error
        }
        return selItem;
    }

    std::any visitIdentifiers(SQLParser::IdentifiersContext *ctx) override {
        std::vector<std::string> selectTables;
        for(int i = 0; i < ctx->Identifier().size(); i++)
            selectTables.push_back(ctx->Identifier(i)->getText());
        return selectTables;
    }

    std::any visitOperator_(SQLParser::Operator_Context *ctx) override {
        fprintf(stderr, "Visit Operator.\n");
        if(ctx->EqualOrAssign() != nullptr)
            return EQU_TYPE;
        else if(ctx->Less() != nullptr)
            return LT_TYPE;
        else if(ctx->LessEqual() != nullptr)
            return LTE_TYPE;
        else if(ctx->Greater() != nullptr)
            return GT_TYPE;
        else if(ctx->GreaterEqual() != nullptr)
            return GTE_TYPE;
        else if(ctx->NotEqual() != nullptr)
            return NEQ_TYPE;
        else
            return 0;// TODO error
    }

    std::any visitAggregator(SQLParser::AggregatorContext *ctx) override {
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
            // TODO error
        }
        return type;
    }

};

#endif