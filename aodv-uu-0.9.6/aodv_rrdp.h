//
// Created by buaa on 12/21/18.
//
#ifndef _AODV_RRDP_H
#define _AODV_RRDP_H

#ifndef NS_NO_GLOBALS
#include <endian.h>
#include "defs.h"
#include "routing_table.h"
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
#error "Adjust your <bits/endian.h> defines"
#endif

    u_int8_t hcnt;
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
    u_int32_t orig_addr;
    u_int32_t lifetime;


    u_int32_t Cost;
    u_int32_t Channel;
} RRDP;

#define RRDP_SIZE sizeof(RRDP)

#endif				/* NS_NO_GLOBALS */

#ifndef NS_NO_DECLARATIONS

RRDP *rrdp_create(u_int8_t flags,
                  struct in_addr dest_addr,
                  u_int32_t dest_seqno,
                  struct in_addr orig_addr, u_int32_t life,u_int32_t Channel=0);

void rrdp_send(RRDP * rrdp, rt_table_t * rev_rt, rt_table_t * fwd_rt, int size);
void rrdp_forward(RRDP * rrdp, int size, rt_table_t * rev_rt,
                  rt_table_t * fwd_rt, int ttl);
void rrdp_process(RRDP * rrdp, int rrdplen, struct in_addr ip_src,
                  struct in_addr ip_dst, int ip_ttl, unsigned int ifindex);
#endif				/* NS_NO_DECLARATIONS */

#endif				/* AODV_RRDP_H */
