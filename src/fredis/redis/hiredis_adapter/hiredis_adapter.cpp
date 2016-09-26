#include "fredis/redis/hiredis_adapter/hiredis_adapter.h"
#include "fredis/redis/RedisClient.h"
#include <event.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

namespace fredis { namespace redis { namespace hiredis_adapter {

void fredisLibeventReadEvent(int fd, short event, void *arg) {
    ((void)fd); ((void)event);
    fredisLibeventEvents *e = (fredisLibeventEvents*)arg;
    redisAsyncHandleRead(e->context);
}

void fredisLibeventWriteEvent(int fd, short event, void *arg) {
    ((void)fd); ((void)event);
    fredisLibeventEvents *e = (fredisLibeventEvents*)arg;
    redisAsyncHandleWrite(e->context);
}

void fredisLibeventAddRead(void *privdata) {
    fredisLibeventEvents *e = (fredisLibeventEvents*)privdata;
    event_add(&e->rev,NULL);
}

void fredisLibeventDelRead(void *privdata) {
    fredisLibeventEvents *e = (fredisLibeventEvents*)privdata;
    event_del(&e->rev);
}

void fredisLibeventAddWrite(void *privdata) {
    fredisLibeventEvents *e = (fredisLibeventEvents*)privdata;
    event_add(&e->wev,NULL);
}

void fredisLibeventDelWrite(void *privdata) {
    fredisLibeventEvents *e = (fredisLibeventEvents*)privdata;
    event_del(&e->wev);
}

void fredisLibeventCleanup(void *privdata) {
    fredisLibeventEvents *e = (fredisLibeventEvents*)privdata;
    event_del(&e->rev);
    event_del(&e->wev);
    free(e);
}

int fredisLibeventAttach(RedisClient *client, redisAsyncContext *ac,
      struct event_base *base) {
    redisContext *c = &(ac->c);
    fredisLibeventEvents *e;

    /* Nothing should be attached when something is already attached */
    if (ac->ev.data != NULL) {
        return REDIS_ERR;
    }
    /* Create container for context and r/w events */
    e = (fredisLibeventEvents*)malloc(sizeof(*e));
    e->context = ac;
    e->client = client;

    /* Register functions to start/stop listening for events */
    ac->ev.addRead = fredisLibeventAddRead;
    ac->ev.delRead = fredisLibeventDelRead;
    ac->ev.addWrite = fredisLibeventAddWrite;
    ac->ev.delWrite = fredisLibeventDelWrite;
    ac->ev.cleanup = fredisLibeventCleanup;
    ac->ev.data = e;

    /* Initialize and install read/write events */
    event_set(&e->rev,c->fd,EV_READ, fredisLibeventReadEvent,e);
    event_set(&e->wev,c->fd,EV_WRITE, fredisLibeventWriteEvent,e);
    event_base_set(base,&e->rev);
    event_base_set(base,&e->wev);
    return REDIS_OK;
}

}}} // fredis::redis::hiredis_adapter

