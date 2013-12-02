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
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);
  readcount=0;
  printf("Cache INIT\n");
}
void insert(char *url,void *data,int size)
{
  if(size>max_size)
    return;
  struct cache *new_entry = Malloc(sizeof(struct cache));
  P(&w);
  new_entry->size = size;
  cur_size += size;
  new_entry->data = data;
  strncpy(new_entry->url,url,strlen(url));
  new_entry->next = NULL;
  head->next = new_entry;
  head = new_entry;
  V(&w);
  struct cache *move = init_block->next;
  P(&mutex);
  readcount++;
  if(readcount==1)
    P(&w);
  V(&mutex);
  while(cur_size>max_size) {
    if(move!=head)
    {
      P(&mutex);
      readcount--;
      if(readcount==0)
        V(&w);
      V(&mutex);
      P(&w);
      init_block->next=init_block->next->next;
      cur_size-=move->size;
      Free(move->data);
      Free(move);
      move=init_block->next;
      V(&w);
      P(&mutex);
      readcount++;
      if(readcount==1)
        P(&w);
      V(&mutex);
    }
  }
  P(&mutex);
  readcount--;
  if(readcount==0)
    V(&w);
  V(&mutex);
}
void *retreive(char *url,int *size)
{
  struct cache *cur,*prev;
  cur=init_block->next;
  prev=init_block;
  void *data = NULL;
  P(&mutex);
  readcount++;
  if(readcount==1)
    P(&w);
  V(&mutex);
  while(cur!=NULL) {
    if(!strcmp(cur->url,url)) {
      if(cur!=head) {
        P(&mutex);
        readcount--;
        if(readcount==0)
          V(&w);
        V(&mutex);
        P(&w);
        prev->next=cur->next;
        head->next=cur;
        head=cur;
        cur->next=NULL;
      }
      *size = cur->size;
      data = cur->data;
      V(&w);
      P(&mutex);
      readcount++;
      if(readcount==1)
        P(&w);
      V(&mutex);
      break;
    }
    prev=cur;
    cur=cur->next;
  }
  P(&mutex);
  readcount--;
  if(readcount==0)
    V(&w);
  V(&mutex);
  return data;
}
