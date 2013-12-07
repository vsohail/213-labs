#include "cache.h"

int cur_size;
int max_size;
int max_obj;
static struct cache *init_block;
static struct cache *head;
static sem_t mutex;
static sem_t w;
static int readcount;
void cache_init(int max_cache_size,int max_obj_size)
{
  max_size=max_cache_size;
  max_obj=max_obj_size;
  init_block = Malloc(sizeof(struct cache));
  head=init_block;
  init_block->next=NULL;
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);
  readcount=0;
}
void insert(char *url,void *data,int size)
{
  if(size>max_size)
    return;
  P(&w);
  struct cache *new_entry = Malloc(sizeof(struct cache));
  new_entry->size = size;
  cur_size += size;
  new_entry->data = data;
  strncpy(new_entry->url,url,strlen(url));
  new_entry->next = NULL;
  new_entry->freshness = (long)time(0);
  head->next = new_entry;
  head = new_entry;
  struct cache *move = init_block->next;
  long tmp = move->freshness;
  struct cache *lru=move,*prev=init_block;
  while(cur_size>max_size) {
    move=init_block->next;
    tmp=move->freshness;
    prev=init_block;
    lru=move;
    while(move->next != head) {
      if(tmp > move->next->freshness)
      {
        tmp=move->next->freshness;
        lru=move->next;
        prev=move;
      }
      move=move->next;
    }
    prev->next=lru->next;
    cur_size-=lru->size;
    Free(lru->data);
    Free(lru);
    lru = NULL;
  }
  V(&w);
}
void *retreive(char *url,int *size)
{
  P(&mutex);
  readcount++;
  if(readcount==1)
    P(&w);
  V(&mutex);
  struct cache *cur;
  cur=init_block->next;
  void *data = NULL;
  while(cur!=NULL) {
    if(!strcmp(cur->url,url)) {
      cur->freshness = (long)time(0);
      *size = cur->size;
      data = cur->data;
      break;
    }
    cur=cur->next;
  }
  P(&mutex);
  readcount--;
  if(readcount==0)
    V(&w);
  V(&mutex);
  return data;
}
