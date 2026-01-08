#include "wsn_util.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/netreg.h"

#define DEFAULT_PORT (1)

static gnrc_netreg_entry_t server = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF);

// Taken from the shell/cmds/gnrc_udp.c udp shell command code

void WSNUtil_StartServer(kernel_pid_t pid)
{
  uint16_t port;

  /* check if server is already running */
  if (server.target.pid != KERNEL_PID_UNDEF) {
    printf("Error: server already running on port %" PRIu32 "\n",
           server.demux_ctx);
    return;
  }
  /* parse port */
  port = DEFAULT_PORT;
  if (port == 0) {
    printf("Error: invalid port specified\n");
    return;
  }
  /* start server (which means registering pktdump for the chosen port) */
  server.target.pid = pid;
  server.demux_ctx = (uint32_t)port;
  gnrc_netreg_register(GNRC_NETTYPE_UDP, &server);
  printf("Success: started UDP server on port %" PRIu16 "\n", port);
}

void WSNUtil_Send(const char *addr_str, const char *data, size_t size)
{
  netif_t *netif;
  uint16_t port = DEFAULT_PORT;
  ipv6_addr_t addr;

  /* parse destination address */
  if (netutils_get_ipv6(&addr, &netif, addr_str) < 0) {
    printf("Error: unable to parse destination address\n");
    return;
  }

  gnrc_pktsnip_t *payload, *udp, *ip;
  unsigned payload_size;
  /* allocate payload */
  payload = gnrc_pktbuf_add(NULL, data, size, GNRC_NETTYPE_UNDEF);
  if (payload == NULL) {
    printf("Error: unable to copy data to packet buffer\n");
    return;
  }
  /* store size for output */
  payload_size = (unsigned)payload->size;
  /* allocate UDP header, set source port := destination port */
  udp = gnrc_udp_hdr_build(payload, port, port);
  if (udp == NULL) {
    printf("Error: unable to allocate UDP header\n");
    gnrc_pktbuf_release(payload);
    return;
  }
  /* allocate IPv6 header */
  ip = gnrc_ipv6_hdr_build(udp, NULL, &addr);
  if (ip == NULL) {
    printf("Error: unable to allocate IPv6 header\n");
    gnrc_pktbuf_release(udp);
    return;
  }
  /* add netif header, if interface was given */
  if (netif != NULL) {
    gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
    if (netif_hdr == NULL) {
      printf("Error: unable to allocate netif header\n");
      gnrc_pktbuf_release(ip);
      return;
    }
    gnrc_netif_hdr_set_netif(netif_hdr->data,
                             container_of(netif, gnrc_netif_t, netif));
    ip = gnrc_pkt_prepend(ip, netif_hdr);
  }
  /* send packet */
  if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP,
                                 GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
    printf("Error: unable to locate UDP thread\n");
    gnrc_pktbuf_release(ip);
    return;
  }
  /* access to `payload` was implicitly given up with the send operation
         * above
         * => use temporary variable for output */
  printf("Success: sent %u byte(s) to [%s]:%u\n", payload_size, addr_str,
         port); 
}
