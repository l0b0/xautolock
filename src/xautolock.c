/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) + Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It implements 
 *          the program's main() function as well as some boring X related
 *          chit-chat.
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

#include "config.h"
#include "options.h"
#include "state.h"
#include "miscutil.h"
#include "diy.h"
#include "message.h"
#include "engine.h"

/*
 *  X error handler. We can safely ignore everything
 *  here (mainly windows that die before we get even
 *  see them).
 */
static int
catchFalseAlarm (Display* d, XErrorEvent event)
{
  return 0;
}

/*
 *  X extension finding support.
 */
#if defined (HasXidle) || defined (HasScreenSaver)
#ifndef DEBUG
#define queryExtension(name,useIt)                  \
{                                                   \
  int dummy;                                        \
  useIt = name##QueryExtension (d, &dummy, &dummy); \
}                                                                
#endif /* DEBUG */
#endif /* HasXidle || HasScreenSaver */
  
/*
 *  Window manager related stuff.
 */
static void
wmSetup (Display* d)
{
 /*
  *  Get ourselves a dummy window in order to allow display and/or
  *  session managers etc. to use XKillClient() on us (e.g. xdm when
  *  not using XDMCP).
  * 
  *  I'm not sure whether the window needs to be mapped for xdm, but
  *  the default setup Sun uses for OpenWindows and olwm definitely
  *  requires it to be mapped.
  *
  *  If we're doing all this anyway, we might as well set the correct
  *  WM properties on the window as a convenience.
  */
  Window               ourWin;    
  XTextProperty        nameProp;  
  XClassHint*          classInfo; 
  XSetWindowAttributes attribs;   

  attribs.override_redirect = True;
  ourWin = XCreateWindow (d, DefaultRootWindow (d), -1, -1, 1, 1, 0,
		          CopyFromParent, InputOnly, CopyFromParent,
			  CWOverrideRedirect, &attribs);

  classInfo = XAllocClassHint ();
  classInfo->res_class = APPLIC_CLASS;
  (void) XStringListToTextProperty (argArray, 1, &nameProp);
  XSetWMProperties (d, ourWin, &nameProp, (XTextProperty*) 0, argArray,
                    nofArgs, (XSizeHints*) 0, (XWMHints*) 0, classInfo);
  (void) XFree (nameProp.value);
  (void) XFree (classInfo);

  (void) XMapWindow (d, ourWin);
}

/*
 *  Combat control.
 */
int
main (int argc, char* argv[])
{
  Display* d;
  time_t   t0, t1;
  Bool     useMit = False;
  Bool     useXidle = False;

 /*
  *  Find out whether there actually is a server on the other side...
  */
  if (!(d = XOpenDisplay (0))) /* = intended */
  {
    error1 ("Couldn't connect to %s\n", XDisplayName (0));
    exit (EXIT_FAILURE);
  }

 /*
  *  More initialisations.
  */
  initState (argc, argv);
  processOpts (d, argc, argv);
  wmSetup (d);
  checkConnectionAndSendMessage (d);
  resetTriggers ();

  if (!noCloseOut) (void) fclose (stdout);
  if (!noCloseErr) (void) fclose (stderr);

#ifdef HasXidle
  queryExtension (Xidle, useXidle)
#endif /* HasXidle */

#ifdef HasScreenSaver
  if (!useXidle) queryExtension (XScreenSaver, useMit)
#endif /* HasScreenSaver */

  if (!useXidle && !useMit) initDiy (d);

  (void) XSetErrorHandler ((XErrorHandler) catchFalseAlarm);
  (void) XSync (d, 0);

  t0 = time (NULL);


 /*
  *  Main event loop.
  */
  for (;;)
  {
    lookForMessages (d);

    if (useXidle || useMit)
    {
      queryIdleTime (d, useXidle);
    }
    else
    {
      processEvents ();
    }

    queryPointer (d);
    evaluateTriggers (d);

    (void) sleep (1);

    if (detectSleep)
    {
      t1 = time (NULL);
      if ((unsigned long) t1 - (unsigned long) t0 > 3) resetLockTrigger ();
      t0 = t1;
    }
  }

  return 0; /* Never reached! */
}
