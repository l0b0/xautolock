/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) + Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It implements 
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

#include "options.h"
#include "state.h"
#include "miscutil.h"
#include "version.h"

/*
 *  Global option settings. Do not modify outside this file.
 */
const char*  locker = LOCKER;            /* as it says                  */
const char*  nowLocker = LOCKER;         /* as it says                  */
const char*  notifier = NOTIFIER;        /* as it says                  */
const char*  killer = KILLER;            /* as it says                  */
time_t       lockTime = LOCK_MINS;       /* as it says                  */
time_t       killTime = KILL_MINS;       /* as it says                  */
time_t       notifyMargin;               /* as it says                  */
Bool         secure = SECURE;            /* as it says                  */
int          bellPercent = BELL_PERCENT; /* as it says                  */
unsigned     cornerSize = CORNER_SIZE;   /* as it says                  */
time_t       cornerDelay = CORNER_DELAY; /* as it says                  */
time_t       cornerRedelay;              /* as it says                  */
Bool         notifyLock = False;         /* whether to notify the user
                                            before locking              */
Bool         useRedelay = False;         /* as it says                  */
cornerAction corners[4] = { ca_ignore, ca_ignore, ca_ignore, ca_ignore };
                                         /* default cornerActions       */
Bool         resetSaver = False;         /* whether to reset the X 
				            screensaver                 */
Bool         noCloseOut = False;         /* whether keep stdout open    */
Bool         noCloseErr = False;         /* whether keep stderr open    */
message      messageToSend = msg_none;   /* message to send to an
                                            already running xautolock   */
Bool         detectSleep = False;        /* whether to reset the timers
					    after a (laptop) sleep, 
					    i.e. after a big time jump  */

#ifdef VMS
struct dsc$descriptor lockerDescr;       /* used to fire up the locker  */
struct dsc$descriptor nowLockerDescr;    /* used to fire up the locker  */
int                   vmsStatus = 1;     /* locker completion status    */
#endif /* VMS */

Bool         notifierSpecified = False;  
Bool         killerSpecified = False;

/*
 *  Guess what, these are private.
 */
static Bool killTimeSpecified = False;
static Bool redelaySpecified = False;
static Bool bellSpecified = False;
static Bool dummySpecified;

static void usage (int exitCode);

/*
 *  Argument scanning support.
 */
static Bool
getInteger (const char* arg, int* in)
{
  char c; /* dummy */
  return (sscanf (arg, "%d%c", in, &c) == 1);
}

static Bool
getPositive (const char* arg, int* pos)
{
  return getInteger (arg, pos) && *pos >= 0;
}

/*
 *  Option action functions
 */
static Bool
helpAction (Display* d, const char* arg)
{
  usage (EXIT_SUCCESS);
  return True; /* keep gcc happy */
}

static Bool
versionAction (Display* d, const char* arg)
{
  error2 ("%s : version %s\n", progName, VERSION);
  exit (EXIT_SUCCESS);
  return True; /* keep gcc happy */
}

static Bool
lockerAction (Display* d, const char* arg)
{
  nowLocker = locker = arg;
  return True;
}

static Bool
nowLockerAction (Display* d, const char* arg)
{
  nowLocker = arg;
  return True;
}

static Bool
killerAction (Display* d, const char* arg)
{
  killerSpecified = True;
  killer = arg;
  return True;
}

static Bool
notifierAction (Display* d, const char* arg)
{
  notifierSpecified = True;
  notifier = arg;
  return True;
}

static Bool
bellAction (Display* d, const char* arg)
{
  bellSpecified = True;
  return getInteger (arg, &bellPercent);
}

static Bool
cornerSizeAction (Display* d, const char* arg)
{
  Bool retVal;
  int tmp;

  if ((retVal = getPositive (arg, &tmp))) /* = intended */
  {
    cornerSize = tmp;
  }

  return retVal;
}

static Bool
cornersAction (Display* d, const char* arg)
{
  int c;

  if (strlen (arg) != 4) return False;
  
  for (c = -1; ++c < 4; )
  {
    switch (arg[c])
    {
      case '0': corners[c] = ca_ignore;    continue;
      case '-': corners[c] = ca_dontLock;  continue;
      case '+': corners[c] = ca_forceLock; continue;
      default:  return False;
    }
  }

  return True;
}

#define TIME_ACTION(name,nameSpecified)                    \
static Bool                                                \
name##Action (Display* d, const char* arg)                 \
{                                                          \
  Bool retVal;                                             \
  int tmp = 0;                                             \
                                                           \
  if ((retVal = getPositive (arg, &tmp))) /* = intended */ \
  {                                                        \
    name = (time_t) tmp;                                   \
  }                                                        \
                                                           \
  nameSpecified = True;                                    \
  return retVal;                                           \
}                                                          \

TIME_ACTION (lockTime     , dummySpecified   )
TIME_ACTION (killTime     , killTimeSpecified)
TIME_ACTION (cornerDelay  , dummySpecified   )
TIME_ACTION (cornerRedelay, redelaySpecified )
TIME_ACTION (notifyMargin , notifyLock       )

#define notifyAction notifyMarginAction

#define MESSAGE_ACTION(name)               \
static Bool                                \
name##Action (Display* d, const char* arg) \
{                                          \
  if (messageToSend) return False;         \
  messageToSend = msg_##name;              \
  return True;                             \
}

MESSAGE_ACTION (disable  )
MESSAGE_ACTION (enable   )
MESSAGE_ACTION (toggle   )
MESSAGE_ACTION (exit     )
MESSAGE_ACTION (lockNow  )
MESSAGE_ACTION (unlockNow)
MESSAGE_ACTION (restart  )

#define BOOL_ACTION(name)                  \
static Bool                                \
name##Action (Display* d, const char* arg) \
{                                          \
  return name = True;                      \
}

BOOL_ACTION (secure    )
BOOL_ACTION (resetSaver)
BOOL_ACTION (noCloseOut)
BOOL_ACTION (noCloseErr)
BOOL_ACTION (detectSleep)

static Bool
noCloseAction (Display* d, const char* arg)
{
  (void) noCloseOutAction (d, arg);
  (void) noCloseErrAction (d, arg);
  return True;
}

/*
 *  Option checking logistics.
 */
#ifndef VMS
static void
addExecToCommand (const char** command)
{
 /*
  *  On UNIX systems, we dont want to have an extra shell process 
  *  hanging about all the time while the locker is running, so we
  *  want to insert an `exec' in front of the command. But since
  *  this obviuosly would fail to work correctly if the command
  *  actually consists of multiple ones, we need to look for `;'
  *  characters first. We can only err on the safe side here...
  */
  if (!strchr (*command, ';'))
  {
    char* tmp;
    (void) sprintf (tmp = newArray (char, strlen (*command) + 6),
		    "exec %s", *command);
    *command = tmp;
  }
}
#endif /* !VMS */

static void 
lockTimeChecker (Display* d)
{
  if (lockTime < MIN_LOCK_MINS)
  {
    error1 ("Setting lock time to minimum value of %ld minute(s).\n",
            (long) (lockTime = MIN_LOCK_MINS));
  }
  else if (lockTime > MAX_LOCK_MINS)
  {
    error1 ("Setting lock time to maximum value of %ld minute(s).\n",
            (long) (lockTime = MAX_LOCK_MINS));
  }

  lockTime *= 60; /* convert to seconds */
}

static void
killTimeChecker (Display* d)
{
  if (killTimeSpecified && !killerSpecified)
  {
    error0 ("Using -killtime without -killer makes no sense.\n");
    return;
  }
 
  if (killTime < MIN_KILL_MINS)
  {
    error1 ("Setting kill time to minimum value of %ld minute(s).\n",
            (long) (killTime = MIN_KILL_MINS));
  }
  else if (killTime > MAX_KILL_MINS)
  {
    error1 ("Setting kill time to maximum value of %ld minute(s).\n",
            (long) (killTime = MAX_KILL_MINS));
  }

  killTime *= 60; /* convert to seconds */
}

static void
lockerChecker (Display* d)
{
#ifndef VMS
  addExecToCommand (&locker);
#else /* VMS */
 /*
  *  Translate things to something that VMS knows how to handle.
  */
  lockerDescr.dsc$w_length = (unsigned short) strlen (locker);
  lockerDescr.dsc$b_class = DSC$K_CLASS_S;
  lockerDescr.dsc$b_dtype = DSC$K_DTYPE_T;
  lockerDescr.dsc$a_pointer = (char*) locker;
#endif /* VMS */
}

static void
nowLockerChecker (Display* d)
{
#ifndef VMS
  addExecToCommand (&nowLocker);
#else /* VMS */
 /*
  *  Translate things to something that VMS knows how to handle.
  */
  nowLockerDescr.dsc$w_length = (unsigned short) strlen (now_locker);
  nowLockerDescr.dsc$b_class = DSC$K_CLASS_S;
  nowLockerDescr.dsc$b_dtype = DSC$K_DTYPE_T;
  nowLockerDescr.dsc$a_pointer = (char*) nowLocker;
#endif /* VMS */
}

static void
notifierChecker (Display* d)
{
  if (strcmp (notifier, ""))
  {
    if (!notifyLock)
    {
      error0 ("Using -notifier without -notify makes no sense.\n");
    }
#ifndef VMS
    else
    {
     /*
      *  Add an `&' to the notifier command, so that it always gets 
      *  run as a background process and things will work out properly 
      *  later. The rationale behind this hack is explained elsewhere.
      */
      char* tmp;
      (void) sprintf (tmp = newArray (char, strlen (notifier) + 3),
		      "%s &", notifier);
      notifier = tmp;
    }
#endif /* !VMS */
  }
}

static void
killerChecker (Display* d)
{
#ifndef VMS
  if (strcmp (killer, ""))
  {
   /*
    *  Add an `&' to the killer command, so that it always gets 
    *  run as a background process and things will work out properly 
    *  later. The rationale behind this hack is explained elsewhere.
    */
    char* tmp;
    (void) sprintf (tmp = newArray (char, strlen (killer) + 3),
		    "%s &", killer);
    killer = tmp;
  }
#endif /* !VMS */
}

static void
notifyChecker (Display* d)
{
  if (   notifyLock
      && (   corners[0] == ca_forceLock  
	  || corners[1] == ca_forceLock
	  || corners[2] == ca_forceLock
	  || corners[3] == ca_forceLock))
  {
    time_t minDelay = MIN (cornerDelay, cornerRedelay);

    if (notifyMargin > minDelay)
    {
      error1 ("Notification time reset to %ld second(s).\n",
              (long) (notifyMargin = minDelay));
    }

    if (notifyMargin > lockTime / 2)
    {
      error1 ("Notification time reset to %ld seconds.\n",
              (long) (notifyMargin = lockTime / 2));
    }
  }
}

static void
bellChecker (Display* d)
{
  if (bellSpecified)
  {
    if (!notifyLock)
    {
      error0 ("Using -bell without -notify makes no sense.\n");
      bellPercent = 0;
    }
    else if (notifierSpecified)
    {
      error0 ("Using both -bell and -notifier makes no sense.\n");
      bellPercent = 0;
    }
  }

  if (   bellPercent < -100
      || bellPercent >  100)
  {
    error1 ("Bell percentage reset to %d%%.\n",
            bellPercent = BELL_PERCENT);
  }
}

static void
cornerSizeChecker (Display* d)
{
  int      s;
  Screen*  scr;
  int      maxCornerSize;

  for (maxCornerSize = 32000, s = -1; ++s < ScreenCount (d); )
  {
    scr = ScreenOfDisplay (d, s);

    if (   maxCornerSize > WidthOfScreen (scr)  / 4
        || maxCornerSize > HeightOfScreen (scr) / 4)
    {
      maxCornerSize = MIN (WidthOfScreen (scr), HeightOfScreen (scr)) / 4;
    }
  }

  if (cornerSize > maxCornerSize)
  {
    error1 ("Corner size reset to %d pixels.\n",
            cornerSize = maxCornerSize);
  }
}

static void
cornerReDelayChecker (Display* d)
{
  if (!redelaySpecified)
  {
    cornerRedelay = cornerDelay;
  }
}

/*
 *  The central option table.
 */
typedef Bool (*optAction)  (Display*, const char*);
typedef void (*optChecker) (Display*);

static struct
{
  const char*   name;    /* as it says              */
  XrmOptionKind kind;    /* as it says              */
  caddr_t       value;   /* only for XrmOptionNoArg */
  optAction     action;  /* as it says              */
  optChecker    checker; /* as it says              */
} options[] = 
{
  {"help"              , XrmoptionNoArg , (caddr_t) "",
    helpAction         , (optChecker) 0            },
  {"version"           , XrmoptionNoArg , (caddr_t) "",
    versionAction      , (optChecker) 0            },
  {"locker"            , XrmoptionSepArg, (caddr_t) 0 ,
    lockerAction       , lockerChecker             },
  {"nowlocker"         , XrmoptionSepArg, (caddr_t) 0 ,
    nowLockerAction    , nowLockerChecker          },
  {"killer"            , XrmoptionSepArg, (caddr_t) 0 ,
    killerAction       , killerChecker             },
  {"notifier"          , XrmoptionSepArg, (caddr_t) 0 ,
    notifierAction     , notifierChecker           },
  {"corners"           , XrmoptionSepArg, (caddr_t) 0 ,
    cornersAction      , (optChecker) 0            },
  {"cornersize"        , XrmoptionSepArg, (caddr_t) 0 ,
    cornerSizeAction   , cornerSizeChecker         },
  {"cornerdelay"       , XrmoptionSepArg, (caddr_t) 0 ,
    cornerDelayAction  , (optChecker) 0            },
  {"cornerredelay"     , XrmoptionSepArg, (caddr_t) 0 ,
    cornerRedelayAction, cornerReDelayChecker      },
  {"killtime"          , XrmoptionSepArg, (caddr_t) 0 ,
    killTimeAction     , killTimeChecker           },
  {"time"              , XrmoptionSepArg, (caddr_t) 0 ,
    lockTimeAction     , lockTimeChecker           },
  {"notify"            , XrmoptionSepArg, (caddr_t) 0 ,
    notifyAction       , notifyChecker             },
  {"bell"              , XrmoptionSepArg, (caddr_t) 0 ,
    bellAction         , bellChecker               },
  {"secure"            , XrmoptionNoArg , (caddr_t) "",
    secureAction       , (optChecker) 0            },
  {"enable"            , XrmoptionNoArg , (caddr_t) "",
    enableAction       , (optChecker) 0            },
  {"disable"           , XrmoptionNoArg , (caddr_t) "",
    disableAction      , (optChecker) 0            },
  {"toggle"            , XrmoptionNoArg , (caddr_t) "",
    toggleAction       , (optChecker) 0            },
  {"exit"              , XrmoptionNoArg , (caddr_t) "",
    exitAction         , (optChecker) 0            },
  {"locknow"           , XrmoptionNoArg , (caddr_t) "",
    lockNowAction      , (optChecker) 0            },
  {"unlocknow"         , XrmoptionNoArg , (caddr_t) "",
    unlockNowAction    , (optChecker) 0            },
  {"restart"           , XrmoptionNoArg , (caddr_t) "",
    restartAction      , (optChecker) 0            },
  {"resetsaver"        , XrmoptionNoArg , (caddr_t) "",
    resetSaverAction   , (optChecker) 0            },
  {"noclose"           , XrmoptionNoArg , (caddr_t) "",
    noCloseAction      , (optChecker) 0            },
  {"nocloseout"        , XrmoptionNoArg , (caddr_t) "",
    noCloseOutAction   , (optChecker) 0            },
  {"nocloseerr"        , XrmoptionNoArg , (caddr_t) "",
    noCloseErrAction   , (optChecker) 0            },
  {"detectsleep"       , XrmoptionNoArg , (caddr_t) "",
    detectSleepAction  , (optChecker) 0            },
}; /* as it says, the order is important! */

/*
 *  Have a guess...
 */
static void
usage (int exitCode)
{
 /*
  *  The relative overhead is enormous here, but who cares.
  *  I'm a perfectionist and usage() doesn't return anyway.
  */
  char*  blanks; /* string full of blanks */
  size_t len;    /* number of blanks      */
  len = strlen ("Usage :  ") + strlen (progName);
  (void) memset (blanks = newArray (char, len + 1), ' ', len);
  blanks[len] = '\0';

 /*
  *  This is where the actual work gets done...
  */
  error0 ("\n");
  error1 ("Usage : %s ", progName);
  error0 ("[-help][-version][-time mins][-locker locker]\n");
  error1 ("%s[-killtime mins][-killer killer]\n", blanks);
  error1 ("%s[-notify margin][-notifier notifier][-bell percent]\n", blanks);
  error1 ("%s[-corners xxxx][-cornerdelay secs]\n", blanks);
  error1 ("%s[-cornerredelay secs][-cornersize pixels]\n", blanks);
  error1 ("%s[-nocloseout][-nocloseerr][-noclose]\n", blanks);
  error1 ("%s[-enable][-disable][-toggle][-exit][-secure]\n", blanks);
  error1 ("%s[-locknow][-unlocknow][-nowlocker locker]\n", blanks);
  error1 ("%s[-restart][-resetsaver][-detectsleep]\n", blanks);

  error0 ("\n");
  error0 (" -help               : print this message and exit.\n");
  error0 (" -version            : print version number and exit.\n");
  error0 (" -time mins          : time before locking the screen");
  error2 (" [%d <= mins <= %d].\n", MIN_LOCK_MINS, MAX_LOCK_MINS);
  error0 (" -locker locker      : program used to lock.\n");
  error0 (" -nowlocker locker   : program used to lock immediately.\n");
  error0 (" -killtime killmins  : time after locking at which to run\n");
  error2 ("                       the killer [%d <= killmins <= %d].\n",
                                  MIN_KILL_MINS, MAX_KILL_MINS);
  error0 (" -killer killer      : program used to kill.\n");
  error0 (" -notify margin      : notify this many seconds before locking.\n");
  error0 (" -notifier notifier  : program used to notify.\n");
  error0 (" -bell percent       : loudness of notification beeps.\n");
  error0 (" -corners xxxx       : corner actions (0, +, -) in this order:\n");
  error0 ("                       topleft topright bottomleft bottomright\n");
  error0 (" -cornerdelay secs   : time to lock screen in a `+' corner.\n");
  error0 (" -cornerredelay secs : time to relock screen in a `+' corner.\n");
  error0 (" -cornersize pixels  : size of corner areas.\n");
  error0 (" -nocloseout         : do not close stdout.\n");
  error0 (" -nocloseerr         : do not close stderr.\n");
  error0 (" -noclose            : close neither stdout nor stderr.\n");
  error0 (" -enable             : enable a running xautolock.\n");
  error0 (" -disable            : disable a running xautolock.\n");
  error0 (" -toggle             : toggle a running xautolock.\n");
  error0 (" -locknow            : tell a running xautolock to lock.\n");
  error0 (" -unlocknow          : tell a running xautolock to unlock.\n");
  error0 (" -restart            : tell a running xautolock to restart.\n");
  error0 (" -exit               : kill a running xautolock.\n");
  error0 (" -secure             : ignore enable, disable, toggle, locknow\n");
  error0 ("                       unlocknow, and restart messages.\n");
  error0 (" -resetsaver         : reset the screensaver when starting "
                                  "the locker.\n");
  error0 (" -detectsleep        : reset timers when awaking from sleep.\n");

  error0 ("\n");
  error0 ("Defaults :\n");

  error0 ("\n");
  error1 ("  time          : %d minutes\n"  , LOCK_MINS   );
  error1 ("  locker        : %s\n"          , LOCKER      );
  error1 ("  nowlocker     : %s\n"          , LOCKER      );
  error1 ("  killtime      : %d minutes\n"  , KILL_MINS   );
  error0 ("  killer        : none\n"                      );
  error0 ("  notify        : don't notify\n"              );
  error0 ("  notifier      : none\n"                      );
  error1 ("  bell          : %d%%\n"        , BELL_PERCENT);
  error0 ("  corners       : 0000\n"                      );
  error1 ("  cornerdelay   : %d seconds\n"  , CORNER_DELAY);
  error1 ("  cornerredelay : %d seconds\n"  , CORNER_DELAY);
  error1 ("  cornersize    : %d pixels\n"   , CORNER_SIZE );

  error0 ("\n");
  error1 ("Version : %s\n", VERSION);

  error0 ("\n");

  exit (exitCode);
}

/*
 *  Public interface to the above lot.
 */
void
processOpts (Display* d, int argc, char* argv[])
{
  int                nofOptions = sizeof (options) / sizeof (options[0]);
                                /* number of supported options   */
  int                j;         /* loop counter                  */
  unsigned           l;         /* temporary storage             */
  unsigned           maxLen;    /* temporary storage             */
  char*              dummy;     /* as it says                    */
  char*              fullname;  /* full resource name            */
  char*              str;       /* temporary storage             */
  XrmValue           value;     /* resource value container      */
  XrmOptionDescList  xoptions;  /* optionslist in Xlib format    */
  XrmDatabase        rescDb = (XrmDatabase) 0;
                                /* resource file database        */
  XrmDatabase        cmdlDb = (XrmDatabase) 0;
                                /* command line options database */

 /*
  *  Collect defaults from various places except the command line into one
  *  resource database, then parse the command line options into an other.
  *  Both databases are not merged, because we want to know where exactly
  *  each resource value came from.
  *
  *  One day I might extend this stuff to fully cover *all* possible
  *  resource value sources, but... One of the problems is that various
  *  pieces of documentation make conflicting claims with respect to the
  *  proper order in which resource value sources should be accessed.
  */
  XrmInitialize ();

  if (XResourceManagerString (d))
  {
    XrmMergeDatabases (XrmGetStringDatabase (XResourceManagerString (d)),
		       &rescDb);
  }
  else if ((str = getenv ("XENVIRONMENT"))) /* = intended */
  {
    XrmMergeDatabases (XrmGetFileDatabase (str), &rescDb);
  }
#if defined (ReadXdefaultsFile) && !defined (VMS)
  else
  {
   /*
    *  In general, the following is a not a good idea. Who is to say that
    *  the user's .Xdefaults file matches the screen being used? And who
    *  says we shouldn't run the file through cpp first? Using which 
    *  options, by the way?
    *
    *  Fortunately, chances are pretty small that anyone will actually run
    *  xautolock remotely. And even if someone does, the same .Xdefaults
    *  file will most likely be involved on both sides of the link. Also,
    *  we only do this of we failed to get our resources from the standard
    *  places. People who have .Xdefaults files that need to be fed to cpp,
    *  will run the appropriate xrdb command on login anyway, such that we 
    *  never get here.
    *  
    *  Having said that, I'm only keeping this stuff around because of the
    *  desire to be backward compatible for those who originally convinced
    *  me to include it, or who since became to depend on it. If I could 
    *  live my life over again, this wouldn't get added in the first place.
    */
    XrmDatabase Xdefaults;
    struct passwd *passwd;
    const char* home;
    char* path;
 
    passwd = getpwuid (getuid ());

    if (passwd)
    {
      home = passwd->pw_dir;
    }
    else
    {
      home = getenv ("HOME");
      if (!home) home = ".";
    }

    path = newArray (char, strlen (home) + strlen ("/.Xdefaults") + 1);
    (void) sprintf (path, "%s/.Xdefaults", home);
    Xdefaults = XrmGetFileDatabase (path);
    if (Xdefaults) XrmMergeDatabases (Xdefaults, &rescDb);

    free (path);
  }
#endif /* ReadXdefaultsFile && !VMS */

  xoptions = newArray (XrmOptionDescRec, nofOptions);

  for (j = -1, maxLen = 0; ++j < nofOptions; )
  {
    l = strlen (options[j].name) + 1;
    maxLen = MAX (maxLen, l);

    (void) sprintf (xoptions[j].option = newArray (char, l + 1),
	            "-%s", options[j].name);
    (void) sprintf (xoptions[j].specifier = newArray (char, l + 1),
                    ".%s", options[j].name);
    xoptions[j].argKind = options[j].kind;
    xoptions[j].value = options[j].value;
  }

  XrmParseCommand (&cmdlDb, xoptions, nofOptions, progName, &argc, argv);

  if (--argc) usage (EXIT_FAILURE);

 /*
  *  Let's be perfect...
  */
  {
    unsigned classLen = strlen (APPLIC_CLASS);
    unsigned progLen  = strlen (progName);

    fullname = newArray (char, MAX (progLen, classLen) + maxLen + 1);
  }

 /*
  *  Call the action functions.
  */
  for (j = -1; ++j < nofOptions; )
  {
    (void) sprintf (fullname, "%s%s", progName, xoptions[j].specifier);

    if (   XrmGetResource (cmdlDb, fullname, DUMMY_RES_CLASS,
                           &dummy, &value)
        == True)
    {
      if (!(*(options[j].action)) (d, value.addr))
      {
	usage (EXIT_FAILURE); 
      }
    }
    else if (   XrmGetResource (rescDb, fullname, DUMMY_RES_CLASS,
                                &dummy, &value)
             == True)
    {
      if (!(*(options[j].action)) (d, value.addr))
      {
        error2 ("Can't interprete \"%s\" for \"%s\", using default.\n", 
                value.addr, fullname);
      }
    }
    else
    {
      (void) sprintf (fullname, "%s%s", APPLIC_CLASS, xoptions[j].specifier);

      if (   (   XrmGetResource (rescDb, fullname, DUMMY_RES_CLASS,
                                 &dummy, &value)
              == True)
          && !(*(options[j].action)) (d, value.addr))
      {
        error2 ("Can't interprete \"%s\" for \"%s\", using default.\n", 
                value.addr, fullname);
      }
    }
  }

 /*
  *  Call the consistency checkers.
  */
  for (j = -1; ++j < nofOptions; )
  {
    if (options[j].checker != (optChecker) NULL)
    {
      (*(options[j].checker)) (d);
    }
  }

 /*
  *  General clean up.
  */
  XrmDestroyDatabase (cmdlDb);
  XrmDestroyDatabase (rescDb);

  for (j = -1; ++j < nofOptions; )
  {
    free (xoptions[j].option);
    free (xoptions[j].specifier);
  }

  free (xoptions);
  free (fullname);
}
