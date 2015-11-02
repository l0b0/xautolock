/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It takes care
 *          of most OS dependencies, and defines the program's default
 *          settings.
 *
 *          Please send bug reports etc. to eyckmans@imec.be.
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

#ifndef __config_h
#define __config_h

#if defined(hpux) || defined (__hpux)
#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE
#endif /* _HPUX_SOURCE */
#endif /* hpux || __hpux */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>

#ifndef VMS
#include <pwd.h>
#include <sys/wait.h>
#endif /* VMS */

#ifdef VMS
#define HasVFork
#include <descrip.h>
#include <clidef.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <ssdef.h>
#include <processes.h>
#include <unixio.h>     /* Needed for close ()  */
#include <unixlib.h>    /* Needed for getpid () */
#endif /* VMS */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

#ifdef HasXidle
#include "xidle.h"
#endif /* HasXidle */

#ifdef HasScreenSaver
#include <X11/extensions/scrnsaver.h>
#endif /* HasScreenSaver */

#ifndef HasVFork
#define vfork           fork
#endif /* HasVFork */

#define SECURE            False       /* default -secure setting           */
#define BELL_PERCENT      40          /* as is says                        */

#define MIN_LOCK_MINS     1           /* minimum number of minutes
                                         before firing up the locker       */
#define LOCK_MINS         10          /* default ...                       */
#define MAX_LOCK_MINS     60          /* maximum ...                       */

#define MIN_KILL_MINS     10          /* minimum number of minutes
                                         before firing up the killer       */
#define KILL_MINS         20          /* default ...                       */
#define MAX_KILL_MINS     120         /* maximum ...                       */

#define CREATION_DELAY    30          /* should be > 10 and
                                         < min(45,(MIN_LOCK_MINS*30))      */
#define CORNER_SIZE       10          /* size in pixels of the
                                         force-lock areas                  */
#define CORNER_DELAY      5           /* number of seconds to wait
                                         before forcing a lock             */

#ifdef VMS
#define SLOW_VMS_DELAY    15          /* explained in VMS.NOTES file       */
#endif /* VMS */

#ifdef VMS
#define LOCKER            "mcr sys$system:decw$pausesession"
                                      /* default locker command            */
#else /* VMS */
#define LOCKER            "xlock"     /* default locker command (avoid
                                         the -allowRoot option!)           */
#endif /* VMS */
#define NOTIFIER          ""          /* default notifier command          */
#define KILLER            ""          /* default killer command            */

#define DUMMY_RES_CLASS   "_xAx_"     /* some X versions don't like a 0
                                         class name, and implementing real
				         classes isn't worth it            */
#define APPLIC_CLASS      "Xautolock" /* application class                 */


#ifdef VMS

#if defined (VAX) && defined (vaxc)
typedef long  pid_t;
#endif /* VAX && vaxc */

/*
 *  Different versions of the DEC C compiler for OpenVMS Alpha define
 *  pid_t in different ways. Later versions define either __PID_T (v5.2)
 *  or __DEV_T (V5.0) when pid_t is already typedef'ed. DEC C V1.3 did
 *  neither, so typedef if those aren't defined.
 */
#if     defined (__DECC)    &&  defined (__alpha) \
    && !defined (____PID_T) && !defined (__DEV_T)
typedef long  pid_t;
#endif /* __DECC && __alpha && !____PID_T && !__DEV_T */

#endif /* VMS */

#endif /* __config_h */
