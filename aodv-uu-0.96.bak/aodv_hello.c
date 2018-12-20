/*****************************************************************************
 *
 * Copyright (C) 2001 Uppsala University and Ericsson AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Erik Nordstr�m, <erik.nordstrom@it.uu.se>
 *          
 *
 *****************************************************************************/

#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else

#include <netinet/in.h>
#include "ns-2/armamain.h"        //added by gaoruiyuan
#include "aodv_hello.h"
#include "aodv_hello_ack.h"        //added by gaoruiyuan
#include "aodv_timeout.h"
#include "aodv_rrep.h"
#include "aodv_rreq.h"
#include "routing_table.h"
#include "timer_queue.h"
#include "params.h"
#include "aodv_socket.h"
#include "defs.h"
#include "debug.h"
#include "ns-2/aodv-uu.h"
#include <math.h>

extern int unidir_hack, receive_n_hellos, hello_jittering, optimized_hellos;
static struct timer hello_timer;

/*added by gaoruiyuan*/
#define INT_MAX 2147483647
#define ALPHA   1
#define BETA    1
static struct timer cost_timer;
/*end added*/
#endif

/* #define DEBUG_HELLO */
/*function added by gaoruiyuan*/
void NS_CLASS cal_cost(void *arg) {
    //fprintf(stderr, "%s start to cal cost()\n", ip_to_str(DEV_NR(NS_DEV_NR).ipaddr));
    hello_sended_t *hs;
    hello_received_t *hr;
    HELLO *hello_msg;
    HELLO_ACK *ack_msg;
    stability_t *stb_item;
    struct timeval *msg_time, now;
    long timediff;
    float prob, arma_predict;
    int *array, len, *temp;
    int flag;
    int i, k_status, cur_status, rt_i;
    int N_hello, R_ack;
    float ETT, BandWidth;

    gettimeofday(&now, NULL);
    N_hello = 0;
    list_t *item;
    list_foreach(item, &nb_tbl.hello_send_list) {
        hs = (hello_sended_t *) item;
        msg_time = &hs->time;
        timediff = timeval_diff(&now, msg_time);
        //fprintf(stderr, "N_hello=%d, timediff=%d\n", N_hello, timediff);
        if (timediff > COST_INTERVAL) { break; }
        N_hello++;
//                hello_msg = &hs->msg;
//                if (hello_msg->channel == channel) { N_hello++; }
    }
    if (N_hello == 0) {
        fprintf(stderr, "calling cal too early, no hello sent\n");
        return;
    }
    //fprintf(stderr, "nb->nhello_send=%d\n", nb_tbl.nhello_send);
    //TODO how to get bandwidth?
    BandWidth = maclist[NS_DEV_NR]->getBandWidth();
    for (rt_i = 0; rt_i < RT_TABLESIZE; rt_i++) {
        list_t *pos;
        list_t *head = &(nb_tbl.tbl[rt_i]);
//        fprintf(stderr, "%s cal loss for %d, &nb_tbl.tbl[rt_i]=%d, Bandwidth=%d\n",
//                ip_to_str(DEV_NR(NS_DEV_NR).ipaddr),
//                rt_i, &(nb_tbl.tbl[rt_i]), BandWidth);
        for (pos = head->next; pos != head; pos = pos->next){
        //list_foreach(pos, &nb_tbl.tbl[i]) {
            nb_table_t *nb = (nb_table_t *) pos;
            fprintf(stderr, "%s cal loss for %s, channel=%d, Bandwidth=%f\n",
                    ip_to_str(DEV_NR(NS_DEV_NR).ipaddr),
                    ip_to_str(nb->neighbor_addr),
                    nb->channel, BandWidth);
            if (nb->state != INVALID) {
                /*Step 1 ETT*/
                list_t *item;
                //fprintf(stderr, "nb->nhello_ack=%d\n", nb->nhello_ack);
                R_ack = 0;
                list_foreach(item, &nb->hello_ack_list) {
                    hr = (hello_received_t *) item;
                    msg_time = &hr->time;
                    timediff = timeval_diff(&now, msg_time);
                    //fprintf(stderr, "R_ack=%d, timediff=%d\n", R_ack, timediff);
                    if (timediff > COST_INTERVAL) { break; }
                    R_ack++;
//                ack_msg = &hr->msg;
//                if (ack_msg->channel == channel) { R_ack++; }
                }
                assert(N_hello >= R_ack);

                ETT = ((float)N_hello*(HELLO_SIZE+HELLO_ACK_SIZE)*8/ (float)((float)R_ack * BandWidth));
                fprintf(stderr, "R_ack=%d, N_hello=%d ", R_ack, N_hello);
                fprintf(stderr, "ETT=%f ", ETT);

                /*Step 2 ARMA*/
                array = nb->data_link->lifetimes;
                len = nb->data_link->lifetime_len;
                //fprintf(stderr, "arma array len=%d\n", len);
                if(len > 3){
                    arma_predict = armamain(array, len);
                }else{
                    arma_predict = 0.0001;
                }
                if(arma_predict <= 0){
                    arma_predict = 0.0001;
                }
                fprintf(stderr, "arma_predict=%f ", arma_predict);

                /*Step 3 Marcov*/
                flag = 0;
                cur_status = 0;
                //temp = nb->all_status;
                for(i = 0; i < 1 << Marcov_K; i++){
                    nb->all_status[i] = 0;
                }
                list_foreach(item, &nb->data_link->stability) {
                    //fprintf(stderr, "OK1");
                    stb_item = (stability_t *) item;
                    stability_t *iter_item;
                    //fprintf(stderr, "%d", stb_item->status);
                    k_status = 0;
                    k_status |= stb_item->status;
                    iter_item = (stability_t *) stb_item->l.next;
                    for (i = 1; i < Marcov_K; i++) {
                        //fprintf(stderr, "%d", iter_item->status);
                        k_status |= iter_item->status << i;
                        if (stb_item->l.next == NULL) { break; }
                        iter_item = (stability_t *) stb_item->l.next;
                    }
                    //fprintf(stderr, " k_status=%d\n", k_status);
                    nb->all_status[k_status]++;
                    if (!flag) { cur_status = k_status; }
                    flag++;
                }
                //fprintf(stderr, "\n");
                if(flag){
                    prob = float(nb->all_status[cur_status]) / float(flag);   //TODO current is a float, need to transfer to int for use
                    fprintf(stderr, "Marcov_predict=%f ", prob);
                }else{
                    prob = 0.99;
                }
                /*step 4. calculate cost*/
                nb->cost = 1000 * (ALPHA * ETT + BETA / (arma_predict * (1.0 - prob)));
                fprintf(stderr, "nb->cost=%d\n", nb->cost);
            }
            else {
                nb->cost = INT_MAX;
            }
            //TODO ALPHA and BETA is defined, right or not?
        }
    }
    timer_set_timeout(&cost_timer, COST_INTERVAL);
    fprintf(stderr, "cal done\n");
}


long NS_CLASS hello_jitter() {
    if (hello_jittering) {
#ifdef NS_PORT
        return (long) (((float) Random::integer(RAND_MAX + 1) / RAND_MAX - 0.5)
                   * JITTER_INTERVAL);
#else
        return (long) (((float) random() / RAND_MAX - 0.5) * JITTER_INTERVAL);
#endif
    } else
        return 0;
}

void NS_CLASS hello_start() {
    fprintf(stderr, "hello_start()\n");
    if (hello_timer.used)
        return;

    gettimeofday(&this_host.fwd_time, NULL);

    DEBUG(LOG_DEBUG, 0, "Starting to send HELLOs!");
    timer_init(&hello_timer, &NS_CLASS hello_send, NULL);
    timer_init(&cost_timer, &NS_CLASS cal_cost, NULL);      //added by gaoruiyuan
    timer_set_timeout(&cost_timer, COST_INTERVAL);          //added by gaoruiyuan
    //Step 2. start to send Hello
    hello_send(NULL);
}

void NS_CLASS hello_stop() {
    DEBUG(LOG_DEBUG, 0,
          "No active forwarding routes - stopped sending HELLOs!");
    timer_remove(&hello_timer);
    timer_remove(&cost_timer);            // added by gaoruiyuan
}

/*hello_create function added by gaoruiyuan*/
HELLO *NS_CLASS hello_create(u_int8_t hcnt,
                             struct in_addr dest_addr,
                             u_int32_t dest_seqno,
                             struct in_addr orig_addr) {
    HELLO *hello;

    hello = (HELLO *) aodv_socket_new_msg();
    hello->type = AODV_HELLO;
    hello->res2 = 0;
    hello->hcnt = hcnt;
    hello->orig_addr = orig_addr.s_addr;
    hello->dest_addr = dest_addr.s_addr;
    hello->dest_seqno = dest_seqno;

    /* Don't print information about hello messages... */
#ifdef DEBUG_OUTPUT
    if (hello->dest_addr != hello->orig_addr) {
    DEBUG(LOG_DEBUG, 0, "Assembled HELLO:");
    log_pkt_fields((AODV_msg *) hello);
    }
#endif

    return hello;
}

void NS_CLASS hello_send(void *arg) {
    //RREP *rrep;				//changed by gaoruiyuan
    HELLO *hello;             //changed by gaoruiyuan
    hello_sended_t *hs;         //added by gaoruiyuan
    AODV_ext *ext = NULL;
    u_int8_t flags = 0;
    struct in_addr dest;
    long time_diff, jitter;
    struct timeval now;
    int msg_size = HELLO_SIZE;
    int i;

    gettimeofday(&now, NULL);

    if (optimized_hellos &&
        timeval_diff(&now, &this_host.fwd_time) > ACTIVE_ROUTE_TIMEOUT) {
        hello_stop();
        return;
    }

    time_diff = timeval_diff(&now, &this_host.bcast_time);
    jitter = hello_jitter();

    /* This check will ensure we don't send unnecessary hello msgs, in case
       we have sent other bcast msgs within HELLO_INTERVAL */
    if (time_diff >= HELLO_INTERVAL) {
        for (i = 0; i < MAX_NR_INTERFACES; i++) {
            if (!DEV_NR(i).enabled)
                continue;
#ifdef DEBUG_HELLO
            DEBUG(LOG_DEBUG, 0, "sending Hello to 255.255.255.255");
#endif
            hello = hello_create(0, DEV_NR(i).ipaddr, this_host.seqno, DEV_NR(i).ipaddr);    // gaoruiyuan changed

            /* Assemble a RREP extension which contain our neighbor set... */
            if (unidir_hack) {
                int i;

                if (ext)
                    ext = AODV_EXT_NEXT(ext);
                else
                    ext = (AODV_ext *) ((char *) hello + HELLO_SIZE);   //changed by gaoruiyuan

                ext->type = RREP_HELLO_NEIGHBOR_SET_EXT;
                ext->length = 0;

                for (i = 0; i < RT_TABLESIZE; i++) {
                    list_t *pos;
                    list_foreach(pos, &rt_tbl.tbl[i]) {
                        rt_table_t *rt = (rt_table_t *) pos;
                        /* If an entry has an active hello timer, we assume
                           that we are receiving hello messages from that
                           node... */
                        if (rt->hello_timer.used) {
#ifdef DEBUG_HELLO
                            DEBUG(LOG_INFO, 0,
                              "Adding %s to hello neighbor set ext",
                              ip_to_str(rt->dest_addr));
#endif
                            memcpy(AODV_EXT_DATA(ext), &rt->dest_addr,
                                   sizeof(struct in_addr));
                            ext->length += sizeof(struct in_addr);
                        }
                    }
                }
                if (ext->length)
                    msg_size = HELLO_SIZE + AODV_EXT_SIZE(ext);     //changed by gaoruiyuan
            }
            /*added by gaoruiyuan*/
            fprintf(stderr, "add to list for num:%d\n", nb_tbl.nhello_send);
            if ((hs = (hello_sended_t *) malloc(sizeof(hello_sended_t))) == NULL) {
                perror("Could not allocate memory for hello_sended node!!\n");
                exit(-1);
            }
            hs->time.tv_sec = now.tv_sec;
            hs->time.tv_usec = now.tv_usec;
            list_add(&nb_tbl.hello_send_list, &hs->l);      //added in head, ensure latest in the front
            nb_tbl.nhello_send++;
            hello->channel = 0;
            /*added end*/
            dest.s_addr = AODV_BROADCAST;   // 255.255.255.255,
            fprintf(stderr, "sending hello to:%s\n", ip_to_str(dest));
            /*changed by gaoruiyuan*/
            aodv_socket_send((AODV_msg *) hello, dest, msg_size, 1, &DEV_NR(i));
            /*end changed*/
        }
        //timer_set_timeout(&hello_timer, HELLO_INTERVAL + jitter);
        timer_set_timeout(&hello_timer, HELLO_INTERVAL);
    } else {
        if (HELLO_INTERVAL - time_diff + jitter < 0)
            timer_set_timeout(&hello_timer,
                              HELLO_INTERVAL - time_diff - jitter);
        else
            timer_set_timeout(&hello_timer,
                              HELLO_INTERVAL - time_diff + jitter);
    }
}


/* Process a hello message changed by gaoruiyuan*/
void NS_CLASS hello_process(HELLO *hello, int hellolen, unsigned int ifindex) {
    u_int32_t hello_seqno, timeout, hello_interval = HELLO_INTERVAL;
    u_int8_t state, flags = 0;
    struct in_addr ext_neighbor, hello_dest, hello_orig;
    rt_table_t *rt;
    nb_table_t *nb;
    AODV_ext *ext = NULL;
    int i;
    struct timeval now;

    //fprintf(stderr, "in function hello_process\n");

    gettimeofday(&now, NULL);

    hello_dest.s_addr = hello->dest_addr;
    hello_orig.s_addr = hello->orig_addr;
    fprintf(stderr,"%s receive a hello msg with dst:%s, orig:%s\n",
            ip_to_str(DEV_NR(NS_DEV_NR).ipaddr),
            ip_to_str(hello_dest), ip_to_str(hello_orig));
    hello_seqno = ntohl(hello->dest_seqno);

    rt = rt_table_find(hello_orig);

    if (rt)
        flags = rt->flags;

    if (unidir_hack)
        flags |= RT_UNIDIR;

    /* Check for hello interval extension: */
    ext = (AODV_ext *) ((char *) hello + HELLO_SIZE);

    while (hellolen > (int) HELLO_SIZE) {
        switch (ext->type) {
            case RREP_HELLO_INTERVAL_EXT:
                if (ext->length == 4) {
                    memcpy(&hello_interval, AODV_EXT_DATA(ext), 4);
                    hello_interval = ntohl(hello_interval);
#ifdef DEBUG_HELLO
                    DEBUG(LOG_INFO, 0, "Hello extension interval=%lu!",
                          hello_interval);
#endif

                } else
                    alog(LOG_WARNING, 0,
                         __FUNCTION__, "Bad hello interval extension!");
                break;
            case RREP_HELLO_NEIGHBOR_SET_EXT:

#ifdef DEBUG_HELLO
                DEBUG(LOG_INFO, 0, "RREP_HELLO_NEIGHBOR_SET_EXT");
#endif
                for (i = 0; i < ext->length; i = i + 4) {
                    ext_neighbor.s_addr =
                            *(in_addr_t * )((char *) AODV_EXT_DATA(ext) + i);

                    if (ext_neighbor.s_addr == DEV_IFINDEX(ifindex).ipaddr.s_addr)
                        flags &= ~RT_UNIDIR;
                }
                break;
            default:
                alog(LOG_WARNING, 0, __FUNCTION__,
                     "Bad extension!! type=%d, length=%d", ext->type, ext->length);
                ext = NULL;
                break;
        }
        if (ext == NULL)
            break;

        hellolen -= AODV_EXT_SIZE(ext);
        ext = AODV_EXT_NEXT(ext);
    }

#ifdef DEBUG_HELLO
    DEBUG(LOG_DEBUG, 0, "rcvd HELLO from %s, seqno %lu",
      ip_to_str(hello_dest), hello_seqno);
#endif
    /* This neighbor should only be valid after receiving 3
       consecutive hello messages... */
    //if (receive_n_hellos)
    //    state = INVALID;
    //else
    state = VALID;

    timeout = ALLOWED_HELLO_LOSS * hello_interval + ROUTE_TIMEOUT_SLACK;

    if (!rt) {
        /* No active or expired route in the routing table. So we add a
           new entry... */

        rt = rt_table_insert(hello_orig, hello_orig, 1,
                             hello_seqno, timeout, state, flags, ifindex);
        //gaoruiyuan changed from dest to orig

        if (flags & RT_UNIDIR) {
            DEBUG(LOG_INFO, 0, "%s new NEIGHBOR, link UNI-DIR",
                  ip_to_str(rt->dest_addr));
        } else {
            DEBUG(LOG_INFO, 0, "%s new NEIGHBOR!", ip_to_str(rt->dest_addr));
        }
        rt->hello_cnt = 1;

    } else {
        /*
        if ((flags & RT_UNIDIR) && rt->state == VALID && rt->hcnt > 1) {
            goto hello_update;
        }

        if (receive_n_hellos && rt->hello_cnt < (receive_n_hellos - 1)) {
            if (timeval_diff(&now, &rt->last_hello_time) <
            (long) (hello_interval + hello_interval / 2))
            rt->hello_cnt++;
            else
            rt->hello_cnt = 1;

            memcpy(&rt->last_hello_time, &now, sizeof(struct timeval));
            return;
        }
        */
        rt_table_update(rt, hello_orig, 1, hello_seqno, timeout, VALID, flags);
        //gaoruiyuan changed from dest to orig
    }
    /*added by gaoruiyuan*/
    fprintf(stderr, "%s handling nb for %s, channel=%d, should add for first\n",
            ip_to_str(DEV_NR(NS_DEV_NR).ipaddr),
            ip_to_str(hello_orig), hello->channel);
    nb = nb_table_find(hello_orig, hello->channel, true);
    //fprintf(stderr, "got one, start validating\n");
    if(nb->state == INVALID){
        nb_table_validate(nb);
    }
    //fprintf(stderr, "validated done, start send back\n");
    nb->nb_timer.used = 1;      // set as used to make sure this timer will be update but not copy
    timer_set_timeout(&(nb->nb_timer), HELLO_INTERVAL*3);
    hello_ack_send(ifindex, hello_orig, hello->channel);
    //fprintf(stderr, "send back done\n");
    /*end added by gaoruiyuan*/

    //hello_update_timeout(rt, &now, ALLOWED_HELLO_LOSS * hello_interval);
    return;
}


#define HELLO_DELAY 50        /* The extra time we should allow an hello
				   message to take (due to processing) before
				   assuming lost . */

//NS_INLINE void NS_CLASS hello_update_timeout(rt_table_t * rt,
//					     struct timeval *now, long time)
//{
//    timer_set_timeout(&rt->hello_timer, time + HELLO_DELAY);
//    memcpy(&rt->last_hello_time, now, sizeof(struct timeval));
//}
