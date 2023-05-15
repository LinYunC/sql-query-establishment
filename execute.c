/*execute.c*/
//
// Project: Execution of queries for SimpleSQL
//
// YOUR NAME
// Northwestern University
// CS 211, Winter 2023
//

#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h>
//
// #include any other system <.h> files?
#include <assert.h>  // assert
#include <ctype.h>   // isdigit
#include <string.h> // strcpy, strcat


// #include any other of our ".h" files?
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
// implementation of function(s), both private and public
//
const char* printColType(int i){
  if (i == 1) {
      return "int";
    }
  else if (i== 2) {
      return "real";
    }
  else if (i == 3) {
      return "string";
    }
  return "";
}

const char* printindexType(int i){
  if ( i == COL_NON_INDEXED) {
      return "non-indexed";
    }
  else if (i == COL_INDEXED) {
      return "indexed";
    }
  else if (i == COL_UNIQUE_INDEXED) {
      return "unique indexed";  
    }

  return "";
}

const char* printOperator(int i){
  if (i == 0){
    return "<";
  }
  else if (i == 1){
    return "<=";
  }
  else if (i == 2){
    return ">";
  }
  else if (i == 3){
    return ">=";
  }
  else if (i == 4){
    return "=";
  }
  else if (i == 5){
    return "<>";
  }
  else if (i == 6) {
    return "like";
  }
}

const char* printColumn(struct COLUMN* column){

  int size = DATABASE_MAX_ID_LENGTH*2+10;
  char* colName = (char*) malloc(sizeof(char) * size);
  strcpy(colName, column->table);
  strcat(colName, ".");
  strcat(colName, column->name);

  char* res = (char*) malloc(sizeof(char) * size);

  switch(column->function) {
    case -1 : 
      return colName;
    case 0: 
      strcpy(res, "MIN(");
      strcat(res, colName);
      strcat(res, ")");
      break;
    case 1: 
      strcpy(res, "MAX(");
      strcat(res, colName);
      strcat(res, ")");
      break;
    case 2:
      strcpy(res, "SUM(");
      strcat(res, colName);
      strcat(res, ")");
      break;
    case 3:
      strcpy(res, "AVG(");
      strcat(res, colName);
      strcat(res, ")");
      break;
    case 4: 
      strcpy(res, "COUNT(");
      strcat(res, colName);
      strcat(res, ")");
      break;
  }
  return res;
}

// your functions
void print_schema(struct TableMeta* tables, int N) {
  for (int i = 0; i < N; i++) {
    printf("Table: %s\n Record size: %i\n", tables[i].name, tables[i].recordSize);
    int K=tables[i].numColumns;

    for (int j = 0; j < K; j++){
      printf("  Column: ");
    // char colType[10];
    // printColType(tables[i].columns[j].colType, colType);
    // char indexType[20];
    //   printindexType(tables[i].columns[j].indexType, indexType);
      printf("%s, %s, %s \n", tables[i].columns[j].name, printColType(tables[i].columns[j].colType), printindexType(tables[i].columns[j].indexType));
    }
  }
}

 

void print_ast(struct QUERY* query){
  printf("**QUERY AST**\n");
  struct SELECT* select = query->q.select;
  printf("Table: %s\n", select->table);

  //columns (linked list)
  struct COLUMN* cur = select->columns;
  while (cur != NULL) {


      // printf("Select column: %s.%s\n", cur->table,cur->name);
      // char* column = printColumn(cur);
      printf("Select column: %s\n", printColumn(cur));
      cur = cur->next;
    }
  //others (shallow tree with pointer)
  struct JOIN* join = select->join;
  struct WHERE* where = select->where;
  struct ORDERBY* orderby = select->orderby;
  struct LIMIT* limit = select->limit;
  struct INTO* into = select->into;
  if (join == NULL){
    printf("Join (NULL)\n");
  }
  else{
    struct COLUMN* left_cur = join->left;
    struct COLUMN* right_cur = join->right;
    printf("Join %s on %s.%s = %s.%s \n", join->table, left_cur->table, left_cur->name, right_cur->table, right_cur->name);
  }
  
  if (where == NULL){
    printf("Where (NULL)\n");
  }
  else {
    struct COLUMN* cur = where->expr->column;

    char* value = where->expr->value;
    const char ch = '\'';

    if (where->expr->litType == STRING_LITERAL) {
      if (strchr(value, ch) != NULL) {
        printf("Where %s.%s %s \"%s\"\n", cur->table, cur->name, printOperator(where->expr->operator), value);
      }
      else {
        printf("Where %s.%s %s \'%s\'\n", cur->table, cur->name, printOperator(where->expr->operator), value);
      }
    } else {
      printf("Where %s.%s %s %s \n", cur->table, cur->name,     printOperator(where->expr->operator), value);  
    }
    
  }
  
  if (orderby == NULL){
    printf("Order By (NULL)\n");
  }
  else {
  //bool   ascending;  // true => ascending, false => descending
    
    struct COLUMN* cur = orderby->column;
    char* asc;
    if (orderby->ascending == true){
      asc = "ASC";
    }
    else{
      asc = "DESC";
    }
    printf("Order By %s %s\n", printColumn(cur), asc);
  }
  
  if (limit == NULL){
    printf("Limit (NULL)\n");
  }
  else {
    printf("Limit %i\n", limit->N);
  }
  
  if (into == NULL){
    printf("Into (NULL)\n");
  }
  else {
    printf("Into %s\n", into->table);
  }
}

void execute_query(struct Database* db, struct QUERY* query) {
  
  FILE *fp;
  // char filename[DATABASE_MAX_ID_LENGTH];
  struct ResultSet* rs = resultset_create();
  // char* tableName = "Movie";
  int pathSize = DATABASE_MAX_ID_LENGTH*2 + 8;
  char path[DATABASE_MAX_ID_LENGTH*2 + 8] = "./";
    
  /* opening file for reading */
  // printf("**END OF QUERY AST**\n");

  int bufferSize = 0;
  char* tableName = (char*) malloc(sizeof(char) * DATABASE_MAX_ID_LENGTH);
  struct TableMeta table;
  for (int i = 0; i < db->numTables; i++) {
    if (strcasecmp(db->tables[i].name, query->q.select->table)==0) {
      
      bufferSize = (db->tables[i].recordSize)+3;
      tableName = db->tables[i].name;
      table = db->tables[i];
      break;
    }
  }
  
  strcat(path, db->name);
  strcat(path, "/");
  strcat(path, tableName);
  strcat(path, ".data");
  // printf("path: %s \n", path);
  fp = fopen(path , "r");
  if(fp == NULL) {
        perror("Error opening file");
        //0125change return(-1);
        exit(-1);
  }

  char* buffer = (char*) malloc(sizeof(char) * bufferSize);
  // char buffer[bufferSize];
  if (buffer == NULL) {
    panic("out of memory");
  }

  // for (int i = 0; i < db->numTables; i++){
  // int K = db->tables[i].numColumns;
  int K = table.numColumns;
  for (int j = 0; j < K; j++) {
    resultset_insertColumn(rs, j+1, 
      table.name, 
      table.columns[j].name,
      NO_FUNCTION,
      table.columns[j].colType);

      // db->tables[i].name, 
      // db->tables[i].columns[j].name,
      // NO_FUNCTION,
      // db->tables[i].columns[j].colType);
    
    // printf("%s, %s, %d \n", 
    //   db->tables[i].name, 
    //   db->tables[i].columns[j].name, 
    //   db->tables[i].columns[j].colType);
// }
  // rs[i].columns[j].tableName = tables[i].name;
  // rs[i].columns[j].colName = tables[i].columns[j].name;
  // rs[i].columns[j].function = NO_FUNCTION;
  // rs[i].columns[j].coltype = tables[i].columns[j].colType;

}
  
  //0125change
  // for (int i = 0; i < 5; i++){   
  //    if(fgets(buffer, bufferSize, fp)!=NULL) {
  //       /* writing content to stdout */
  //       printf("%s \n", buffer);

      //   printf("%i\n", strlen(buffer));
      // for (int j = 0; j < strlen(buffer); j++){
      //   if (buffer[j] == "/'"){
      //     printf("|");
     //    }
     //  }
     // }


  int row = 0;
  while (!feof(fp)){
    if(fgets(buffer, bufferSize, fp)!=NULL){
      // printf("%s \n", buffer);
      char* cp = buffer;
      
      while (*cp != '\0'){
        if (*cp == '\''){
          *cp = '\0';
          cp++;
          while (*cp != '\''){
            cp++;
          } 
          *cp = '\0';
        }
        if (*cp == '\"'){
          *cp = '\0';
          cp++;
          while (*cp != '\"'){
            cp++;
          } 
          *cp = '\0';
        }
        if (isspace(*cp)){
            *cp = '\0';
        }
        cp++;
      }

      resultset_addRow(rs);
      row++;
      char* start = (char*) malloc(sizeof(char) * bufferSize);
      start = buffer;
      for (int i = 0; i < table.numColumns; i++) {
        if (table.columns[i].colType == COL_TYPE_INT) {
          int n = atoi(start);
          resultset_putInt(rs, row, i+1, n);
        } else if (table.columns[i].colType == COL_TYPE_REAL) {
          resultset_putReal(rs, row, i+1, atof(start));
        } else if (table.columns[i].colType == COL_TYPE_STRING) {
          
          resultset_putString(rs, row, i+1, start);
        } 
        
        char* end = start;
        while(*end != '\0') {
          end++;
        }
        start = end;
        while(*start == '\0') {
          start++;
        }
      }
    } 
  }
  free(buffer);

  //query->select->where->expr
  //void resultset_deleteRow(struct ResultSet* rs, int rowNum /*1..N*/);

  //step 6
  //select * from Movies2 where ID = 912;
  struct RSColumn* cur_col = rs->columns;
  int numRow = rs->numRows;
  if (query->q.select->where != NULL){
    while (cur_col != NULL) {
      if (strcasecmp(query->q.select->where->expr->column->name, cur_col->colName)==0){
        for (int i = numRow; i > 0; i--){
          struct RSValue* data = cur_col->data;
          
          if (query->q.select->where->expr->operator == EXPR_LT){
            if (cur_col->coltype == COL_TYPE_INT){
              if (data[i-1].value.i >= atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_REAL){
                if (data[i-1].value.r >= atof(query->q.select->where->expr->value)){
                  resultset_deleteRow(rs, i);
                }
              }
            if (cur_col->coltype == COL_TYPE_STRING){
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) >= 0){
                resultset_deleteRow(rs, i);
              }
            }
          }  

          if (query->q.select->where->expr->operator == EXPR_LTE){
            if (cur_col->coltype == COL_TYPE_INT){
              if (data[i-1].value.i > atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_REAL){
              if (data[i-1].value.r > atof(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_STRING){
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) > 0){
                resultset_deleteRow(rs, i);
              }
            }
          }

          if (query->q.select->where->expr->operator == EXPR_GT){
            if (cur_col->coltype == COL_TYPE_INT){
              if (data[i-1].value.i <= atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            
            if (cur_col->coltype == COL_TYPE_REAL){
              if (data[i-1].value.r <= atof(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_STRING){
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) <= 0){
                resultset_deleteRow(rs, i);
              }
            }
          }
          if (query->q.select->where->expr->operator == EXPR_GTE){
            if (cur_col->coltype == COL_TYPE_INT){
              if (data[i-1].value.i < atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_REAL){
              if (data[i-1].value.r < atof(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_STRING){
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) < 0){
                resultset_deleteRow(rs, i);
              }
            }
          }

        
          if (query->q.select->where->expr->operator == EXPR_EQUAL){
            if (cur_col->coltype == COL_TYPE_INT){  
              if (data[i-1].value.i != atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
              if (cur_col->coltype == COL_TYPE_REAL){
                if (data[i-1].value.r != atof(query->q.select->where->expr->value)){
                  resultset_deleteRow(rs, i);
                }
              }
        
            
            if (cur_col->coltype == COL_TYPE_STRING){
              // printf("data.value: %s, expr.value: %s, res: %d \n", data[i-1].value.s, query->q.select->where->expr->value, strcasecmp(data[i-1].value.s, query->q.select->where->expr->value));
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) != 0){
                resultset_deleteRow(rs, i);
              }
            }
          }
          if (query->q.select->where->expr->operator == EXPR_NOT_EQUAL){
            if (cur_col->coltype == COL_TYPE_INT ){
              if (data[i-1].value.i == atoi(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            if (cur_col->coltype == COL_TYPE_REAL){
              if (data[i-1].value.r == atof(query->q.select->where->expr->value)){
                resultset_deleteRow(rs, i);
              }
            }
            
            if (cur_col->coltype == COL_TYPE_STRING){
              if (strcasecmp(data[i-1].value.s, query->q.select->where->expr->value) == 0){
                resultset_deleteRow(rs, i);
              }
            }
          } 
        }
        cur_col = cur_col->next;
      } else {
        cur_col = cur_col->next;
      }
    }
  }
    
  //step 7
  for (int i = rs->numCols; i > 0; i--){
    bool isColSelected = false;
    struct COLUMN* cur = query->q.select->columns;
    while (cur != NULL){
      if (strcasecmp(table.columns[i-1].name, cur->name) == 0){
        isColSelected = true;
        break;
      }
      cur = cur->next;
    }
    if (isColSelected == false) {
      resultset_deleteColumn(rs, i);
    }
  }
    
  //step 8
  struct COLUMN* cur = query->q.select->columns;
  int toPos = 1;
  while (cur != NULL){
    int fromPos = resultset_findColumn(rs, 1, rs->columns->tableName, cur->name);
    if (fromPos != -1){
      resultset_moveColumn(rs, fromPos, toPos);
    }
    cur = cur->next;
    toPos++;
  }
  
  //step 9
  cur = query->q.select->columns;
  int colPos = 1;
  while (cur != NULL){
    if (cur->function != NO_FUNCTION){
      resultset_applyFunction(rs, cur->function, colPos);
      rs->numRows = 1;
    }
    cur = cur->next;
    colPos++;
  }

  // step 10
  if (query->q.select->limit != NULL) {
    cur_col = rs->columns;
    while (cur_col != NULL) {
      for (int i = rs->numRows; i > query->q.select->limit->N; i--){
        resultset_deleteRow(rs, i);
      }
      cur_col = cur_col->next;
    }
  }

  resultset_print(rs);
  fclose(fp);
      
}

