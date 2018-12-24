//
// Created by buaa on 12/21/18.
//
#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else
#include <netinet/in.h>
#include "aodv_rrdp.h"
#include "aodv_neighbor.h"
#include "aodv_hello.h"
#include "routing_table.h"
#include "aodv_timeout.h"
#include "timer_queue.h"
#include "aodv_socket.h"
#include "defs.h"
#include "debug.h"
#include "params.h"

extern int unidir_hack, optimized_hellos, llfeedback;

#endif


RRDP *NS_CLASS rrdp_create(u_int8_t flags,
                           struct in_addr dest_addr,
                           u_int32_t dest_seqno,
                           struct in_addr orig_addr, u_int32_t life ,u_int32_t Channel)
{
    RRDP *rrdp;

    rrdp = (RRDP *) aodv_socket_new_msg();
    rrdp->type = AODV_RRDP;

    rrdp->hcnt = 0;
    rrdp->dest_addr = dest_addr.s_addr;
    rrdp->dest_seqno = htonl(dest_seqno);
    rrdp->orig_addr = orig_addr.s_addr;
    rrdp->lifetime = htonl(life);
    rrdp->Channel = Channel;
    rrdp->Cost = 0;
    fprintf(stderr,"create rrdp :orig_addr:%s,dest_addr:%s\n",ip_to_str(orig_addr),ip_to_str(dest_addr));

    /* Don't print information about hello messages... */
#ifdef DEBUG_OUTPUT
    if (rrdp->dest_addr != rrdp->orig_addr) {
	    log_pkt_fields((AODV_msg *) rrdp);
    }
#endif

    return rrdp;
}
void NS_CLASS rrdp_send(RRDP * rrdp, rt_table_t * rev_rt, rt_table_t * fwd_rt, int size)
{
    u_int8_t rrdp_flags = 0;
    struct in_addr dest;

    if (!rev_rt) {
        DEBUG(LOG_WARNING, 0, "Can't send RRDP, rev_rt = NULL!");
        return;
    }

    dest.s_addr = rrdp->dest_addr;
//by dormosue: we dont need ack just like rrcp
    aodv_socket_send((AODV_msg *) rrdp, rev_rt->next_hop, size, MAXTTL,
                     &DEV_IFINDEX(rev_rt->ifindex));

    /* Update precursor lists */
    if (fwd_rt) {
        precursor_add(fwd_rt, rev_rt->next_hop);
        precursor_add(rev_rt, fwd_rt->next_hop);
    }

    if (!llfeedback && optimized_hellos)
    hello_start();
}

void NS_CLASS rrdp_forward(RRDP * rrdp, int size, rt_table_t * rev_rt,
                           rt_table_t * fwd_rt, int ttl)
{
    /* Sanity checks... */
    if (!fwd_rt || !rev_rt) {
    DEBUG(LOG_WARNING, 0, "Could not forward RRDP because of NULL route!");
    return;
    }

    if (!rrdp) {
    DEBUG(LOG_WARNING, 0, "No RRDP to forward!");
    return;
    }

    DEBUG(LOG_DEBUG, 0, "Forward RRDP to %s\n", ip_to_str(rev_rt->next_hop));


    rrdp = (RRDP *) aodv_socket_queue_msg((AODV_msg *) rrdp, size);



    aodv_socket_send((AODV_msg *) rrdp, rev_rt->next_hop, size, ttl,
                     &DEV_IFINDEX(rev_rt->ifindex));

    precursor_add(fwd_rt, rev_rt->next_hop);
    precursor_add(rev_rt, fwd_rt->next_hop);

    rt_table_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);
}

void NS_CLASS rrdp_process(RRDP * rrdp, int rrdplen, struct in_addr ip_src,
                           struct in_addr ip_dst, int ip_ttl,
                           unsigned int ifindex)
{
    u_int32_t rrdp_lifetime, rrdp_seqno;
    u_int8_t rrdp_new_hcnt,rrdp_new_cost;
    u_int8_t pre_repair_hcnt = 0, pre_repair_flags = 0;

    rt_table_t *fwd_rt, *rev_rt;
    AODV_ext *ext;
    unsigned int extlen = 0;
    int rt_flags = 0;
    struct in_addr rrdp_dest, rrdp_orig;


    u_int32_t rrdp_Channel,rrdp_Cost;
#ifdef CONFIG_GATEWAY
    struct in_addr inet_dest_addr;
    int inet_rrdp = 0;
#endif

    /* Convert to correct byte order on affeected fields: */
    rrdp_dest.s_addr = rrdp->dest_addr;
    rrdp_orig.s_addr = rrdp->orig_addr;
    rrdp_seqno = ntohl(rrdp->dest_seqno);
    rrdp_lifetime = ntohl(rrdp->lifetime);
    /* Increment RRDP hop count to account for intermediate node... */
    rrdp_Channel = rrdp->Channel;
    rrdp_Cost = rrdp->Cost + nb_table_find(ip_src, rrdp_Channel, true)->cost;
    fprintf(stderr,"process rrdp ,from :%d to: %d \n",ip_to_str(ip_src),ip_to_str(ip_dst));


    if (rrdplen < (int) RRDP_SIZE) {
        alog(LOG_WARNING, 0, __FUNCTION__,
             "IP data field too short (%u bytes)"
                     " from %s to %s", rrdplen, ip_to_str(ip_src), ip_to_str(ip_dst));
        return;
    }

    /* Ignore messages which aim to a create a route to one self */
    if (rrdp_dest.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr)
        return;

    DEBUG(LOG_DEBUG, 0, "from %s about %s->%s",
          ip_to_str(ip_src), ip_to_str(rrdp_orig), ip_to_str(rrdp_dest));
#ifdef DEBUG_OUTPUT
    log_pkt_fields((AODV_msg *) rrdp);
#endif


    fwd_rt = rt_table_find(rrdp_dest);
    rev_rt = rt_table_find(rrdp_orig);

     if (!fwd_rt) {
        /* We didn't have an existing entry, so we insert a new one. */
        fwd_rt = rt_table_insert(rrdp_dest, ip_src, rrdp_new_hcnt, rrdp_seqno,
                                 rrdp_lifetime, VALID, rt_flags, ifindex,
				 rrdp_Channel, rrdp_Cost);
    } else if (fwd_rt->dest_seqno == 0 ||
               (int32_t) rrdp_seqno > (int32_t) fwd_rt->dest_seqno ||
               (rrdp_seqno == fwd_rt->dest_seqno &&
                (fwd_rt->state == INVALID || fwd_rt->flags & RT_UNIDIR ||rrdp_Cost < fwd_rt->LA
                ))) {
        pre_repair_hcnt = fwd_rt->hcnt;
        pre_repair_flags = fwd_rt->flags;

        fwd_rt = rt_table_update(fwd_rt, ip_src, rrdp_new_hcnt, rrdp_seqno,
                                 rrdp_lifetime, VALID,
                                 rt_flags | fwd_rt->flags,rrdp_Channel,rrdp_Cost);
    } else {
        //fprintf(stderr,"rrdp_process:2\n");
        if (fwd_rt->hcnt > 1) {
            DEBUG(LOG_DEBUG, 0,
                  "Dropping RRDP, fwd_rt->hcnt=%d fwd_rt->seqno=%ld",
                  fwd_rt->hcnt, fwd_rt->dest_seqno);
        }
        return;
    }


    if (rrdp_orig.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr) {
        return ;  //结束使命
    }

    if (rev_rt && rev_rt->state == VALID) {
        rrdp_forward(rrdp, rrdplen, rev_rt, fwd_rt, --ip_ttl);
    } else {
        DEBUG(LOG_DEBUG, 0, "Could not forward RRDP - NO ROUTE!!!");
    }


    if (!llfeedback && optimized_hellos)
        hello_start();
}

