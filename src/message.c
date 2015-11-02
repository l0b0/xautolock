/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It implements
 *          the program's IPC features.
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

#include "message.h"
#include "state.h"
#include "options.h"
#include "miscutil.h"

static Atom semaphore;   /* semaphore property for locating 
                            an already running xautolock    */
static Atom messageAtom; /* message property for talking to
                            an already running xautolock    */

#define SEM_PID "_SEMAPHORE_PID"  
#define MESSAGE "_MESSAGE"       

/*
 *  Message handlers.
 */
static void
disableByMessage (Display* d, Window root)
{
 /*
  *  The order in which things are done is rather important here.
  */
  if (!secure)
  {
    setLockTrigger (lockTime);
    disableKillTrigger ();
    disabled = True;
  }
}

static void
enableByMessage (Display* d, Window root)
{
  if (!secure) 
  {
    resetTriggers ();
    disabled = False;
  }
}

static void
toggleByMessage (Display* d, Window root)
{
  if (!secure)
  {
    if ((disabled = !disabled)) /* = intended */
    {
      setLockTrigger (lockTime);
      disableKillTrigger ();
    }
    else
    {
      resetTriggers ();
    }
  }
}

static void
exitByMessage (Display* d, Window root)
{
  if (!secure)
  {
    error0 ("Exiting. Bye bye...\n");
    exit (0);
  }
}

static void
lockNowByMessage (Display* d, Window root)
{
  if (!secure && !disabled) lockNow = True;
}

static void
unlockNowByMessage (Display* d, Window root)
{
  if (!secure && !disabled) unlockNow = True;
}

static void
restartByMessage (Display* d, Window root)
{
  if (!secure)
  {
    XDeleteProperty (d, root, semaphore);
    XFlush (d);
    execv (argArray[0], argArray);
  }
}

/*
 *  Function for looking for messages from another xautolock.
 */
void
lookForMessages (Display* d)
{
  Window        root;         /* as it says              */
  Atom          type;         /* actual property type    */
  int           format;       /* dummy                   */
  unsigned long nofItems;     /* dummy                   */
  unsigned long after;        /* dummy                   */
  int*          contents;     /* message property value  */
  static Bool   first = True; /* is this the first call? */

 /*
  *  It would be cleaner (more X-like, using less cpu time and
  *  network bandwith, ...) to implement this functionality by
  *  keeping an eye open for PropertyNotify events on the root
  *  window, or (even better) by using ClientMessage events.
  *  Maybe I'll rewrite it one day to do just that, but the
  *  current approach works just fine too.
  *
  *  Note that we must clear the property before acting on it!
  *  Otherwise funny things can happen on receipt of msg_exit.
  */
  root = RootWindowOfScreen (ScreenOfDisplay (d, 0));

  (void) XGetWindowProperty (d, root, messageAtom, 0L, 2L, 
                             False, AnyPropertyType, &type,
			     &format, &nofItems, &after,
			     (unsigned char**) &contents);

  XDeleteProperty (d, root, messageAtom);
  XFlush (d);

 /*
  *  The very first time we get here, we ignore the message. Why?
  *  Well, suppose some weirdo sends a SIGSTOP to a running xautolock,
  *  then does an `xautolock -exit' and finally sends a SIGKILL to 
  *  the stopped xautolock. This would leave an unread message sitting
  *  around. Unread that is, till the next xautolock is started.
  *
  *  To solve this, we want any xautolock that enters the main event
  *  loop to consume and ignore any message that may have been around
  *  for a while and forgotten about.
  */
  if (first)
  {
    first = False;
  }
  else if (type == XA_INTEGER)
  {
    switch (*contents)
    {
      case msg_disable:
       disableByMessage (d, root);
       break;

      case msg_enable:
       enableByMessage (d, root);
       break;

      case msg_toggle:
       toggleByMessage (d, root);
       break;

      case msg_lockNow:
       lockNowByMessage (d, root);
       break;

      case msg_unlockNow:
       unlockNowByMessage (d, root);
       break;

      case msg_restart:
       restartByMessage (d, root);
       break;

      case msg_exit:
       exitByMessage (d, root);
       break;

      default:
       /* unknown message, ignore silently */
       break;
    }
  }

  (void) XFree ((char*) contents);
}

/*
 *  Function for creating the communication atoms.
 */
void
getAtoms (Display* d)
{
  char* sem; /* semaphore property name */
  char* mes; /* message property name   */
  char* ptr; /* iterator                */

  sem = newArray (char, strlen (progName) + strlen (SEM_PID) + 1);
  (void) sprintf (sem, "%s%s", progName, SEM_PID);
  for (ptr = sem; *ptr; ++ptr) *ptr = (char) toupper (*ptr);
  semaphore = XInternAtom (d, sem, False);
  free (sem);

  mes = newArray (char, strlen (progName) + strlen (MESSAGE) + 1);
  (void) sprintf (mes, "%s%s", progName, MESSAGE);
  for (ptr = mes; *ptr; ++ptr) *ptr = (char) toupper (*ptr);
  messageAtom = XInternAtom (d, mes, False);
  free (mes);
}

/*
 *  Function for finding out whether another xautolock is already 
 *  running and for sending it a message if that's what the user
 *  wanted.
 */
void
checkConnectionAndSendMessage (Display* d)
{
  pid_t         pid;      /* as it says               */
  Window        root;     /* as it says               */
  Atom          type;     /* actual property type     */
  int           format;   /* dummy                    */
  unsigned long nofItems; /* dummy                    */
  unsigned long after;    /* dummy                    */
  pid_t*        contents; /* semaphore property value */

  getAtoms (d);

  root = RootWindowOfScreen (ScreenOfDisplay (d, 0));

  (void) XGetWindowProperty (d, root, semaphore, 0L, 2L, False,
                             AnyPropertyType, &type, &format,
			     &nofItems, &after,
                             (unsigned char**) &contents);

  if (type == XA_INTEGER)
  {
   /*
    *  This breaks if the other xautolock is not on the same machine.
    */
    if (kill (*contents, 0))
    {
      if (messageToSend)
      {
        error1 ("No process with PID %d, or the process "
		"is owned by another user.\n", *contents);
        exit (EXIT_FAILURE);
      }
    }
    else if (messageToSend)
    {
      (void) XChangeProperty (d, root, messageAtom, XA_INTEGER, 
                              8, PropModeReplace, 
			      (unsigned char*) &messageToSend, 
			      (int) sizeof (messageToSend));
      XFlush (d);
      exit (EXIT_SUCCESS);
    }
    else
    {
#ifdef VMS
      error2 ("%s is already running (PID %x).\n", progName, *contents);
#else /* VMS */
      error2 ("%s is already running (PID %d).\n", progName, *contents);
#endif /* VMS */
      exit (EXIT_FAILURE);
    }
  }
  else if (messageToSend)
  {
    error1 ("Could not locate a running %s.\n", progName);
    exit (EXIT_FAILURE);
  }

  pid = getpid ();
  (void) XChangeProperty (d, root, semaphore, XA_INTEGER, 8, 
                          PropModeReplace, (unsigned char*) &pid,
			  (int) sizeof (pid));

  (void) XFree ((char*) contents);
}
