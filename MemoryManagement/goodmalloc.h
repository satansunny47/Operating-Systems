#ifndef __GOOD_H__
#define __GOOD_H__

#include <string.h> 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define MEM_SIZE 250000000
#define VAR_NAME_SIZE 10
#define STACK_SIZE 100
#define NUM_VARIABLES 100
#define STR_SIZE 250
#define LIMIT 100000


#define INT 0
#define CHAR 1
#define LL_INT 2
#define BOOLEAN 3

int max(int a, int b);

typedef struct Node {
    int data;
    struct Node* next;
    struct Node* prev;
}Node;

typedef struct BLOCK {
  char* name;
  int sz;
  int curr_sz;
  Node* list;
}BLOCK;

typedef struct _Variable
{
  char *name;
  int type, size, localAddress, arrLen, isTobeCleaned;
} Variable;

// Stack's Implementation
typedef struct _Stack
{
  Variable *stck[STACK_SIZE];
  int topIndex;
} Stack;
//
typedef struct
{
  int *pageTable[NUM_VARIABLES];
  int localAddress, maxMemIndex;
  int actualAddressToLocalAdress[MEM_SIZE / 4];
  Variable variableList[NUM_VARIABLES];
  Stack variableStack;
} Data;


// Important Data Structures
extern int *memory_;
extern Data *data_;

/*
  Important Functions
*/
void createMem();
int createList(char *name, int type, int sz);
int assignVal(char* name, int offset, int num, int arr[]);
void freeElem();

// Utiliy Functions
Node *mergeSort(Node *head);

Variable *CreateVariable( char *name, int type, int localAddr, int arrLen);

// mediumInt CreateMediumInt(int val);
// int toInt(mediumInt *mi);

Stack *createStack();
void push(Stack *s, Variable *x);
void pop(Stack *s);
Variable *top(Stack *s);
bool isEmpty(Stack *s);
int idxToWord(Data type, int idx);
int idxToOffset(Data type, int idx);


#endif 
// __GOOD_H__