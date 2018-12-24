#ifndef _AODV_RRCQ_H
#define _AODV_RRCQ_H

#ifndef NS_NO_GLOBALS
#include "defs.h"
#include "routing_table.h"
/* RREQ Flags: */
#define RRCQ_JOIN          0x1
#define RRCQ_REPAIR        0x2
#define RRCQ_GRATUITOUS    0x4
#define RRCQ_DEST_ONLY     0x8

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
    u_int8_t res2;
    u_int8_t hcnt;
    u_int32_t rrcq_id;
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
    u_int32_t orig_addr;
    u_int32_t orig_seqno;




    u_int32_t Cost;
    u_int32_t Channel;
    u_int32_t dest_count;
} RRCQ;
typedef struct {
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
} RRCQ_udest;

#define RRCQ_SIZE sizeof(RRCQ)
#define RRCQ_UDEST_SIZE sizeof(RRCQ_udest)

#define RRCQ_CALC_SIZE(rrcq) (RRCQ_SIZE + (rrcq->dest_count)*RRCQ_UDEST_SIZE) //这里和rreq 不同

#define RRCQ_UDEST_FIRST(rrcq) (RRCQ_udest *)((char*)(rrcq)+RRCQ_SIZE)

#define RRCQ_UDEST_NEXT(udest) ((RRCQ_udest *)((char *)udest + RRCQ_UDEST_SIZE))

#define RRCQ_SIZE sizeof(RRCQ)

/* A data structure to buffer information about received RREQ's */
struct rrcq_record {
    list_t l;
    struct in_addr orig_addr;	/* Source of the RREQ */
    u_int32_t rreq_id;		/* RRCQ's broadcast ID */
    struct timer rec_timer;


    struct in_addr src_addr;

};

struct blacklist_rrcq {
    list_t l;
    struct in_addr dest_addr;
    struct timer bl_timer;
};
#endif				/* NS_NO_GLOBALS */

#ifndef NS_NO_DECLARATIONS

RRCQ *rrcq_create(u_int8_t flags, int dest_addr, u_int32_t dest_seqno, int orig_addr);
void rrcq_send(struct in_addr dest_addr, u_int32_t dest_seqno, int ttl, u_int8_t flags);
void rrcq_forward(RRCQ * rreq, int size, int ttl);
void rrcq_process(RRCQ * rreq, int rreqlen, struct in_addr ip_src, struct in_addr ip_dst, int ip_ttl, unsigned int ifindex);
void rrcq_route_discovery(struct in_addr dest_addr, u_int8_t flags, struct ip_data *ipd);
void rrcq_record_timeout(void *arg);
void rrcq_blacklist_timeout(void *arg);

void  rrcq_add_udest(RRCQ * rrcq, struct in_addr udest,
                             u_int32_t udest_seqno)
{
    RRCQ_udest *ud;

    ud = (RRCQ_udest *) ((char *)rrcq + RRCQ_CALC_SIZE(rrcq));
    ud->dest_addr = udest.s_addr;
    ud->dest_seqno = htonl(udest_seqno);
    rrcq->dest_count++;
    fprintf(stderr,"add udest:dest_addr:%s,dest_seqno:%d\n",ip_to_str(udest),udest_seqno);
}

void rrcq_local_repair(rt_table_t * rt, struct in_addr src_addr,
                       struct ip_data *ipd);




#ifdef NS_PORT
struct rrcq_record *rrcq_record_insert(struct in_addr orig_addr,
				       u_int32_t rreq_id,struct in_addr src_addr);
struct rrcq_record *rrcq_record_find(struct in_addr orig_addr,
				     u_int32_t rreq_id , struct in_addr src_addr);
struct blacklist *rrcq_blacklist_find(struct in_addr dest_addr);
#endif				/* NS_PORT */


#endif /* NS_NO_DECLARATIONS */

#endif //NS_2_35_AODV_RRCQ_H
