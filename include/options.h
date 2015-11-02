/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It declares
 *          all option and X resource processing support.
 *
 *          Please send bug reports etc. to mce@scarlet.be.
 *
 * --------------------------------------------------------------------------
 *
 * Copyright 1990, 1992-1999, 2001-2002, 2004, 2007 by  Stefan De Troch and
 * Michel Eyckmans.
 *
 * Versions 2.0 and above of xautolock are available under version 2 of the
 * GNU GPL. Earlier versions are available under other conditions. For more
 * information, see the License file.
 *
 *****************************************************************************/

#ifndef __options_h
#define __options_h

#include "config.h"

typedef enum
{
  ca_ignore,     /* ignore corner    */
  ca_dontLock,   /* prevent locking  */
  ca_forceLock   /* lock immediately */
} cornerAction;

typedef enum
{
  msg_none,      /* as it says                           */
  msg_disable,   /* disable a running xautolock          */
  msg_enable,    /* enable a running xautolock           */
  msg_toggle,    /* toggle a running xautolock           */
  msg_exit,      /* kill a running xautolock             */
  msg_lockNow,   /* tell running xautolock to lock now   */
  msg_unlockNow, /* tell running xautolock to unlock now */
  msg_restart,   /* tell running xautolock to restart    */
} message;

/*
 *  Global option settings. Documented in options.c. 
 *  Do not modify any of these from outside that file.
 */
extern const char   *locker, *nowLocker, *notifier, *killer;
extern time_t       lockTime, killTime, notifyMargin,
                    cornerDelay, cornerRedelay;
extern int          bellPercent;
extern unsigned     cornerSize;
extern Bool         secure, notifyLock, useRedelay, resetSaver, 
                    noCloseOut, noCloseErr, detectSleep;
extern cornerAction corners[4];
extern message      messageToSend; 

extern Bool         killerSpecified, notifierSpecified;

#ifdef VMS
extern struct dsc$descriptor lockerDescr, nowLockerDescr;
extern int                   vmsStatus;  
#endif /* VMS */

extern void processOpts (Display* d, int argc, char* argv[]);

#endif /* options.h */
