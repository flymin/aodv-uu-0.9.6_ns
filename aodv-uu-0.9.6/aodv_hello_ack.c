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
 * Authors: Erik Nordström, <erik.nordstrom@it.uu.se>
 *          
 *
 *****************************************************************************/

#ifdef NS_PORT
#include "ns-2/aodv-uu.h"
#else

#include <netinet/in.h>
#include "aodv_timeout.h"
#include "aodv_rrep.h"
#include "aodv_rreq.h"
#include "routing_table.h"
#include "timer_queue.h"
#include "params.h"
#include "aodv_socket.h"
#include "defs.h"
#include "debug.h"

extern int unidir_hack;

#endif

#include "aodv_hello_ack.h"         //added by gaoruiyuan

//#define DEBUG_HELLO


//long NS_CLASS hello_jitter() {
//    if (hello_jittering) {  //jittering ????,?????????????????????
//#ifdef NS_PORT
//        return (long) (((float) Random::integer(RAND_MAX + 1) / RAND_MAX - 0.5)
//                   * JITTER_INTERVAL);
//#else
//        return (long) (((float) random() / RAND_MAX - 0.5) * JITTER_INTERVAL);
//#endif
//    } else
//        return 0;
//}

//void NS_CLASS hello_start() {
//	fprintf(stderr, "hello_start()");
//    if (hello_timer.used)       //?????????????,???????hello
//        return;
//    //Step 1. ???????????
//    gettimeofday(&this_host.fwd_time, NULL);
//
//    DEBUG(LOG_DEBUG, 0, "Starting to send HELLOs!");
//    timer_init(&hello_timer, &NS_CLASS hello_send, NULL);
//    //Step 2. ??Hello
//    hello_send(NULL);
//}

//void NS_CLASS hello_stop() {
//    DEBUG(LOG_DEBUG, 0,
//          "No active forwarding routes - stopped sending HELLOs!");
//    timer_remove(&hello_timer);
//}

/*hello_create function added by gaoruiyuan*/
HELLO_ACK *NS_CLASS hello_ack_create(struct in_addr orig_addr, struct in_addr dest_addr,
                                int channel) {
    HELLO_ACK *hello_ack;

    hello_ack = (HELLO_ACK *) aodv_socket_new_msg();
    hello_ack->type = AODV_HELLO_ACK;
    hello_ack->res2 = 0;
    hello_ack->hcnt = 0;
    hello_ack->orig_addr = orig_addr.s_addr;
    hello_ack->dest_addr = dest_addr.s_addr;
    hello_ack->channel = channel;

    /* Don't print information about hello messages... */
#ifdef DEBUG_OUTPUT
    if (hello_ack->dest_addr != hello_ack->orig_addr) {
    DEBUG(LOG_DEBUG, 0, "Assembled HELLO:");
    log_pkt_fields((AODV_msg *) hello_ack);
    }
#endif

    return hello_ack;
}

void NS_CLASS hello_ack_send(int receive_dev, struct in_addr sendto, int channel) {
    HELLO_ACK *hello_ack;             //changed by gaoruiyuan
    int msg_size = HELLO_ACK_SIZE;  //changed by gaoruiyuan
//    fprintf(stderr, "send hello_ack from %s to %s\n",
//            ip_to_str(DEV_IFINDEX(receive_dev).ipaddr),
//            ip_to_str(sendto));
    hello_ack = hello_ack_create(DEV_IFINDEX(receive_dev).ipaddr, sendto, channel);
    aodv_socket_send((AODV_msg *) hello_ack, sendto, msg_size, 1, &DEV_IFINDEX(receive_dev));
}


/* Process a hello message changed by gaoruiyuan*/
void NS_CLASS hello_ack_process(HELLO_ACK *hello_ack, int rreplen, unsigned int ifindex) {
    hello_received_t *hs;         //added by gaoruiyuan
    struct in_addr hello_orig;
    struct in_addr hello_dest;
    nb_table_t *nb;
    struct timeval now;
    //fprintf(stderr, "in function hello_ack_process\n");
    gettimeofday(&now, NULL);

    hello_orig.s_addr = hello_ack->orig_addr;
    fprintf(stderr,"%s reveive a hello_ack msg with orig:%s channel=%d\n",
            ip_to_str(DEV_NR(NS_DEV_NR).ipaddr),
            ip_to_str(hello_orig), hello_ack->channel);

    nb = nb_table_find(hello_orig, hello_ack->channel, false);

#ifdef DEBUG_HELLO
    DEBUG(LOG_DEBUG, 0, "rcvd HELLO from %s, seqno %lu",
      ip_to_str(hello_dest), hello_seqno);
#endif
    /* This neighbor should only be valid after receiving 3
       consecutive hello messages...
    //if (receive_n_hellos)
    //    state = INVALID;
    //else
    */

    if (!nb) {
        hello_dest.s_addr = hello_ack->dest_addr;
        fprintf(stderr,"receive a hello_ack msg for %s but cannot find nb, ignore\n",
                ip_to_str(hello_dest));
        return;
    }
    else {
        if ((hs = (hello_received_t *) malloc(sizeof(hello_received_t))) == NULL) {
            perror("Could not allocate memory for hello_received node!!\n");
            exit(-1);
        }
        hs->time.tv_sec = now.tv_sec;
        hs->time.tv_usec = now.tv_usec;
        list_add(&nb->hello_ack_list, &hs->l);      //added in head, ensure latest in the front
        nb->nhello_ack++;
        if(nb->state == INVALID){
            nb_table_validate(nb);
        }
        //timer_set_timeout(&(nb->nb_timer), ACTIVE_ROUTE_TIMEOUT);
    }

    //hello_update_timeout(rt, &now, ALLOWED_HELLO_LOSS * hello_interval);
    return;
}


#define HELLO_DELAY 50        /* The extra time we should allow an hello
				   message to take (due to processing) before
				   assuming lost . */

//NS_INLINE void NS_CLASS hello_update_timeout(rt_table_t *rt,
//                                             struct timeval *now, long time) {
//    timer_set_timeout(&rt->hello_timer, time + HELLO_DELAY);
//    memcpy(&rt->last_hello_time, now, sizeof(struct timeval));
//}
