//
// Created by buaa on 12/26/17.
//

#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else
#include <netinet/in.h>

#include "aodv_rreq.h"
#include "aodv_rrep.h"
#include "routing_table.h"
#include "aodv_timeout.h"
#include "timer_queue.h"
#include "aodv_socket.h"
#include "params.h"
#include "seek_list.h"
#include "defs.h"
#include "debug.h"

#include "locality.h"

#include "aodv_rrcq.h"
#include "aodv_rrcp.h"
#endif

/* Comment this to remove packet field output: */

#define DEBUG_OUTPUT

#ifndef NS_PORT
static LIST(rrcq_records);
static LIST(rrcq_blacklist);
static struct rrcq_record *rrcq_record_insert(struct in_addr orig_addr,       u_int32_t rreq_id,struct in_addr src_addr);
static struct rrcq_record *rrcq_record_find(struct in_addr orig_addr,    u_int32_t rreq_id,struct in_addr src_addr);

struct blacklist *rrcq_blacklist_find(struct in_addr dest_addr);

#endif

struct blacklist *NS_CLASS rrcq_blacklist_find(struct in_addr dest_addr)
{
    list_t *pos;

    list_foreach(pos, &rrcq_blacklist) {
        struct blacklist *bl = (struct blacklist *) pos;

        if (bl->dest_addr.s_addr == dest_addr.s_addr)
            return bl;
    }
    return NULL;
}

NS_STATIC struct rrcq_record *NS_CLASS rrcq_record_insert(struct in_addr
                                                          orig_addr,
                                                          u_int32_t rreq_id)
{
    struct rrcq_record *rec;

    rec = rrcq_record_find(orig_addr, rreq_id,src_addr);

    if (rec)
        return rec;

    if ((rec =
                 (struct rrcq_record *) malloc(sizeof(struct rrcq_record))) == NULL) {
        fprintf(stderr, "Malloc failed!!!\n");
        exit(-1);
    }
    rec->orig_addr = orig_addr;
    rec->rreq_id = rreq_id;
    rec->src_addr = src_addr;
    timer_init(&rec->rec_timer, &NS_CLASS rrcq_record_timeout, rec);

    list_add(&rrcq_records, &rec->l);

    DEBUG(LOG_INFO, 0, "Buffering RRCQ %s rreq_id=%lu time=%u",
          ip_to_str(orig_addr), rreq_id, PATH_DISCOVERY_TIME);

    timer_set_timeout(&rec->rec_timer, PATH_DISCOVERY_TIME);
    return rec;
}

NS_STATIC struct rrcq_record *NS_CLASS rrcq_record_find(struct in_addr
                                                        orig_addr,
                                                        u_int32_t rreq_id,struct in_addr src_addr)
{
    list_t *pos;

    list_foreach(pos, &rrcq_records) {
        struct rrcq_record *rec = (struct rrcq_record *) pos;
        if (rec->orig_addr.s_addr == orig_addr.s_addr &&
            (rec->rreq_id == rreq_id) &&(rec->src_addr.s_addr == src_addr.s_addr))
            return rec;
    }
    return NULL;
}


RRCQ *NS_CLASS rrcq_create(u_int8_t flags, int dest_addr, u_int32_t dest_seqno, int orig_addr)
{
    RRCQ *rrcq;

    rrcq = (RRCQ *) aodv_socket_new_msg();
    rrcq->type = AODV_RRCQ;
    rrcq->res1 = 0;
    rrcq->res2 = 0;
    rrcq->hcnt = 0;
    rrcq->rrcq_id = htonl(this_host.rreq_id++);

    rrcq->dest_seqno = htonl(dest_seqno);

    /* Immediately before a node originates a RREQ flood it must
       increment its sequence number... */
    seqno_incr(this_host.seqno);
    rrcq->orig_seqno = htonl(this_host.seqno);

    rrcq->dest_addr = dest_addr;
    rrcq->orig_addr = orig_addr;

    //////
    rrcq->Channel=0;
    rrcq->Cost=0;
    //////


    rrcq->j = 1;
    rrcq->r = 1;
    rrcq->g = 1;
    rrcq->d = 1;

    fprintf(stderr,"---------------This is the rrcq_create!--------------\n");

#ifdef DEBUG_OUTPUT
    log_pkt_fields((AODV_msg *) rrcq);
#endif

    return rrcq;
}
void NS_CLASS rrcq_send(struct in_addr dest_addr, u_int32_t dest_seqno, int ttl, u_int8_t flags)
{
    fprintf(stderr,"---------------This is the rrcq_send!--------------\n");
    RRCQ *rrcq;

    rt_table_t * rt, *rt_dest_addr;
    //u_int32_t dest_seqno;
    struct in_addr dest, orig_addr;
    int i;
    seek_list_t *seek_entry;

    rt_dest_addr = rt_table_find(dest_addr);   //找到路由表上的item
    //dest_seqno = rt_dest_addr->dest_seqno;

    dest.s_addr = AODV_BROADCAST;

    //find current ip
    for (i = 0; i < MAX_NR_INTERFACES; i++) {
        if (DEV_NR(i).enabled) {
            orig_addr = DEV_NR(i).ipaddr;
            break;
        }
    }
    rrcq = rrcq_create(flags, dest_addr.s_addr, dest_seqno, orig_addr.s_addr);



    for (i = 0; i < RT_TABLESIZE; i++) {
        list_t *pos;
        list_foreach(pos, &rt_tbl.tbl[i]) {    //refer to routing_table.c
            rt = (rt_table_t *) pos;

            if ( !seek_list_find(rt->dest_addr) &&
                 rt->next_hop.s_addr == rt_dest_addr->dest_addr.s_addr &&
                 rt->dest_addr.s_addr != rt_dest_addr->dest_addr.s_addr)


                rt->flags |= RT_REPAIR;  //repair
                rt_table_invalidate(rt);

                DEBUG(LOG_DEBUG, 0,
                      "  %s  REPAIR",
                      ip_to_str(rt->dest_addr));
                fprintf(stderr,
                  "  %s  REPAIR",
                  ip_to_str(rt->dest_addr));
                /* If the link that broke are marked for repair,
                   then do the same for all additional unreachable
                   destinations. */



            rrcq_add_udest(rrcq, rt->dest_addr,
                           rt->dest_seqno);

            }
        }
    fprintf(stderr,
            " rrcq_udest_count is %d\n",
            rrcq->dest_count);

    for (i = 0; i < MAX_NR_INTERFACES; i++) {
        if (!DEV_NR(i).enabled)
            continue;
        aodv_socket_send((AODV_msg *) rrcq, dest, RRCQ_CALC_SIZE(rrcq),
                         ttl, &DEV_NR(i));
    }
}

void NS_CLASS rrcq_forward(RRCQ * rrcq, int size, int ttl)
{
    struct in_addr dest, orig;
    int i;

    dest.s_addr = AODV_BROADCAST;
    orig.s_addr = rrcq->orig_addr;

    /* FORWARD the RRCQ if the TTL allows it. */
    DEBUG(LOG_INFO, 0, "forwarding RRCQ src=%s, rrcq_id=%lu",
          ip_to_str(orig), ntohl(rrcq->rrcq_id));
    fprintf(stderr, "forwarding RRCQ src=%s, rrcq_id=%lu\n",
          ip_to_str(orig), ntohl(rrcq->rrcq_id));
    /* Queue the received message in the send buffer */
    rrcq = (RRCQ *) aodv_socket_queue_msg((AODV_msg *) rrcq,
                                           (int)RRCQ_CALC_SIZE(rrcq));

    rrcq->hcnt++;		/* Increase hopcount to account for
				 * intermediate route */

    /* Send out on all interfaces */
    for (i = 0; i < MAX_NR_INTERFACES; i++) {
        if (!DEV_NR(i).enabled)
            continue;
        aodv_socket_send((AODV_msg *) rrcq, dest,
                         (int)RRCQ_CALC_SIZE(rrcq), ttl, &DEV_NR(i));
    }
}
void NS_CLASS rrcq_process(RRCQ * rrcq, int rrcqlen, struct in_addr ip_src, struct in_addr ip_dst, int ip_ttl, unsigned int ifindex)
{
    AODV_ext *ext;
    /*RRCP *rrcp = NULL;
    RRCP_udest *udest;*/
    int rrcq_size = RRCQ_CALC_SIZE(rrcq);
    rt_table_t *rev_rt = NULL, *fwd_rt = NULL;
    u_int32_t rrcq_orig_seqno, rrcq_dest_seqno;
    u_int32_t rrcq_id, rrcq_new_hcnt, life;
    //unsigned int extlen = 0;
    struct in_addr rrcq_dest, rrcq_orig;


    u_int32_t rrcq_Cost,rrcq_Channel;


    rrcq_dest.s_addr = rrcq->dest_addr;
    rrcq_orig.s_addr = rrcq->orig_addr;
    rrcq_id = ntohl(rrcq->rrcq_id);
    rrcq_dest_seqno = ntohl(rrcq->dest_seqno);
    rrcq_orig_seqno = ntohl(rrcq->orig_seqno);
    rrcq_new_hcnt = rrcq->hcnt + 1;

    rrcq_Cost = rrcq->Cost + Func_La(ip_src,DEV_IFINDEX(ifindex).ipaddr);
    rrcq_Channel = Func_Cha(ip_src,DEV_IFINDEX(ifindex).ipaddr);



    if (rrcq_orig.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr)
        return;
    DEBUG(LOG_DEBUG, 0, "ip_src=%s rrcq_orig=%s rrcq_dest=%s ttl=%d",
          ip_to_str(ip_src), ip_to_str(rrcq_orig), ip_to_str(rrcq_dest),
          ip_ttl);
    ///add dormouse
    fprintf(stderr, "ip_src=%s rrcq_orig=%s rrcq_dest=%s ttl=%d\n",
          ip_to_str(ip_src), ip_to_str(rrcq_orig), ip_to_str(rrcq_dest),
          ip_ttl);
    ///addend
    if (rrcqlen < (int)RRCQ_CALC_SIZE(rrcq)) {
        alog(LOG_WARNING, 0, __FUNCTION__,
             "IP data too short (%u bytes) from %s to %s. Should be %d bytes.",
             rrcqlen, ip_to_str(ip_src), ip_to_str(ip_dst),
             RRCQ_CALC_SIZE(rrcq));

        return;
    }

    if (rrcq_blacklist_find(ip_src)) {
        DEBUG(LOG_DEBUG, 0, "prev hop of RRCQ blacklisted, ignoring!");
        return;
    }


    /* Ignore already processed RREQs. */
    if (rrcq_record_find(rrcq_orig, rrcq_id,ip_src))
        return;

    /* Now buffer this RRCQ so that we don't process a similar RRCQ we
       get within PATH_DISCOVERY_TIME. */
    rrcq_record_insert(rrcq_orig, rrcq_id,ip_src);


    struct timeval now;
    gettimeofday(&now,NULL);

#ifdef DEBUG_OUTPUT
    log_pkt_fields((AODV_msg *) rrcq);
#endif


    /* The node always creates or updates a REVERSE ROUTE entry to the
       source of the RREQ. */
    rev_rt = rt_table_find(rrcq_orig);

    /* Calculate the extended minimal life time. */
    life = PATH_DISCOVERY_TIME - 2 * rrcq_new_hcnt * NODE_TRAVERSAL_TIME;


    if (rev_rt == NULL) {
        DEBUG(LOG_DEBUG, 0,
              "Creating REVERSE route entry, RRCQ orig: %s",
              ip_to_str(rrcq_orig));

        rev_rt = rt_table_insert(rrcq_orig, ip_src, rrcq_new_hcnt, rrcq_orig_seqno, life, INVALID, 0,
                                 ifindex,rrcq_Cost,rrcq_Channel);//here to increase
    } else {
        if (rev_rt->dest_seqno == 0 ||
            (int32_t) rrcq_orig_seqno > (int32_t) rev_rt->dest_seqno ||
            (rrcq_orig_seqno == rev_rt->dest_seqno &&
             (rev_rt->state == INVALID
              || rrcq_Cost < rev_rt->Cost))) {
            rev_rt =rt_table_update(rev_rt, ip_src, rrcq_new_hcnt,rrcq_orig_seqno, life, INVALID,rev_rt->flags,rrcq_Cost,rrcq_Channel);//here to increase calcount
        }
    }


    if (rrcq_dest.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr) {

        if (rrcq_dest_seqno != 0) {
            if ((int32_t) this_host.seqno < (int32_t) rrcq_dest_seqno)
                this_host.seqno = rrcq_dest_seqno;
            else if (this_host.seqno == rrcq_dest_seqno)
                seqno_incr(this_host.seqno);
        }
        fprintf(stderr,"terminal node has recieved the rrcq ,staring send rrcp ,rrcq_udest_count is %d!\n",rrcq->dest_count);
         RRCP* rrcp=rrcp_create(rrcq,0,MY_ROUTE_TIMEOUT,0,0,rev_rt->Channel);//DAIDING
         rrcp_send(rrcp,rev_rt,NULL,RRCP_CALC_SIZE(rrcp));
//!!!!!!

    }
    else
    {
        if (ip_ttl > 1) {
            rrcq->Cost = rrcq_Cost;
            rrcq->Channel = rrcq_Channel;

            rrcq_forward(rrcq,rrcqlen ,--ip_ttl);
        }
    }
}


void NS_CLASS rrcq_route_discovery(struct in_addr dest_addr, u_int8_t flags, struct ip_data *ipd)
{
    return ;
}

void NS_CLASS rrcq_record_timeout(void *arg)
{
    fprintf(stderr,"---------------This is the rrcq_send!--------------\n");
    struct rrcq_record *rec = (struct rrcq_record *) arg;

    list_detach(&rec->l);
    free(rec);
}

void NS_CLASS rrcq_blacklist_timeout(void *arg)
{
    struct blacklist *bl = (struct blacklist *) arg;

    list_detach(&bl->l);
    free(bl);
}
void NS_CLASS rrcq_local_repair(rt_table_t * rt, struct in_addr src_addr, struct ip_data *ipd)
{
    struct timeval now;
    seek_list_t *seek_entry;
    rt_table_t *src_entry;
    int ttl;
    u_int8_t flags = 0;

    if (!rt)
        return;

    if (seek_list_find(rt->dest_addr))
        return;

    if (!(rt->flags & RT_REPAIR))
        return;

    gettimeofday(&now, NULL);

    ///by dormouse  a
    fprintf(stderr, "REPAIRING route to %s\n", ip_to_str(rt->next_hop));

    ///byend


    DEBUG(LOG_DEBUG, 0, "REPAIRING route to %s", ip_to_str(rt->next_hop));

    /* Caclulate the initial ttl to use for the RREQ. MIN_REPAIR_TTL
       mentioned in the draft is the last known hop count to the
       destination. */

    src_entry = rt_table_find(src_addr);

    if (src_entry)
        ttl = (int) (Max(rt->hcnt, 0.5 * src_entry->hcnt) + LOCAL_ADD_TTL);
    else
        ttl = rt->hcnt + LOCAL_ADD_TTL;

    DEBUG(LOG_DEBUG, 0, "%s, rrcq ttl=%d, dest_hcnt=%d",
          ip_to_str(rt->dest_addr), ttl, rt->hcnt);

    /* Reset the timeout handler, was probably previously
       local_repair_timeout */
    rt->rt_timer.handler = &NS_CLASS route_expire_timeout;

    if (timeval_diff(&rt->rt_timer.timeout, &now) < (2 * NET_TRAVERSAL_TIME))
        rt_table_update_timeout(rt, 2 * NET_TRAVERSAL_TIME);


    rrcq_send(rt->next_hop, rt->dest_seqno, ttl, flags);
    //to find a local way  :by dormouse

    /* Remember that we are seeking this destination and setup the
       timers */
    seek_entry = seek_list_insert(rt->dest_addr, rt->dest_seqno,
                                  ttl, flags, ipd);

    if (expanding_ring_search)
        timer_set_timeout(&seek_entry->seek_timer,
                          2 * ttl * NODE_TRAVERSAL_TIME);
    else
        timer_set_timeout(&seek_entry->seek_timer, NET_TRAVERSAL_TIME);

    DEBUG(LOG_DEBUG, 0, "Seeking_a %s ttl=%d", ip_to_str(rt->dest_addr), ttl);

    return;
}


