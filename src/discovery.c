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
#include <gdk/gdk.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <iphlpapi.h>
#endif

#include "discovered.h"
#include "discovery.h"
#include "protocol1_discovery.h"
#include "protocol2_discovery.h"
#ifdef SOAPYSDR
#include "soapy_discovery.h"
#endif

#ifdef _WIN32
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

MIB_IPADDRTABLE *getIfAddressWin()
{
    int i;
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;
    LPVOID lpMsgBuf;

    pIPAddrTable = (MIB_IPADDRTABLE *)MALLOC(sizeof(MIB_IPADDRTABLE));
    if (pIPAddrTable)
    {
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) ==
            ERROR_INSUFFICIENT_BUFFER)
        {
            FREE(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE *)MALLOC(dwSize);
        }
        if (pIPAddrTable == NULL)
        {
            g_print("Memory allocation failed for GetIpAddrTable\n");
            exit(1);
        }
    }
    if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) != NO_ERROR)
    {
        printf("GetIpAddrTable failed with error %d\n", dwRetVal);
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPTSTR)&lpMsgBuf, 0, NULL))
        {
            g_print("\tError: %s", lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        exit(1);
    }

    return pIPAddrTable;
    //   if (pIPAddrTable)
    //   {
    //       FREE(pIPAddrTable);
    //       pIPAddrTable = NULL;
    //   }
}
#endif

void discovery() {
g_print("discovery\n");
  devices=0;
  protocol1_discovery();
  protocol2_discovery();
#ifdef SOAPYSDR
  soapy_discovery();
#endif
}
