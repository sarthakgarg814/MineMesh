#include "dedup.h"
#include "app_config.h"
#include <string.h>
static char ids[APP_DEDUP_HISTORY][80]; static unsigned next;
typedef struct { size_t len; char data[APP_JSON_MAX + 1]; } retry_item_t;
static retry_item_t retry_items[APP_RETRY_QUEUE_LENGTH]; static unsigned retry_head; static unsigned retry_tail; static unsigned retry_count;
bool dedup_seen_or_add(const char *id){if(!id||!id[0])return true;for(unsigned i=0;i<APP_DEDUP_HISTORY;i++)if(ids[i][0]&&strcmp(ids[i],id)==0)return true;strlcpy(ids[next],id,sizeof(ids[next]));next=(next+1)%APP_DEDUP_HISTORY;return false;}
bool retry_enqueue(const char *data,size_t len){if(!data||len==0||len>APP_JSON_MAX||retry_count>=APP_RETRY_QUEUE_LENGTH)return false;retry_item_t *item=&retry_items[retry_tail];memcpy(item->data,data,len);item->data[len]='\0';item->len=len;retry_tail=(retry_tail+1)%APP_RETRY_QUEUE_LENGTH;retry_count++;return true;}
bool retry_dequeue(char *data,size_t *len,size_t capacity){if(!data||!len||retry_count==0||capacity<retry_items[retry_head].len+1)return false;retry_item_t *item=&retry_items[retry_head];memcpy(data,item->data,item->len+1);*len=item->len;retry_head=(retry_head+1)%APP_RETRY_QUEUE_LENGTH;retry_count--;return true;}
