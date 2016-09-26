#pragma once

// SI 9/25/2016
// adapted from hiredis/adapters/libevent.h
// adds an extra member to the connection context struct
// to hold a point to the relevant fredis::redis::RedisClient instance.
// original license is below:
  /*
   * Copyright (c) 2010-2011, Pieter Noordhuis <pcnoordhuis at gmail dot com>
   *
   * All rights reserved.
   *
   * Redistribution and use in source and binary forms, with or without
   * modification, are permitted provided that the following conditions are met:
   *
   *   * Redistributions of source code must retain the above copyright notice,
   *     this list of conditions and the following disclaimer.
   *   * Redistributions in binary form must reproduce the above copyright
   *     notice, this list of conditions and the following disclaimer in the
   *     documentation and/or other materials provided with the distribution.
   *   * Neither the name of Redis nor the names of its contributors may be used
   *     to endorse or promote products derived from this software without
   *     specific prior written permission.
   *
   * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   * POSSIBILITY OF SUCH DAMAGE.
   */

#include <event.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

namespace fredis { namespace redis {
class RedisClient;
}}

namespace fredis { namespace redis { namespace hiredis_adapter {

typedef struct fredisLibeventEvents {
    redisAsyncContext *context {nullptr};
    RedisClient *client {nullptr};
    struct event rev, wev;
} fredisLibeventEvents;

void fredisLibeventReadEvent(int fd, short event, void *arg);

void fredisLibeventWriteEvent(int fd, short event, void *arg);

void fredisLibeventAddRead(void *privdata);

void fredisLibeventDelRead(void *privdata);

void fredisLibeventAddWrite(void *privdata);

void fredisLibeventDelWrite(void *privdata);

void fredisLibeventCleanup(void *privdata);

int fredisLibeventAttach(RedisClient *client,
    redisAsyncContext *ac,
    struct event_base *base);

}}} // fredis::redis::hiredis_adapter

