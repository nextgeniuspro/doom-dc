// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(linux) || defined(__SVR4)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#else
#ifdef __BEOS__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <byteorder.h>
#ifndef IPPORT_USERRESERVED
#define IPPORT_USERRESERVED	5000
#endif
#else
#ifdef __WIN32__
#define Win32_Winsock
#include <windows.h>
#else
#endif
#endif
#endif

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"

#ifdef __GNUG__
#pragma implementation "i_net.h"
#endif
#include "i_net.h"





// For some odd reason...
#ifndef B_HOST_IS_LENDIAN
#define B_HOST_IS_LENDIAN 1
#endif
#if !defined(sparc) && B_HOST_IS_LENDIAN
#ifndef ntohl
#define ntohl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))
#endif

#ifndef ntohs
#define ntohs(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8)))
#endif
	  
#ifndef htonl
#define htonl(x) ntohl(x)
#endif
#ifndef htons
#define htons(x) ntohs(x)
#endif
#endif

void	NetSend (void);
boolean NetListen (void);


//
// NETWORKING
//

int	DOOMPORT =	0;

int			sendsocket;
int			insocket;
 

void	(*netget) (void);
void	(*netsend) (void);


//
// UDPsocket
//
int UDPsocket (void)
{
    
		
    return -1;
}

//
// BindToLocalPort
//
void
BindToLocalPort
( int	s,
  int	port )
{
    
}


//
// PacketSend
//
void PacketSend (void)
{  
}


//
// PacketGet
//
void PacketGet (void)
{
    
  
}



int GetLocalAddress (void)
{
   return 0;
}


//
// I_InitNetwork
//
void I_InitNetwork (void)
{

	doomcom = malloc(sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );

	doomcom-> ticdup = 1;
	doomcom-> extratics = 0;

	    // single player game
        netgame = FALSE;
        doomcom->id = DOOMCOM_ID;
        doomcom->numplayers = doomcom->numnodes = 1;
        doomcom->deathmatch = FALSE;
        doomcom->consoleplayer = 0;
 
  
}


void I_NetCmd (void)
{
   
}

