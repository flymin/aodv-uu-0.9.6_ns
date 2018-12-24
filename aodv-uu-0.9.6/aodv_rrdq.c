//
// Created by buaa on 12/21/18.
//

#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else
#include <netinet/in.h>

#include "aodv_rrdq.h"
#include "aodv_rrdp.h"
#include "routing_table.h"
#include "aodv_timeout.h"
#include "timer_queue.h"
#include "aodv_socket.h"
#include "params.h"
#include "seek_list.h"
#include "defs.h"
#include "debug.h"
#include "aodv_rrcq.h"
#include "locality.h"
#endif

#define DEBUG_OUTPUT


RRDQ *NS_CLASS rrdq_create(u_int8_t flags, struct in_addr dest_addr,
                           u_int32_t dest_seqno, struct in_addr orig_addr)
{

    //fprintf(stderr,"rrdq_create:orig_addr:%d,dest_addr:%d,time:%ld,%ld\n",orig_addr.s_addr,dest_addr.s_addr,now.tv_sec,now.tv_usec);
    RRDQ *rrdq;

    rrdq = (RRDQ *) aodv_socket_new_msg();
    rrdq->type = AODV_RRDQ;
    rrdq->hcnt = 0;
    rrdq->rrdq_id = htonl(this_host.rreq_id++);
    rrdq->dest_addr = dest_addr.s_addr;
    rrdq->dest_seqno = htonl(dest_seqno);
    rrdq->orig_addr = orig_addr.s_addr;
    struct timeval now;
    gettimeofday(&now,NULL);

    seqno_incr(this_host.seqno);
    rrdq->orig_seqno = htonl(this_host.seqno);

    rrdq->Cost = 0;
    rrdq->Channel = 0;

    fprintf(stderr,"create rrdq :orig_addr:%s,dest_addr:%s,rrdq_id:%d\n",ip_to_str(orig_addr),ip_to_str(dest_addr),rrdq->rrdq_id);


#ifdef DEBUG_OUTPUT
    log_pkt_fields((AODV_msg *) rrdq);
#endif

    return rrdq;
}

void NS_CLASS rrdq_send(struct in_addr dest_addr,u_int32_t dest_seqno,
                        int ttl, u_int8_t flags)
{
    RRDQ *rrdq;
    struct in_addr dest;
    int i;

    dest.s_addr = AODV_BROADCAST;
    if (rreq_gratuitous)
        flags |= RREQ_GRATUITOUS;
    /* Broadcast on all interfaces */
    for (i = 0; i < MAX_NR_INTERFACES; i++) {
        if (!DEV_NR(i).enabled)
            continue;
        rrdq = rrdq_create(flags, dest_addr, dest_seqno, DEV_NR(i).ipaddr);
        aodv_socket_send((AODV_msg *) rrdq, dest, RRDQ_SIZE, ttl, &DEV_NR(i));
    }

    return ;
}

void NS_CLASS rrdq_forward(RRDQ * rrdq, int size, int ttl)
{
    struct in_addr dest, orig;
    int i;

    dest.s_addr = AODV_BROADCAST;
    orig.s_addr = rrdq->orig_addr;

    /* FORWARD the RRDQ if the TTL allows it. */


    /* Queue the received message in the send buffer */
    rrdq = (RRDQ *) aodv_socket_queue_msg((AODV_msg *) rrdq, size);

    rrdq->hcnt++;       /* Increase hopcount to account for
                 * intermediate route */

    fprintf(stderr, "rrdq_forward RRDQ src=%s, rrdq_id=%lu",
            ip_to_str(orig), ntohl(rrdq->rrdq_id));

    /* Send out on all interfaces */
    for (i = 0; i < MAX_NR_INTERFACES; i++) {
        if (!DEV_NR(i).enabled)
            continue;
        aodv_socket_send((AODV_msg *) rrdq, dest, size, ttl, &DEV_NR(i));
    }
}


void NS_CLASS rrdq_process(RRDQ * rrdq, int rrdqlen, struct in_addr ip_src,
                           struct in_addr ip_dst, int ip_ttl,
                           unsigned int ifindex)
{

    RRDP *rrdp = NULL;
    int rrdp_size = RRDP_SIZE;
    rt_table_t *rev_rt, *fwd_rt = NULL;
    u_int32_t rrdq_orig_seqno, rrdq_dest_seqno;
    u_int32_t rrdq_id, rrdq_new_hcnt, life;
    unsigned int extlen = 0;
    struct in_addr rrdq_dest, rrdq_orig;

    u_int32_t rrdq_Cost,rrdq_Channel;


    rrdq_dest.s_addr = rrdq->dest_addr;
    rrdq_orig.s_addr = rrdq->orig_addr;
    rrdq_id = ntohl(rrdq->rrdq_id);
    rrdq_dest_seqno = ntohl(rrdq->dest_seqno);
    rrdq_orig_seqno = ntohl(rrdq->orig_seqno);
    rrdq_new_hcnt = rrdq->hcnt + 1;

    rrdq_Channel = rrdq->Channel;
    rrdq_Cost = rrdq->Cost + nb_table_find(ip_src, rrdq_Channel, true)->cost;

    /* Ignore RRDQ's that originated from this node. Either we do this
       or we buffer our own sent RRDQ's as we do with others we
       receive. */
    if (rrdq_orig.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr)
        return;


    fprintf(stderr,  " ip_src=%s rrdq_orig=%s rrdq_dest=%s ttl=%d\n",
            ip_to_str(ip_src), ip_to_str(rrdq_orig), ip_to_str(rrdq_dest),
            ip_ttl);

    if (rrdqlen < (int) RRDQ_SIZE) {
        alog(LOG_WARNING, 0,
             __FUNCTION__, "IP data field too short (%u bytes)"
                     "from %s to %s", rrdqlen, ip_to_str(ip_src), ip_to_str(ip_dst));
        return;
    }


    if (rrcq_blacklist_find(ip_src)) {
        DEBUG(LOG_DEBUG, 0, "prev hop of RRDQ blacklisted, ignoring!");
        fprintf(stderr, "prev hop of RRDQ blacklisted, ignoring!");

        return;
    }

    /* Ignore already processed RRDQs. */
 //   if (!rrcq_record_find(rrdq_orig, rrdq_id,ip_src))
 //       ;

    rrcq_record_insert(rrdq_orig, rrdq_id,ip_src);

    fprintf(stderr,"rrdq_process: current: %d , from:%d , to:%d\n",DEV_IFINDEX(ifindex).ipaddr.s_addr,rrdq_orig.s_addr,rrdq_dest.s_addr);

#ifdef DEBUG_OUTPUT
    log_pkt_fields((AODV_msg *) rrdq);
#endif

    /* The node always creates or updates a REVERSE ROUTE entry to the
       source of the RREQ. */
    rev_rt = rt_table_find(rrdq_orig);

    /* Calculate the extended minimal life time. */
    life = PATH_DISCOVERY_TIME - 2 * rrdq_new_hcnt * NODE_TRAVERSAL_TIME;



    // fprintf(stderr,"rrdq_process:rrdq_new_cost:%d\n",rrdq_new_cost);
    if (rev_rt == NULL) {
        DEBUG(LOG_DEBUG, 0, "Creating REVERSE route entry, RRDQ orig: %s",
              ip_to_str(rrdq_orig));

        rev_rt = rt_table_insert(rrdq_orig, ip_src, rrdq_new_hcnt,
                                 rrdq_orig_seqno, life, INVALID, 0, ifindex,
				 rrdq_Channel, rrdq_Cost);
    } else {
        if (rev_rt->dest_seqno == 0 ||
            (int32_t) rrdq_orig_seqno > (int32_t) rev_rt->dest_seqno ||
            (rrdq_orig_seqno == rev_rt->dest_seqno &&
             (rev_rt->state == INVALID || rrdq_Cost< rev_rt->LA))) {

            rev_rt = rt_table_update(rev_rt, ip_src, rrdq_new_hcnt,
                                     rrdq_orig_seqno, life, INVALID,
                                     rev_rt->flags,rrdq_Channel,rrdq_Cost);
       }
    }



    if (rrdq_dest.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr) {

        /* WE are the RRDQ DESTINATION. Update the node's own
           sequence number to the maximum of the current seqno and the
           one in the RRDQ. */
        if (rrdq_dest_seqno != 0) {
            if ((int32_t) this_host.seqno < (int32_t) rrdq_dest_seqno)
                this_host.seqno = rrdq_dest_seqno;
            else if (this_host.seqno == rrdq_dest_seqno)
                seqno_incr(this_host.seqno);
        }
        fprintf(stderr,"<-----starting send rrdp ------->  \n" );
        rrdp=rrdp_create(0,DEV_IFINDEX(rev_rt->ifindex).ipaddr,this_host.seqno, \
                         rev_rt->dest_addr , MY_ROUTE_TIMEOUT,rrdq_Channel);
        rrdp_send(rrdp,rev_rt,NULL,RRDP_SIZE);


    } else {
        /* We are an INTERMEDIATE node. - check if we have an active
         * route entry */

        fwd_rt = rt_table_find(rrdq_dest);

        forward:
        if (ip_ttl > 1) {
            /* Update the sequence number in case the maintained one is
             * larger */
            if (fwd_rt && !(fwd_rt->flags & RT_INET_DEST)&&   \
                    (int32_t) fwd_rt->dest_seqno > (int32_t) rrdq_dest_seqno)   \
                rrdq->dest_seqno = htonl(fwd_rt->dest_seqno);


            rrdq_forward(rrdq, rrdqlen, --ip_ttl);

        } else {
            fprintf(stderr,"rrdq not forward\n");
            DEBUG(LOG_DEBUG, 0, "RRDQ not forwarded - ttl=0");
        }
    }
}
