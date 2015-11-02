/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It implements 
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

#include "state.h"
#include "miscutil.h"

const char* progName    = 0;     /* our own name                       */
char**      argArray    = 0;     /* our command line arguments         */
unsigned    nofArgs     = 0;     /* number of command line arguments   */

Bool        disabled    = False; /* whether to ignore all timeouts     */
Bool        lockNow     = False; /* whether to lock immediately        */
Bool        unlockNow   = False; /* whether to unlock immediately      */
time_t      lockTrigger = 0;     /* time at which to invoke the locker */
time_t      killTrigger = 0;     /* time at which to invoke the killer */
pid_t       lockerPid   = 0;     /* process id of the current locker   */

/*
 *  Please have a guess what this is for... :-)
 */
void 
initState (int argc, char* argv[])
{
  int i = 0;
  char* ptr;

 /*
  *  Beautify argv[0] and remember it for later use. Also remember
  *  the complete set of arguments just in case we're asked to
  *  restart ourselves later on.
  */
  argArray = newArray (char*, argc + 1);

  while (argv[i])
  {
    argArray[i] = strdup (argv[i]);
    ++i;
  }

  argArray[i] = 0;
 
#ifdef VMS
  if (ptr = strrchr (argv[0], ']'))
  {
    progName = ptr + 1;
  }
  else
  {
    progName = argv[0];
  }

  if ((ptr = strchr (progName, '.'))) /* = intended */
  {
    *ptr = '\0';
  }
#else /* VMS */
  if ((ptr = strrchr (argv[0], '/'))) /* = intended */
  {
    progName = ptr + 1;
  }
  else
  {
    progName = argv[0];
  }
#endif /* VMS */
}
