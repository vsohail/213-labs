#include <stdio.h>
#include "csapp.h"
#include <time.h>
struct cache {
  int size;
  char url[MAXLINE];
  void *data;
  long freshness;
  struct cache *next;
};

void cache_init(int max_cache_size,int max_obj_size);
void insert(char *url,void *data,int size);
void *retreive(char *url,int *size);


