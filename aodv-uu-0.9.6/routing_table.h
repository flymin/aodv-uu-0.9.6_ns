/*****************************************************************************
 *
 * Copyright (C) 2001 Uppsala University & Ericsson AB.
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
#ifndef _ROUTING_TABLE_H
#define _ROUTING_TABLE_H

#ifndef NS_NO_GLOBALS
#include "defs.h"
#include "list.h"

typedef struct rt_table rt_table_t;

/* Neighbor struct for active routes in Route Table */
typedef struct precursor {
    list_t l;
    struct in_addr neighbor;
} precursor_t;

#define FIRST_PREC(h) ((precursor_t *)((h).next))

#define seqno_incr(s) ((s == 0) ? 0 : ((s == 0xFFFFFFFF) ? s = 1 : s++))

typedef u_int32_t hash_value;   /* A hash value */

/* added by gaoruiyuan */
#define ARMA_PRE_MAXLEN 200

typedef struct nb_table nb_table_t;

typedef enum {
    stable = 1, unstable = 0
}stability;

typedef struct{
    list_t l;
    stability status;
}stability_t;

typedef struct{
    list_t l;
    struct in_addr dest_addr;
    int lifetime_len;
    int lifetimes[ARMA_PRE_MAXLEN];
    int channel;
    list_t stability;
}stable_data_t;

/* Route table entries */
struct nb_table{
	list_t l;
	struct in_addr neighbor_addr;	/* IP address of the destination */
	unsigned int channel;	/* Network interface index... */
	hash_value hash;
	u_int8_t state;		/* The state of this entry */
	struct timer nb_timer;	/* The timer associated with this entry */

	int nhello_ack;         //gaoruiyuan added
	list_t hello_ack_list;  /*gaoruiyuan add, to record hello ack received*/
	stable_data_t *data_link; //gaoruiyuan added pointing at data
	int cost;                   //gaoruiyuan added
	struct timeval setup;       //gaoruiyuan added
    int all_status[1 << Marcov_K];
};

/* added end */

/* Route table entries */
struct rt_table {
    list_t l;
    struct in_addr dest_addr;	/* IP address of the destination */
    u_int32_t dest_seqno;
    unsigned int ifindex;	/* Network interface index... */
    struct in_addr next_hop;	/* IP address of the next hop to the dest */
    u_int8_t hcnt;		/* Distance (in hops) to the destination */
    u_int16_t flags;		/* Routing flags */
    u_int8_t state;		/* The state of this entry */
    struct timer rt_timer;	/* The timer associated with this entry */
    struct timer ack_timer;	/* RREP_ack timer for this destination */
    struct timer hello_timer;
    struct timeval last_hello_time;
    u_int8_t hello_cnt;
    hash_value hash;
    int nprec;			/* Number of precursors */
    list_t precursors;		/* List of neighbors using the route */

    /********** Modified by Hao Hao **********/
	u_int32_t LA;
    u_int8_t Channel;
    /*****************************************/
};


/* Route entry flags */
#define RT_UNIDIR        0x1
#define RT_REPAIR        0x2
#define RT_INV_SEQNO     0x4
#define RT_INET_DEST     0x8	/* Mark for Internet destinations (to be relayed
				 * through a Internet gateway. */
#define RT_GATEWAY       0x10

/* Route entry states */
#define INVALID   0
#define VALID     1


#define RT_TABLESIZE 64		/* Must be a power of 2 */
#define RT_TABLEMASK (RT_TABLESIZE - 1)

struct routing_table {
    unsigned int num_entries;
    unsigned int num_active;
    list_t tbl[RT_TABLESIZE];
};

/* added by gaoruiyuan */
struct neighbor_table {
    unsigned int num_entries;
    int nhello_send;        //gaoruiyuan added
    list_t hello_send_list; /*gaoruiyuan add, to record hello msg sended*/
    list_t tbl[RT_TABLESIZE];
    list_t message[RT_TABLESIZE];
};


/* end adding */

void precursor_list_destroy(rt_table_t * rt);
#endif				/* NS_NO_GLOBALS */

#ifndef NS_NO_DECLARATIONS

struct routing_table rt_tbl;

//function added by gaoruiyuan
struct neighbor_table nb_tbl;
void nb_table_init();
stable_data_t *find_data_link(struct in_addr *addr, int channel, int index);
void nb_table_invalidate(void *arg);
int nb_table_validate(nb_table_t *nb);
nb_table_t *nb_table_find(struct in_addr dest_addr, unsigned int channel, bool create);
// end adding


void rt_table_init();
void rt_table_destroy();
/********** Modified by Hao Hao **********/
rt_table_t *rt_table_insert(struct in_addr dest, struct in_addr next,
			    u_int8_t hops, u_int32_t seqno, u_int32_t life,
			    u_int8_t state, u_int16_t flags,
			    unsigned int ifindex,
                u_int8_t channel, u_int32_t la);
rt_table_t *rt_table_update(rt_table_t * rt, struct in_addr next, u_int8_t hops,
			    u_int32_t seqno, u_int32_t lifetime, u_int8_t state,
			    u_int16_t flags,
                u_int8_t channel, u_int32_t la);
/*****************************************/
NS_INLINE rt_table_t *rt_table_update_timeout(rt_table_t * rt,
					      u_int32_t lifetime);
void rt_table_update_route_timeouts(rt_table_t * fwd_rt, rt_table_t * rev_rt);
rt_table_t *rt_table_find(struct in_addr dest);
rt_table_t *rt_table_find_gateway();
int rt_table_update_inet_rt(rt_table_t * gw, u_int32_t life);
int rt_table_invalidate(rt_table_t * rt);
void rt_table_delete(rt_table_t * rt);
void precursor_add(rt_table_t * rt, struct in_addr addr);
void precursor_remove(rt_table_t * rt, struct in_addr addr);

#endif				/* NS_NO_DECLARATIONS */

#endif				/* ROUTING_TABLE_H */
