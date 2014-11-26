/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "dev/serial-line.h"
#if CONTIKI_TARGET_Z1
#include "dev/uart0.h"
#else
#include "dev/uart1.h"
#endif
#include "bcp-collect-common.h"
#include "collect-view.h"
#include "list.h"
#include "bcp.h" 
#include "packetstack.h" 

#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8775
#define UDP_SERVER_PORT 5688
//#define UDP_SERVER1_PORT 6553
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

extern unsigned short node_id;
static struct uip_udp_conn *server_conn;
extern list_t mylist_list;
static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;
uip_ipaddr_t addr;




/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");   // we are creating a process over here
AUTOSTART_PROCESSES(&udp_client_process, &collect_common_process, &test_process); //we are starting the udp client process and the collect common process.
/*---------------------------------------------------------------------------*/
void
bcp_collect_common_set_sink(void)
{
  /* A udp client can never become sink */
}
/*---------------------------------------------------------------------------*/

void
bcp_collect_common_net_print(void)
{
  //rpl_dag_t *dag;
  //uip_ds6_route_t *r;

  /* Let's suppose we have only one instance */
  //dag = rpl_get_any_dag();
  /*if(dag->preferred_parent != NULL) {   //insert some code about getting the parent inforamtion
    PRINTF("Preferred parent: ");
    PRINT6ADDR(bcp_get_parent_ipaddr(dag->preferred_parent));   //write a function about getting the ipaddress of the parent
    PRINTF("\n");
  }
  for(r = uip_ds6_route_head();
      r != NULL;
      r = uip_ds6_route_next(r)) {
    PRINT6ADDR(&r->ipaddr);
  }
  PRINTF("---\n");*/
}
/*---------------------------------------------------------------------------*/
static void
bcp_tcpip_handler(void)
{
  uint8_t *appdata;
  rimeaddr_t sender;
  uint8_t seqno;
  uint8_t hops;
  struct collect_view_data_msg *msg;
  struct collect_view_data_msg msg1;

  hdr_information_t hdr_info; 


  if(uip_newdata()) {
    appdata = (uint8_t *)uip_appdata;
    sender.u8[0] = UIP_IP_BUF->srcipaddr.u8[15];
    sender.u8[1] = UIP_IP_BUF->srcipaddr.u8[14];
    seqno = *appdata;
    hops = uip_ds6_if.cur_hop_limit - UIP_IP_BUF->ttl + 1;
    
    hdr_info.seqno = seqno;
    hdr_info.hops = UIP_IP_BUF->ttl - 1;  
    hdr_info.sender.u8[0] = sender.u8[0];
    hdr_info.sender.u8[1] = sender.u8[1];

     uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, node_id-1);

  switch(node_id){
    case 2: 
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7401,0x0001,0x0101);
      break;
    case 3: 
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7402,0x0002,0x0202);
      break;
    case 4:
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7403,0x0003,0x0303);

  }
  
   msg = appdata ;
 
   if(node_id != 1){
   bcp_uip_udp_packet_sendto(client_conn, msg, sizeof(msg1)+2,
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT),&hdr_info);
  }
  else{
    bcp_collect_common_recv(&sender, seqno, hops,
                        appdata + 2, uip_datalen() - 2);
   }
  }
}
/*---------------------------------------------------------------------------*/
void
bcp_collect_common_send(void)
{
  static uint8_t seqno;
  struct {
    uint8_t seqno;
    uint8_t for_alignment;
    struct collect_view_data_msg msg;
  } msg;
  /* struct collect_neighbor *n; */
  uint16_t parent_etx;
  uint16_t rtmetric;
  uint16_t num_neighbors;
  uint16_t beacon_interval;
  bcp_parent_t *preferred_parent;
  rimeaddr_t parent;
  //uip_ipaddr_t* addr;
  //rpl_dag_t *dag;

  if(client_conn == NULL) {
    /* Not setup yet */
    return;
  }
  memset(&msg, 0, sizeof(msg));
  seqno++;
  if(seqno == 0) {
    /* Wrap to 128 to identify restarts */
    seqno = 128;
  }
  msg.seqno = seqno;

  rimeaddr_copy(&parent, &rimeaddr_null);
  parent_etx = 0;

  /* Let's suppose we have only one instance */
  
      uip_ds6_nbr_t *nbr;
      //nbr = uip_ds6_nbr_lookup(bcp_get_parent_ipaddr(preferred_parent));
      nbr = uip_ds6_nbr_lookup(&addr);
      printf("we are after neighbor lookup \n");

      if(nbr != NULL) {
        printf("Yay.neighbor is not null\n");
        /* Use parts of the IPv6 address as the parent address, in reversed byte order. */
        parent.u8[RIMEADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
        parent.u8[RIMEADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];

        printf("rime addr %d:%d\n",parent.u8[RIMEADDR_SIZE - 1],parent.u8[RIMEADDR_SIZE - 2]);
        parent_etx = 2;
      }
      
    rtmetric = 3;
    beacon_interval = 4;
    num_neighbors = 1;


    uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, node_id-1);

  switch(node_id){
    case 2: 
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7401,0x0001,0x0101);
      break;
    case 3: 
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7402,0x0002,0x0202);
      break;
    case 4:
      uip_ip6addr(&addr, 0xfe80,0,0,0,0x0212,0x7403,0x0003,0x0303);

  }
  

  /* num_neighbors = collect_neighbor_list_num(&tc.neighbor_list); */
  printf("udp-sender.c: We have created a packet\n");
  collect_view_construct_message(&msg.msg, &parent,
                                 parent_etx, rtmetric,
                                 num_neighbors, beacon_interval);

  bcp_uip_udp_packet_sendto(client_conn, &msg, sizeof(msg),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT), NULL);
}
/*---------------------------------------------------------------------------*/
void
bcp_collect_common_net_init(void)
{
#if CONTIKI_TARGET_Z1
  uart0_set_input(serial_line_input_byte);
#else
  uart1_set_input(serial_line_input_byte);
#endif
  serial_line_init();
}
/*---------------------------------------------------------------------------*/
static void
bcp_print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
bcp_set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, node_id);
  //uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);

  /* set server address */
  
  
  

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  bcp_set_global_address();

    

  PRINTF("UDP client process started\n");

  bcp_print_local_addresses();

  bcp_reset_beacon_timer();
   
  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  /*new connection as server */
  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
        UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));


  


  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      printf("receiving a packet/n");
      bcp_tcpip_handler();
    }
  }

  PROCESS_END();
}


/*---------------------------------------------------------------------------*/
