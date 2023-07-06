#include "goodmalloc.h"
#include <iostream>
using namespace std;

Node *split(Node *head);

// Function to merge two linked lists
Node *merge(Node *first, Node *second)
{
  // If first linked list is empty
  if (!first)
    return second;

  // If second linked list is empty
  if (!second)
    return first;

  // Pick the smaller value
  if (first->data < second->data)
  {
    first->next = merge(first->next, second);
    first->next->prev = first;
    first->prev = NULL;
    return first;
  }
  else
  {
    second->next = merge(first, second->next);
    second->next->prev = second;
    second->prev = NULL;
    return second;
  }
}

// Function to do merge sort
Node *mergeSort(Node *head)
{
  if (!head || !head->next)
    return head;
  Node *second = split(head);

  // Recur for left and right halves
  head = mergeSort(head);
  second = mergeSort(second);

  // Merge the two sorted halves
  return merge(head, second);
}

int main()
{
  srand(time(0));
  createMem();
  int localAddress = createList("My_List", LL_INT, 50000);
  int arr[50000];
  printf("Before sorting:\n");
  for (int i = 0; i < 50000; i++)
  {
    arr[i] = rand() % 100000 + 1;
    printf("%d ", arr[i]);
  }
  printf("\n");
  assignVal("My_List", 0, 50000, arr);
  BLOCK *head = (BLOCK *)data_->pageTable[localAddress / 4];


  head->list = mergeSort(head->list);
  printf("\nAfter sorting\n");

  while (head->list)
  {
    Node *temp = head->list;
    printf("%d ", head->list->data);
    temp = head->list;
    head->list = head->list->next;
  }
  printf("\n");
  freeElem();
  return 0;
}
