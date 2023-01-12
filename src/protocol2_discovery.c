/* Copyright (C)
 * 2018 - John Melton, G0ORX/N6LYT
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#endif
#include <string.h>
#include <errno.h>

#include "discovered.h"

static char interface_name[64];
static struct sockaddr_in interface_addr = {0};
static struct sockaddr_in interface_netmask = {0};

#define DISCOVERY_PORT 1024
static int discovery_socket;

#ifdef _WIN32
MIB_IPADDRTABLE *getIfAddressWin();
void protocol2_discover(int IFnum, u_long IFaddr, u_long IFnet_mask);
#else
void protocol2_discover(struct ifaddrs *iface);
#endif

static GThread *discover_thread_id;
gpointer protocol2_discover_receive_thread(gpointer data);

void print_device(int i)
{
    g_print("discovery: found protocol=%d device=%d software_version=%s status=%d address=%s (%02X:%02X:%02X:%02X:%02X:%02X) on %s\n",
            discovered[i].protocol,
            discovered[i].device,
            discovered[i].software_version,
            discovered[i].status,
            inet_ntoa(discovered[i].info.network.address.sin_addr),
            discovered[i].info.network.mac_address[0],
            discovered[i].info.network.mac_address[1],
            discovered[i].info.network.mac_address[2],
            discovered[i].info.network.mac_address[3],
            discovered[i].info.network.mac_address[4],
            discovered[i].info.network.mac_address[5],
            discovered[i].info.network.interface_name);
}

void protocol2_discovery()
{
    struct ifaddrs *addrs, *ifa;
    int i;

#ifdef _WIN32
    PMIB_IPADDRTABLE pIPAddrTable;
    pIPAddrTable = getIfAddressWin();

    for (i = 0; i < (int)pIPAddrTable->dwNumEntries; i++)
    {
        g_print("\n\tSearch on Interface Index:\t%ld\n", pIPAddrTable->table[i].dwIndex);
        protocol2_discover(pIPAddrTable->table[i].dwIndex, (u_long)pIPAddrTable->table[i].dwAddr, pIPAddrTable->table[i].dwMask);
    }
    //  if (pIPAddrTable)
    //  {
    //      FREE(pIPAddrTable);
    //      pIPAddrTable = NULL;
    //  }

#else
    getifaddrs(&addrs);
    ifa = addrs;
    while (ifa)
    {
        g_main_context_iteration(NULL, 0);
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            if ((ifa->ifa_flags & IFF_UP) == IFF_UP && (ifa->ifa_flags & IFF_RUNNING) == IFF_RUNNING && (ifa->ifa_flags & IFF_LOOPBACK) != IFF_LOOPBACK)
            {
                protocol2_discover(ifa);
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(addrs);
#endif
    g_print("protocol2_discovery found %d devices\n", devices);

    for (i = 0; i < devices; i++)
    {
        print_device(i);
    }
}

#ifdef _WIN32
void protocol2_discover(int IFnum, u_long IFaddr, u_long IFnet_mask)
#else
void protocol2_discover(struct ifaddrs *iface)
#endif
{
    int rc;
    struct sockaddr_in *sa;
    struct sockaddr_in *mask;
    char addr[16];
    char net_mask[16];

#ifdef _WIN32
    IN_ADDR IPAddr;
    sprintf(interface_name, "%s %d", "Ethernet", IFnum);
#else
    strcpy(interface_name, iface->ifa_name);
#endif

    g_print("protocol2_discover: looking for HPSDR devices on %s\n", interface_name);

    // send a broadcast to locate metis boards on the network
    discovery_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (discovery_socket < 0)
    {
        perror("protocol2_discover: create socket failed for discovery_socket\n");
        exit(-1);
    }

    int optval = 1;
    setsockopt(discovery_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));
    // setsockopt(discovery_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    interface_addr.sin_family = AF_INET;
#ifdef _WIN32
    interface_netmask.sin_addr.s_addr = IFnet_mask;
    interface_addr.sin_addr.s_addr = IFaddr;
#else
    sa = (struct sockaddr_in *)iface->ifa_addr;
    mask = (struct sockaddr_in *)iface->ifa_netmask;
    interface_addr.sin_addr.s_addr = sa->sin_addr.s_addr;
    interface_netmask.sin_addr.s_addr = mask->sin_addr.s_addr;
#endif

    interface_addr.sin_port = htons(0);
    if (bind(discovery_socket, (struct sockaddr *)&interface_addr, sizeof(interface_addr)) < 0)
    {
        perror("protocol2_discover: bind socket failed for discovery_socket\n");
        exit(1);
    }

#ifdef _WIN32
    IPAddr.S_un.S_addr = IFaddr;
    strcpy(addr, inet_ntoa(IPAddr));
    g_print("\tIP Address:     \t%s\n", inet_ntoa(IPAddr));
    IPAddr.S_un.S_addr = IFnet_mask;
    strcpy(net_mask, inet_ntoa(IPAddr));
#else
    strcpy(addr, inet_ntoa(sa->sin_addr));
    strcpy(net_mask, inet_ntoa(mask->sin_addr));
#endif
    g_print("protocol2_discover: bound to %s %s %s\n", interface_name, addr, net_mask);

    // allow broadcast on the socket
    int on = 1;
    rc = setsockopt(discovery_socket, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on));
    if (rc != 0)
    {
        g_print("protocol2_discover: cannot set SO_BROADCAST: rc=%d\n", rc);
        exit(-1);
    }

    // setup to address
    struct sockaddr_in to_addr = {0};
    to_addr.sin_family = AF_INET;
    to_addr.sin_port = htons(DISCOVERY_PORT);
    to_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // start a receive thread to collect discovery response packets
    discover_thread_id = g_thread_new("protocol2 discover receive", protocol2_discover_receive_thread, NULL);
    if (!discover_thread_id)
    {
        g_print("g_thread_new failed on protocol2_discover_receive_thread\n");
        exit(-1);
    }
    g_print("protocol2_disovery: thread_id=%p\n", discover_thread_id);

    // send discovery packet
    unsigned char buffer[60];
    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x02;
    int i;
    for (i = 5; i < 60; i++)
    {
        buffer[i] = 0x00;
    }

    if (sendto(discovery_socket, buffer, 60, 0, (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0)
    {
        perror("protocol2_discover: sendto socket failed for discovery_socket\n");
        if (errno != EHOSTUNREACH)
        {
            exit(-1);
        }
    }

    // wait for receive thread to complete
    g_thread_join(discover_thread_id);
#ifdef _WIN32
    closesocket(discovery_socket);
#else
    close(discovery_socket);
    g_print("protocol2_discover: exiting discover for %s\n", iface->ifa_name);
#endif
}

// void* protocol2_discover_receive_thread(void* arg) {
gpointer protocol2_discover_receive_thread(gpointer data)
{
    struct sockaddr_in addr;
    socklen_t len;
    unsigned char buffer[2048];
    int bytes_read;
    struct timeval tv;
    int i;
    int version;

    version = 0;
#ifdef _WIN32
    int timeout = 2000;
    setsockopt(discovery_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(discovery_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
#endif

    len = sizeof(addr);
    while (1)
    {
        bytes_read = recvfrom(discovery_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &len);
        if (bytes_read < 0)
        {
            g_print("protocol2_discover: bytes read %d\n", bytes_read);
            perror("protocol2_discover: recvfrom socket failed for discover_receive_thread");
            break;
        }
        g_print("protocol2_discover: received %d bytes\n", bytes_read);
        if (bytes_read == 1444)
        {
            if (devices > 0)
            {
                break;
            }
        }
        else
        {
            if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0)
            {
                int status = buffer[4] & 0xFF;
                if (status == 2 || status == 3)
                {
                    if (devices < MAX_DEVICES)
                    {
                        discovered[devices].protocol = PROTOCOL_2;
                        discovered[devices].device = buffer[11] & 0xFF;
                        switch (discovered[devices].device)
                        {
                        case NEW_DEVICE_ATLAS:
                            strcpy(discovered[devices].name, "Atlas");
                            // discovered[devices].supported_receivers=5;
                            discovered[devices].adcs = 1;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_HERMES:
                            strcpy(discovered[devices].name, "Hermes");
                            // discovered[devices].supported_receivers=5;
                            discovered[devices].adcs = 1;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_HERMES2:
                            strcpy(discovered[devices].name, "Hermes2");
                            // discovered[devices].supported_receivers=7;
                            discovered[devices].adcs = 2;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_ANGELIA:
                            strcpy(discovered[devices].name, "Angelia");
                            // discovered[devices].supported_receivers=7;
                            discovered[devices].adcs = 2;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_ORION:
                            strcpy(discovered[devices].name, "Orion");
                            // discovered[devices].supported_receivers=7;
                            discovered[devices].adcs = 2;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_ORION2:
                            strcpy(discovered[devices].name, "Orion2");
                            // discovered[devices].supported_receivers=7;
                            discovered[devices].adcs = 2;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        case NEW_DEVICE_HERMES_LITE:
                            strcpy(discovered[devices].name, "Hermes Lite");
                            // discovered[devices].supported_receivers=5;
                            discovered[devices].adcs = 1;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 30720000.0;
                            break;
                        default:
                            strcpy(discovered[devices].name, "Unknown");
                            discovered[devices].supported_receivers = 5;
                            discovered[devices].adcs = 1;
                            discovered[devices].frequency_min = 0.0;
                            discovered[devices].frequency_max = 61440000.0;
                            break;
                        }

                        discovered[devices].supported_receivers = buffer[20] & 0xFF;
                        discovered[devices].supported_transmitters = 1;

                        version = buffer[13] & 0xFF;
                        sprintf(discovered[devices].software_version, "%d", version);
                        for (i = 0; i < 6; i++)
                        {
                            discovered[devices].info.network.mac_address[i] = buffer[i + 5];
                        }
                        discovered[devices].status = status;
                        memcpy((void *)&discovered[devices].info.network.address, (void *)&addr, sizeof(addr));
                        discovered[devices].info.network.address_length = sizeof(addr);
                        memcpy((void *)&discovered[devices].info.network.interface_address, (void *)&interface_addr, sizeof(interface_addr));
                        memcpy((void *)&discovered[devices].info.network.interface_netmask, (void *)&interface_netmask, sizeof(interface_netmask));
                        discovered[devices].info.network.interface_length = sizeof(interface_addr);
                        strcpy(discovered[devices].info.network.interface_name, interface_name);
                        g_print("protocol2_discover: found %d protocol=%d device=%d software_version=%s status=%d address=%s (%02X:%02X:%02X:%02X:%02X:%02X) on %s DDCs=%d\n",
                                devices,
                                discovered[devices].protocol,
                                discovered[devices].device,
                                discovered[devices].software_version,
                                discovered[devices].status,
                                inet_ntoa(discovered[devices].info.network.address.sin_addr),
                                discovered[devices].info.network.mac_address[0],
                                discovered[devices].info.network.mac_address[1],
                                discovered[devices].info.network.mac_address[2],
                                discovered[devices].info.network.mac_address[3],
                                discovered[devices].info.network.mac_address[4],
                                discovered[devices].info.network.mac_address[5],
                                discovered[devices].info.network.interface_name,
                                buffer[20] & 0xFF);
                        devices++;
                    }
                }
            }
        }
    }
    g_print("protocol2_discover: exiting protocol2_discover_receive_thread\n");
    return NULL;
}
