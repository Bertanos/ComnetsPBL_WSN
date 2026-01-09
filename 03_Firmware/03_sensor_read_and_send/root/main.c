#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "thread.h"
#include "shell.h"
#include "msg.h"

#include "net/gnrc.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/udp.h"
#include "net/utils.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/rpl.h"
#include "net/gnrc/rpl/structs.h"

#include "periph/i2c.h"

#include "ztimer.h"

#include "main.h"
#include "wsn_util.h"

#define MSG_QUEUE_SIZE     (8)

static msg_t _main_msg_queue[MSG_QUEUE_SIZE];
static msg_t _thread_msg_queue[MSG_QUEUE_SIZE];
static char threadStack[THREAD_STACKSIZE_DEFAULT];
static kernel_pid_t threadPid = KERNEL_PID_UNDEF;
static msg_t ipcMsg = (msg_t) {.type = WSN_IPC_PERIODIC_OPERATION};

static bool running = false;
static ztimer_t intervalTimer;

static WSN_Role_e myRole = WSN_UNSET_ROLE;

static char *rootAddrStr = "2001::1";
static int num_messages = 0;

// Function for root node to parse incoming packets.
static void PacketReceptionHandler(gnrc_pktsnip_t *pkt)
{
  int snips = 0;
  int size = 0;
  gnrc_pktsnip_t *snip = pkt;
  while(snip != NULL)
  {
    switch(snip->type)
    {
      case GNRC_NETTYPE_NETIF: // MAC layer
        {
          break;
        }
      case GNRC_NETTYPE_IPV6: // IPv6 layer
        {
          break;
        }
      case GNRC_NETTYPE_UDP: // UDP layer
        {
          break;
        }
      case GNRC_NETTYPE_UNDEF: // Payload
        {
          char buf[32]; 
          memset(buf, 0x00, 32);
          memcpy(buf, snip->data, snip->size);
          printf("payload: %s\n", buf);
          printf("length: %d\n", snip->size);
          break;
        }
      default:
        break;
    }
    size += snip->size;
    snip = snip->next;
    snips++;
  }
}

void *WSN_NodeThread(void *arg)
{
  (void) arg;
  msg_t msg, reply;
  msg_init_queue(_thread_msg_queue, MSG_QUEUE_SIZE);
  printf("WSN_NodeThread Started. %d\n", myRole);
  do {

    // msg_receive(&msg) is a blocking call. It blocks until there is a message in the queue of this thread
    msg_receive(&msg);

    switch(msg.type)
    {
      case GNRC_NETAPI_MSG_TYPE_RCV:
        {
          printf("Data received\n");

          // Handle our newly received packet
          PacketReceptionHandler((gnrc_pktsnip_t *) msg.content.ptr);

          // We need to release the memory after we're done with it
          gnrc_pktbuf_release((gnrc_pktsnip_t *) msg.content.ptr);
          break;
        }
      case GNRC_NETAPI_MSG_TYPE_SND:
        {
          // UNUSED
          break;
        }
      case GNRC_NETAPI_MSG_TYPE_GET:
      case GNRC_NETAPI_MSG_TYPE_SET:
        {
          msg_reply(&msg, &reply);
          break;
        }
      case WSN_IPC_PERIODIC_OPERATION:
        {
          printf("WSN_IPC_PERIODIC_OPERATION\n");

          if (running)
          {
            //ztimer_set_msg(ZTIMER_USEC, &intervalTimer, OPERATION_PERIOD_US, &ipcMsg, threadPid);
          }
          break;
        }
      default:
        {
          break;
        }
    }
  } while (running);
  printf("Thread exiting\n");
}

void WSN_Init(WSN_Role_e role)
{
  if (role != WSN_ROOT_ROLE && role != WSN_SENSOR_ROLE)
  {
    printf("Bad role value! %d\n", role);
    return;
  }
  if (myRole != WSN_UNSET_ROLE)
  {
    printf("Run 'wsn stop' first!\n");
    return;
  }

  myRole = role;

  printf("Initializing %s node\n", (myRole == WSN_SENSOR_ROLE) ? "SENSOR" : "ROOT");
  threadPid = thread_create(threadStack, sizeof(threadStack), THREAD_PRIORITY_MAIN - 1, 0, WSN_NodeThread, NULL, (role == WSN_SENSOR_ROLE) ? "wsn_sensor" : "wsn_root");

  if (myRole == WSN_ROOT_ROLE)
  {
    netif_t *mainIface = netif_iter(NULL);
    int16_t mainIfaceId = netif_get_id(mainIface);
    char cmdBuf[32];

    // Set global ipv6 addr
    ipv6_addr_t addr;
    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID;
    uint8_t prefix_len = ipv6_addr_split_int(rootAddrStr, '/', 64U);
    prefix_len = (prefix_len < 1) ? 64U : prefix_len;
    ipv6_addr_from_str(&addr, rootAddrStr);
    flags |= (prefix_len << 8U);
    if (netif_set_opt(mainIface, NETOPT_IPV6_ADDR, flags, &addr, sizeof(addr)) < 0)
    {
      printf("Error: unable to add IPv6 addr\n");
      return;
    }
    printf("Successfully added IPv6 address %s\n", rootAddrStr);

    // rpl init 7
    if (gnrc_rpl_init(mainIfaceId) < 0)
    {
      printf("Error: unable to init rpl\n");
      return;
    }
    printf("Successfully initded rpl \n");

    // rpl root 
    gnrc_rpl_instance_t *inst = gnrc_rpl_root_init(mainIfaceId, &addr, false, false);
    if (inst == NULL)
    {
      printf("Error: unable to init rpl root\n");
      return;
    }
    printf("Successfully inited rpl root\n");

    WSNUtil_StartServer(threadPid);
    printf("Successfully inited WSN ROOT and UDP listener\n");
  }
  else if (myRole == WSN_SENSOR_ROLE)
  {
    //
  }
}

void WSN_Deinit(void)
{
  myRole = WSN_UNSET_ROLE;
  running = false;
}

int main(void) {
  printf("Hello I'm the root node\n");
  WSN_Init(WSN_ROOT_ROLE);
  running = true;
}