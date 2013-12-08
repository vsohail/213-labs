/*
 * Name: Sohil Habib
 * Andrew Id:snhabib
 *
 * LRU cache
 * The cache follows the readers-writers sollution favouring
 * multiple readers
 * The cache structhas
 * ->the size,
 * ->url for matching as a hit,
 * ->the pointer to the data
 * ->and the freshness value obtained using the C time function
 * Each block points to the next block
 *
 * The cache inserts an element with a full write lock
 * and evicts least fresh elements
 *
 * The retreive function does not read a write lock
 * as it performs an atomic cache freshness update
 */
#include "cache.h"

int cur_size;
int max_size;
int max_obj;
static struct cache *init_block;
static struct cache *head;
static sem_t mutex;
static sem_t w;
static int readcount;
/* Initialize the cache */
void cache_init(int max_cache_size,int max_obj_size)
{
  max_size=max_cache_size;
  max_obj=max_obj_size;
  init_block = Malloc(sizeof(struct cache));
  head=init_block;
  init_block->next=NULL;
  /* Initialize the mutexes */
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);
  readcount=0;
}
/*
 * Insert a block into the cache
 */
void insert(char *url,void *data,int size)
{
  /* if cache insertion exceeds MAX_CACHE_SIZE, return */
  if(size>max_size)
    return;
  P(&w); /* Obtain write lock */
  struct cache *new_entry = Malloc(sizeof(struct cache));
  /* Initialize the new block */
  new_entry->size = size;
  cur_size += size;
  new_entry->data = data;
  strncpy(new_entry->url,url,strlen(url));
  new_entry->next = NULL;
  new_entry->freshness = (long)time(0);
  head->next = new_entry;
  head = new_entry;
  /* Initialize variables needed for traversal */
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
    /* Evict least fresh(LRU) element from cache */
    prev->next=lru->next;
    cur_size-=lru->size;
    Free(lru->data);
    Free(lru);
    lru = NULL;
  }
  V(&w); /* Release write lock */
}
/*
 * Reads the cache to find a hit
 * Updates freshness if accessed
 */
void *retreive(char *url,int *size)
{
  /* Update readcount and prevent write */
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
      cur->freshness = (long)time(0); /* update freshness */
      *size = cur->size;
      data = cur->data;
      break;
    }
    cur=cur->next;
  }
  /* decrement readcount and release writelock if no more readers */
  P(&mutex);
  readcount--;
  if(readcount==0)
    V(&w);
  V(&mutex);
  return data;
}
