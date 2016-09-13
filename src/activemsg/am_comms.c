/*
 *
 * Copyright (c) 2011 - 2016
 *   University of Houston System and UT-Battelle, LLC.
 * Copyright (c) 2009 - 2016
 *   Silicon Graphics International Corp.  SHMEM is copyrighted
 *   by Silicon Graphics International Corp. (SGI) The OpenSHMEM API
 *   (shmem) is released by Open Source Software Solutions, Inc., under an
 *   agreement with Silicon Graphics International Corp. (SGI).
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * o Neither the name of the University of Houston System,
 *   UT-Battelle, LLC. nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>              /* NULL */
#include <string.h>             /* memcpy() */
#include <sys/types.h>          /* size_t */
#include "trace.h"

#include "shmem.h"
#include "comms/comms.h"
#include "uthash.h"

#ifdef HAVE_FEATURE_PSHMEM
#include "pshmem.h"
#endif /* HAVE_FEATURE_PSHMEM */

#if defined(HAVE_FEATURE_EXPERIMENTAL)

struct shmemx_am_handler2id_map *am_maphashptr=NULL;
volatile struct shmemx_am_handler2id_map bcast_handler_entry;
volatile int request_cnt = 0;


void 
shmemx_am_attach (int function_id, shmemx_am_handler_w_token function_handler, int metric_choice_code)
{
   struct shmemx_am_handler2id_map* temp_handler_entry;
   struct shmemx_am_handler2id_map* temp_entry;
   temp_handler_entry = (struct shmemx_am_handler2id_map*) malloc(sizeof(struct shmemx_am_handler2id_map));
   temp_handler_entry->id = function_id;
   temp_handler_entry->fn_ptr = function_handler;

#ifdef EESHMEM_ENABLED
   temp_handler_entry->metric_choice_code = metric_choice_code;
   temp_handler_entry->pstate_direction = 1;
   temp_handler_entry->best_pstate = ORIGINAL_PSTATE;
   temp_handler_entry->next_pstate = ORIGINAL_PSTATE;
   temp_handler_entry->best_metric = DBL_MAX;
   temp_handler_entry->timestamp   = 0 ;
   temp_handler_entry->flag = INIT;
   bcast_handler_entry.id=-1;
#endif

   HASH_FIND_INT(am_maphashptr, &function_id, temp_entry);
   if(temp_entry!=NULL) {
	   fprintf(stderr,"*****Error, handler id%d already registered\n",function_id);
   	   exit(-1);
   }
   HASH_ADD_INT(am_maphashptr, id, temp_handler_entry);
   /* shmem attach is a collective operation. This might change in the future */
   shmem_barrier_all();
}

void
shmemx_am_detach(int handler_id)
{
   struct shmemx_am_handler2id_map* temp_handler_entry;
   HASH_FIND_INT( am_maphashptr, &handler_id, temp_handler_entry );
   HASH_DEL( am_maphashptr, temp_handler_entry);  /* delete - pointer to handler */
   free(am_maphashptr);                           /* optional */
}

#ifdef EESHMEM_ENABLED
#ifdef EESHMEM_GLOBAL_UPDATE

#define SET_PADDING_BUF \
         void* temp_source_addr = source_addr; \
         source_addr = malloc(nbytes+2*REC_SIZE); \
         if(nbytes) memcpy((source_addr+2*REC_SIZE),temp_source_addr,nbytes);\
         memset(source_addr,0,2*REC_SIZE); \
         nbytes+=2*REC_SIZE;

#define RESET_PADDING_BUF \
        free(source_addr); \
        source_addr = temp_source_addr;

#define PACK_RECENT_CTRL_UPDATE \
      memcpy((source_addr+OFFSET_RECENT_RECORD), &(bcast_handler_entry),sizeof(struct shmemx_am_handler2id_map)); \
      bcast_handler_entry.id=-1; \

#define PACK_GLOBAL_CTRL_UPDATE \
   struct shmemx_am_handler2id_map* remotetemp_handler_entry; \
   HASH_FIND_INT( am_maphashptr, &handler_id, remotetemp_handler_entry ); \
   memcpy((source_addr+OFFSET_GLOBAL_RECORD), remotetemp_handler_entry, \
           sizeof(struct shmemx_am_handler2id_map)); \

#endif /* EESHMEM_GLOBAL_UPDATE */
#endif /* EESHMEM_ENABLED */ 


void
shmemx_am_request(int dest, int handler_id, void* source_addr, size_t nbytes)
{
#ifdef EESHMEM_ENABLED
   #ifdef EESHMEM_GLOBAL_UPDATE
     SET_PADDING_BUF
     PACK_RECENT_CTRL_UPDATE
     PACK_GLOBAL_CTRL_UPDATE
   #endif
#endif
   request_cnt++;
   //atomic_inc_am_counter();
   GASNET_SAFE(gasnet_AMRequestMedium2 
		   (dest, GASNET_HANDLER_activemsg_request_handler, 
		    source_addr, nbytes, 
		    handler_id, 
		    shmem_my_pe()));
#ifdef EESHMEM_ENABLED
   #ifdef EESHMEM_GLOBAL_UPDATE
     RESET_PADDING_BUF
   #endif
#endif
}


void
shmemx_am_reply(int handler_id, void* source_addr, size_t nbytes, shmemx_am_token_t temp_token)
{
#ifdef EESHMEM_ENABLED
   #ifdef EESHMEM_GLOBAL_UPDATE
     SET_PADDING_BUF
     PACK_RECENT_CTRL_UPDATE
     PACK_GLOBAL_CTRL_UPDATE
  #endif
#endif
   GASNET_SAFE(gasnet_AMReplyMedium2 
		  ((gasnet_token_t)temp_token->gasnet_token, GASNET_HANDLER_activemsg_reply_handler, 
	           source_addr, nbytes, 
		   handler_id, 
		   shmem_my_pe()));
   temp_token->is_reply_called = 1;
#ifdef EESHMEM_ENABLED
   #ifdef EESHMEM_GLOBAL_UPDATE
     RESET_PADDING_BUF
   #endif
#endif
}



void
shmemx_am_quiet()
{
   //atomic_wait_am_zero();
   GASNET_BLOCKUNTIL(request_cnt==0);
}


void 
shmemx_am_poll()
{
   gasnet_AMPoll();
}


#define MAX_HSL_CNT 100
gasnet_hsl_t gasnet_hsl_arr[MAX_HSL_CNT];
static int shmemx_am_mutex_counter=-1;
typedef int shmemx_am_mutex;

#define SHMEM2GASNET_HSL(op,ret)\
ret shmemx_am_mutex_##op(shmemx_am_mutex* index) \
{ \
   return gasnet_hsl_##op(&gasnet_hsl_arr[*index]); \
}

void shmemx_am_mutex_init(shmemx_am_mutex* index)
{
   *index = ++shmemx_am_mutex_counter;
   gasnet_hsl_init(&gasnet_hsl_arr[*index]);
}

SHMEM2GASNET_HSL(destroy,void)
SHMEM2GASNET_HSL(lock,void)
SHMEM2GASNET_HSL(unlock,void)
SHMEM2GASNET_HSL(trylock,int)

#endif /* HAVE_FEATURE_EXPERIMENTAL */
