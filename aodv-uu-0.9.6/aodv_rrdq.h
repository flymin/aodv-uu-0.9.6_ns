//
// Created by buaa on 12/28/17.
//

#ifndef _AODV_RRDQ_H
#define _AODV_RRDQ_H


#ifndef NS_NO_GLOBALS

#include <endian.h>

#include "defs.h"
#include "seek_list.h"
#include "routing_table.h"
#include "aodv_rreq.h"

typedef struct {
    u_int8_t type;
#if defined(__LITTLE_ENDIAN)
    u_int8_t res1:4;
    u_int8_t d:1;
    u_int8_t g:1;
    u_int8_t r:1;
    u_int8_t j:1;
#elif defined(__BIG_ENDIAN)
    u_int8_t j:1;		/* Join flag (multicast) */
    u_int8_t r:1;		/* Repair flag */
    u_int8_t g:1;		/* Gratuitous RREP flag */
    u_int8_t d:1;		/* Destination only respond */
    u_int8_t res1:4;
#else

#endif

    u_int32_t rrdq_id;
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
    u_int32_t orig_addr;
    u_int32_t orig_seqno;
    u_int8_t hcnt;

    u_int32_t Cost;
    u_int32_t Channel;

} RRDQ;




#define RRDQ_SIZE sizeof(RRDQ)

#endif				/* NS_NO_GLOBALS */


#ifndef NS_NO_DECLARATIONS

#endif /* NS_NO_DECLARATIONS */


RRDQ *rrdq_create(u_int8_t flags, struct in_addr dest_addr,
                  u_int32_t dest_seqno, struct in_addr orig_addr);

void rrdq_send(struct in_addr dest_addr, u_int32_t dest_seqno, int ttl,
               u_int8_t flags);

void rrdq_forward(RRDQ * rrdq, int size, int ttl);

void rrdq_process(RRDQ * rrdq, int rrdqlen, struct in_addr ip_src,
                  struct in_addr ip_dst, int ip_ttl, unsigned int ifindex);

#endif //RRDQ_H
