//
// Created by buaa on 12/26/17.
//

#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else
#include "aodv_rrcp.h"
#include <netinet/in.h>
#include "routing_table.h"
#include "aodv_neighbor.h"
#include "aodv_hello.h"
#include "routing_table.h"
#include "aodv_timeout.h"
#include "timer_queue.h"
#include "aodv_socket.h"
#include "defs.h"
#include "debug.h"
#include "params.h"
#include "seek_list.h"
#include "aodv_rrcq.h"
#endif

RRCP *NS_CLASS rrcp_create(RRCQ * rrcq,u_int8_t flags, int hcnt, int cost, u_int32_t life)
{

    RRCP *rrcp;

    rrcp = (RRCP *) aodv_socket_queue_msg((AODV_msg *) rrcq,RRCP_CALC_SIZE(rrcq));

    rrcp->type = AODV_RRCP;
    rrcp->cost = 0;
    rrcp->hcnt = 0;
    rrcp->lifetime =htonl(life);
    struct in_addr dest_addr,orig_addr;
    dest_addr.s_addr=rrcq->dest_addr;
    orig_addr.s_addr=rrcq->orig_addr;
    DEBUG (LOG_DEBUG ,0,"create rrcp::  to:%s  from:%s \n",ip_to_str(dest_addr),ip_to_str(orig_addr));
    fprintf(stderr ,"create rrcp::  to:%s  from:%s \n",ip_to_str(dest_addr),ip_to_str(orig_addr));
    rrcp->dest_count=rrcq->dest_count;
    seqno_incr(this_host.seqno);//zhe li yao zizeng 1
    rrcp->dest_seqno = this_host.seqno;


#ifdef DEBUG_OUTPUT
    if (rrcp->dest_addr != rrcp->orig_addr) {
		DEBUG(LOG_DEBUG, 0, "Assembled RRCP:");
		log_pkt_fields((AODV_msg *) rrcp);
	}
#endif

    return rrcp;
}

void  NS_CLASS rrcp_send(RRCP * rrcp, rt_table_t * rev_rt, rt_table_t * fwd_rt, int size)
{
    DEBUG(LOG_DEBUG, 0, " send RRCP");

    u_int8_t rrcp_flags = 0;
    struct in_addr dest;

    if (!rev_rt) {
        DEBUG(LOG_WARNING, 0, "Can't send RRCP, rev_rt = NULL!");
        return;
    }

    dest.s_addr = rrcp->dest_addr;


    DEBUG(LOG_DEBUG, 0, "Sending RRCP to next hop %s about %s->%s",
          ip_to_str(rev_rt->next_hop), ip_to_str(rev_rt->dest_addr),
          ip_to_str(dest));

    fprintf(stderr, "Sending RRCP to next hop %s about %s->%s",
          ip_to_str(rev_rt->next_hop), ip_to_str(rev_rt->dest_addr),
          ip_to_str(dest));
    aodv_socket_send((AODV_msg *) rrcp, rev_rt->next_hop, size, MAXTTL,
                     &DEV_IFINDEX(rev_rt->ifindex));

    /* Update precursor lists */
    if (fwd_rt) {
        precursor_add(fwd_rt, rev_rt->next_hop);
        precursor_add(rev_rt, fwd_rt->next_hop);
    }

//    if (!llfeedback && optimized_hellos)
//        hello_start();

}

void NS_CLASS rrcp_forward(RRCP * rrcp, rt_table_t * rev_rt,
                           rt_table_t * fwd_rt, int ttl)
{

    if (!fwd_rt || !rev_rt) {
        DEBUG(LOG_WARNING, 0,
              "Could not forward RRCP because of NULL route!");
        return;
    }

    if (!rrcp) {
        DEBUG(LOG_WARNING, 0, "No RRCP to forward!");
        return;
    }

    rrcp =
            (RRCP *) aodv_socket_queue_msg((AODV_msg *) rrcp,
                                          RRCP_CALC_SIZE(rrcp));
    rrcp->hcnt = fwd_rt->hcnt;	/* Update the hopcount */

    aodv_socket_send((AODV_msg *) rrcp, rev_rt->next_hop, RRCP_CALC_SIZE(rrcp), ttl,
                     &DEV_IFINDEX(rev_rt->ifindex));

    precursor_add(fwd_rt, rev_rt->next_hop);
    precursor_add(rev_rt, fwd_rt->next_hop);

    rt_table_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);


}

void NS_CLASS rrcp_process(RRCP * rrcp, int rrcplen, struct in_addr ip_src,
                           struct in_addr ip_dst, int ip_ttl,
                           unsigned int ifindex)
{

    u_int32_t rrcp_lifetime, rrcp_seqno, rrcp_new_hcnt, udest_seqno;
    u_int8_t pre_repair_hcnt = 0, pre_repair_flags = 0,rerr_flags=0;
    rt_table_t *fwd_rt, *rev_rt, *rt;
    int rt_flags = 0, rrcp_dest_cnt;
    RRCP_udest *udest;
    int if_send_rrer=0;
    RERR* rerr=(RERR*)NULL;


    struct in_addr rrcp_dest, rrcp_orig, udest_addr;

    /* Convert to correct byte order on affeected fields: */
    rrcp_dest.s_addr = rrcp->dest_addr;
    rrcp_orig.s_addr = rrcp->orig_addr;
    rrcp_seqno = ntohl(rrcp->dest_seqno);
    rrcp_lifetime = ntohl(rrcp->lifetime);
    rrcp_dest_cnt = rrcp->dest_count;//
    /* Increment rrcp hop count to account for intermediate node... */
    rrcp_new_hcnt = rrcp->hcnt + 1;

    if (rrcplen < (int)RRCP_SIZE) {
        alog(LOG_WARNING, 0, __FUNCTION__,
             "IP data field too short (%u bytes)"
                     " from %s to %s", rrcplen, ip_to_str(ip_src),
             ip_to_str(ip_dst));
        return;
    }

    /* Ignore messages which aim to a create a route to one self */
    if (rrcp_dest.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr)
        return;


    DEBUG(LOG_DEBUG, 0,"recv rrcp:orig_addr:%s,ip_src:%s,channel:%d,dest_addr:%s\n",ip_to_str(rrcp_orig),ip_to_str(ip_src),ifindex,ip_to_str(rrcp_dest));
    fprintf(stderr,"recv rrcp:orig_addr:%s,ip_src:%s,channel:%d,dest_addr:%s\n",ip_to_str(rrcp_orig),ip_to_str(ip_src),ifindex,ip_to_str(rrcp_dest));

    fwd_rt = rt_table_find(rrcp_dest);
    rev_rt = rt_table_find(rrcp_orig);


    if (!fwd_rt) {
        fwd_rt =
                rt_table_insert(rrcp_dest, ip_src, rrcp_new_hcnt,
                                rrcp_seqno, rrcp_lifetime, VALID, rt_flags,
                                ifindex);
    } else if (fwd_rt->dest_seqno == 0
               || (int32_t) rrcp_seqno > (int32_t) fwd_rt->dest_seqno
               || (rrcp_seqno == fwd_rt->dest_seqno
                   && (fwd_rt->state == INVALID || fwd_rt->flags & RT_UNIDIR
                       || rrcp_new_hcnt < fwd_rt->hcnt))) {

        pre_repair_hcnt = fwd_rt->hcnt;
        pre_repair_flags = fwd_rt->flags;

        fwd_rt =
                rt_table_update(fwd_rt, ip_src, rrcp_new_hcnt, rrcp_seqno,
                                rrcp_lifetime, VALID,
                                (rt_flags | fwd_rt->flags) & (~RT_REPAIR));
    } else {
        if (fwd_rt->hcnt > 1) {
            DEBUG(LOG_DEBUG, 0,
                  "Dropping RRCP, fwd_rt->hcnt=%d fwd_rt->seqno=%ld",
                  fwd_rt->hcnt, fwd_rt->dest_seqno);
        }
        return;
    }



    if (rrcp_orig.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr) {

        if (fwd_rt->hcnt > 1 ) {
            if_send_rrer=1;

            rerr_flags |= RERR_NODELETE;
            rerr = rerr_create(rerr_flags, fwd_rt->dest_addr,
                               fwd_rt->dest_seqno,1);    ///////////
        }
    }

    udest = RRCP_UDEST_FIRST(rrcp);
    fprintf(stderr,"rrcp_process:dest!!!!!!!!!!!!!,count is %d\n",rrcp->dest_count);



    while (rrcp_dest_cnt>0) {


        udest_addr.s_addr = udest->dest_addr;
        udest_seqno = ntohl(udest->dest_seqno);

        DEBUG(LOG_DEBUG, 0, "unreachable dest=%s seqno=%lu",
              ip_to_str(udest_addr), rrcp_seqno);
        fprintf(stderr, "unreachable dest=%s seqno=%lu",
              ip_to_str(udest_addr), rrcp_seqno);
        rt = rt_table_find(udest_addr);
            if(rt!=NULL)
                 rt_table_update(rt, ip_src, rrcp_new_hcnt,
                                     udest_seqno, rrcp_lifetime, VALID,
                                     rt_flags | fwd_rt->flags);///

            if(if_send_rrer){
                rerr_add_udest(rerr, udest_addr,udest_seqno);
            }

        rrcp_dest_cnt--;
        udest = RRCP_UDEST_NEXT(udest);
    }

    //rrdq
    if (rrcp_orig.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr) {

        if(if_send_rrer)
        {
            if (rerr) {

                //fprintf(stderr,"rrcp_process:send_rerr\n");
                int i;
                for (i = 0; i < MAX_NR_INTERFACES; i++) {
                    struct in_addr dest;

                    if (!DEV_NR(i).enabled)
                        continue;
                    else{
                    dest.s_addr = AODV_BROADCAST;
                    aodv_socket_send((AODV_msg *) rerr, dest,
                                     RERR_CALC_SIZE(rerr), 1,
                                     &DEV_NR(i));
                    }
                }

            }
        }

    } else {
        /* --- Here we FORWARD the RRCP on the REVERSE route --- */
        if (rev_rt && rev_rt->state == VALID) {
            rrcp_forward(rrcp, rev_rt, fwd_rt, --ip_ttl);
        } else {
            DEBUG(LOG_DEBUG, 0,
                  "Could not forward RREP - NO ROUTE!!!");
        }
    }

//    if (!llfeedback && optimized_hellos)
//        hello_start();
}
#ifdef AAAA

#endif