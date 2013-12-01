#include <stdio.h>
#include <stdlib.h>
struct x{
char *s;
void * a;
int size;
void *next;
}*head;

int main()
{
  head = (struct x *)calloc(1,sizeof(struct x));
  head->size = 2;
  return 0;
}
