//
// Created by buaa on 12/26/17.
//

#ifndef _AODV_RRCP_H
#define _AODV_RRCP_H

#ifndef NS_NO_GLOBALS
#include "defs.h"
#include "routing_table.h"
#include "aodv_rrcq.h"

typedef struct {
    u_int8_t type;

    u_int8_t hcnt;

    u_int32_t lifetime;
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
    u_int32_t orig_addr;
    u_int32_t orig_seqno;

    u_int32_t Cost;
    u_int32_t Channel;
    u_int32_t dest_count;
}RRCP ;
typedef struct {
    u_int32_t dest_addr;
    u_int32_t dest_seqno;
} RRCP_udest;

#define RRCP_SIZE sizeof(RRCP)

#define RRCP_UDEST_SIZE sizeof(RRCP_udest)

#define RRCP_CALC_SIZE(rrcp) (RRCP_SIZE + (rrcp->dest_count)*RRCP_UDEST_SIZE) //这里和rreq 不同

#define RRCP_UDEST_FIRST(rrcp) (RRCP_udest *)((char*)(rrcp)+RRCP_SIZE)

#define RRCP_UDEST_NEXT(udest) ((RRCP_udest *)((char *)udest + RRCP_UDEST_SIZE))


#endif				/* NS_NO_GLOBALS */




#ifndef NS_NO_DECLARATIONS

RRCP* rrcp_create(RRCQ * rrcq,
                  u_int8_t flags,
                  int hcnt,
                  int cost,
                  u_int32_t life,u_int32_t Channel
                  );

AODV_ext *rrcp_add_ext(RRCP * rrcp, int type, unsigned int offset,
                       int len, char *data);

void rrcp_forward(RRCP * rrcp,  int size,rt_table_t* rev_rt,rt_table_t* fwd_rt,int  ttl);
void rrcp_process(RRCP * rrcp, int rrcplen, struct in_addr ip_src,
                  struct in_addr ip_dst, int ip_ttl,
                  unsigned int ifindex);
void rrcp_send(RRCP * rrcp, rt_table_t * rev_rt, rt_table_t * fwd_rt, int size);

#endif /* NS_NO_DECLARATIONS */

#endif //NS_2_35_AODV_RRCP_H
