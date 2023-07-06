#include "goodmalloc.h"
#include <iostream>
#include <map>
using namespace std;

int *memory_;
Data *data_;
map<char *, int> mp;

struct Memory 
{
  int *start; // start of memory
  int *end; // end of memory
  size_t size; // size of memory in words
  size_t totalFree; // total free memory in words (including the header)
  u_int numFreeBlocks; // number of free blocks
  size_t currMaxFree; // current maximum free block size in words

  int init(size_t bytes)
  {
    bytes = ((bytes + 3) >> 2) << 2;
    start = (int *)malloc(bytes);
    if (start == NULL)
    {
      return -1;
    }
    end = start + (bytes >> 2);
    size = bytes >> 2; // in words
    *start = (bytes >> 2) << 1;
    *(start + (bytes >> 2) - 1) = (bytes >> 2) << 1;

    totalFree = bytes >> 2;
    numFreeBlocks = 1;
    currMaxFree = bytes >> 2;
    return 0;
  }

  // Absolute address to offset
  int getOffset(int *p)
  {
    return (int)(p - start);
  }

  // Offset to absolute address
  int *getAddr(int offset)
  {
    return (start + offset);
  }
};

Variable *CreateVariable(char *name, int type, int localAddr, int arrLen)
{
  if (!(type == LL_INT && strlen(name) < VAR_NAME_SIZE))
  {
    fprintf(stderr, "Invalid Variable\n");
    exit(1);
  }
  Variable *a = (Variable *)malloc(sizeof(Variable));
  a->name = name;
  a->type = type;
  a->isTobeCleaned = 0;
  a->arrLen = arrLen;
  a->localAddress = localAddr;
  if (type = LL_INT)
    a->size = arrLen;
  else
    a->size = (4 * arrLen + 3) / 4;
  return a;
}

Stack *createStack()
{
  Stack *s = (Stack *)malloc(sizeof(Stack));
  s->topIndex = -1;
  return s;
}

void push(Stack *s, Variable *x)
{
  if (s->topIndex == STACK_SIZE - 1)
  {
    fprintf(stderr, "Global Stack Size Limit Reached. Exitting.\n");
    exit(1);
  }
  s->stck[++s->topIndex] = x;
}

void pop(Stack *s)
{
  if (s->topIndex == -1)
  {
    fprintf(stderr, "Error, stack is Empty!\n");
    exit(1);
  }
  s->topIndex--;
}

Variable *top(Stack *s)
{
  return s->stck[s->topIndex];
}
bool isEmpty(Stack *s)
{
  if (s->topIndex == -1)
    return 1;
  return 0;
}

void createMem()
{
  printf("createMem was called\n");
  memory_ = (int *)malloc(MEM_SIZE + sizeof(Data));
  data_ = (Data *)(memory_ + MEM_SIZE / 4);
  for (int i = 0; i < MEM_SIZE / 4; i++)
    data_->actualAddressToLocalAdress[i] = -1;
  data_->localAddress = 0;
  data_->maxMemIndex = -1;
  printf("Memory segment created\n");
}

int max(int a, int b)
{
  return a > b ? a : b;
}

int assignVal(char *name, int offset, int num, int arr[])
{
  BLOCK *block = ((BLOCK *)data_->pageTable[mp[name] / 4]);
  for (int i = 0; i < num; i++)
  {
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = arr[i];
    temp->next = temp->prev = NULL;
    block->curr_sz++;
    if (!(block->list))
      (block->list) = temp;
    else
    {
      temp->next = block->list;
      (block->list)->prev = temp;
      (block->list) = temp;
    }
  }
  return mp[name];
}

// Utility function to swap two integers
void swap(int *A, int *B)
{
  int temp = *A;
  *A = *B;
  *B = temp;
}

// Split a doubly linked list (BLOCK) into 2 DLLs of half sizes
Node *split(Node *head)
{
  Node *fast = head, *slow = head;
  while (fast->next && fast->next->next)
  {
    fast = fast->next->next;
    slow = slow->next;
  }
  Node *temp = slow->next;
  slow->next = NULL;
  return temp;
}

int createList(char *name, int type, int sz)
{
  printf("createList function was called.\nCreating list with name: %s and type: %s and length: %d\n", name, "LINKED LIST", sz);
  BLOCK *block = (BLOCK *)malloc(sizeof(BLOCK));
  block->name = name;
  block->sz = sz;
  block->curr_sz = 0;
  block->list = (Node *)malloc(sz * sizeof(Node));
  for (int i = 0; i < sz; i++)
  {
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = rand() % LIMIT + 1;
    temp->next = temp->prev = NULL;
    block->curr_sz++;
    if (!(block->list))
      (block->list) = temp;
    else
    {
      temp->next = block->list;
      (block->list)->prev = temp;
      (block->list) = temp;
    }
  }

  // Add the newly created BLOCK to the Data variable
  Variable *var = CreateVariable(name, type, data_->localAddress, sz);
  data_->variableList[data_->localAddress / 4] = *var;
  int len = var->size;
  for (int i = 0; i < MEM_SIZE / 4; i++)
  {
    int flag = 1;
    for (int j = 0; j < len; j++)
    {
      if (j + i < MEM_SIZE / 4)
      {
        if (!(data_->actualAddressToLocalAdress[i + j] == -1))
        {
          flag = 0;
          break;
        }
      }
      else
        break;
    }
    if (flag == 1)
    {
      for (int j = 0; j < len; j++)
      {
        if (j + i < MEM_SIZE / 4)
        {
          data_->maxMemIndex = max(data_->maxMemIndex, i + j);
          data_->actualAddressToLocalAdress[j + i] = data_->localAddress;
        }
      }
      data_->pageTable[data_->localAddress / 4] = memory_ + i;
      break;
    }
  }

  int temp = data_->localAddress;
  data_->localAddress += 4;
  push(&data_->variableStack, &data_->variableList[temp / 4]);

  mp[name] = temp;
  return temp;
}

// Get word location for arrays using index in the array
int idxToWord(Data type, int idx)
{
  int cnt;
  if (idx)
  {
    cnt = (1 << 5) / sizeof(type);
  }
  else
  {
    cnt = (1 << 2) / sizeof(type);
  }
  int word = idx / cnt;
  return word;
}

// Get offset in a word for arrays using index in the array
int idxToOffset(Data type, int idx)
{
  int cnt;
  if (idx)
  {
    cnt = (1 << 5) / sizeof(type);
  }
  else
  {
    cnt = (1 << 2) / sizeof(type);
  }
  int word = idx / cnt;
  return (idx - word * cnt) * sizeof(type);
}

void freeElem()
{
  Variable *curr = top(&data_->variableStack); // Get the top of the stack
  while (curr != NULL && !isEmpty(&data_->variableStack)) // Pop all the variables that are to be cleaned
  {
    curr->isTobeCleaned = 1; // Set the isTobeCleaned to 1
    curr->type = -1; // Set the type to -1
    pop(&data_->variableStack); // Pop the last element
    curr = top(&data_->variableStack); // Get the top of the stack
  }
  if (curr == NULL) // If stack is empty, return
    pop(&data_->variableStack); // Pop the last element

  if (isEmpty(&data_->variableStack)) // If stack is empty, return
  {
    for (int i = 0; i < data_->localAddress; i += 4) // Iterate over the variable list
    {
      if (data_->variableList[i / 4].isTobeCleaned) // If the variable is to be cleaned
      {
        int locAddr = i; // Get the local address
        int len = data_->variableList[locAddr / 4].size; // Get the length of the variable
        int index = data_->pageTable[locAddr / 4] - memory_; // Get the index of the variable
        printf("freeElem function was called. Freeing %s\n", data_->variableList[locAddr / 4].name); // Print the name of the variable
        for (int i = 0; i < len; i++) // Iterate over the length of the variable
        {
          if (index + i < MEM_SIZE / 4) // If the index is valid
            data_->actualAddressToLocalAdress[index + i] = -1; // Set the actual address to local address mapping to -1
          else
          {
            fprintf(stderr, "Freeing invalid index.\n");
            exit(1);
          }
        }
        data_->variableList[i / 4].isTobeCleaned = 0; // Set the isTobeCleaned to 0
      }
    }
  }
}
