/*execute.h*/

//
// Project: Execution of queries for SimpleSQL
//
// Lin Yun Chang
// Northwestern University
// CS 211, Winter 2023
//

#pragma once

//
// #include header files needed to compile function declarations
//
#include "execute.h"
#include "analyzer.h"
#include "ast.h"
#include "database.h"
#include "execute.h"
#include "parser.h"
#include "resultset.h"
#include "scanner.h"
#include "tokenqueue.h"
#include "util.h"
//
// function declarations:
const char* printColType(int i);
const char* printindexType(int i);
const char* printOperator(int i);
const char* printColumn(struct COLUMN* column);
void print_schema(struct TableMeta* tables, int N);
void print_ast(struct QUERY* query);
void execute_query(struct Database* db, struct QUERY* query);
