/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It declares
 *          everything needed to keep track of the program's state.
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

#ifndef __state_h
#define __state_h

#include "config.h"

extern const char* progName;
extern char**      argArray;
extern unsigned    nofArgs;
extern Bool        disabled;
extern Bool        lockNow;
extern Bool        unlockNow;
extern time_t      lockTrigger;
extern time_t      killTrigger;
extern pid_t       lockerPid;

#define setLockTrigger(delta) (lockTrigger = time ((time_t*) 0) + (delta))
#define setKillTrigger(delta) (killTrigger = time ((time_t*) 0) + (delta))
#define disableKillTrigger()  (killTrigger = 0)
#define resetLockTrigger()    setLockTrigger (lockTime);
#define resetTriggers()       setLockTrigger (lockTime);                   \
                              if (killTrigger) setKillTrigger (killTime);  \

extern void initState (int argc, char* argv[]);

#endif /* __state_h */
