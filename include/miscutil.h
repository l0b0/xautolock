/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It provides 
 *          general purpose logistic support.
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

#ifndef __miscutil_h
#define __miscutil_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif /* MAX */

#define error0(s)       ((void) fprintf (stderr, (s)))
#define error1(s,a1)    ((void) fprintf (stderr, (s), (a1)))
#define error2(s,a1,a2) ((void) fprintf (stderr, (s), (a1), (a2)))

static caddr_t          c_ptr = (caddr_t) &c_ptr; /* This is dirty! */
#define allocate(t,s)   (c_ptr = (caddr_t) malloc ((unsigned) (s)), \
                           (c_ptr == (caddr_t) 0)                   \
                         ? (error0 ("Out of memory.\n"),            \
                            exit (EXIT_FAILURE),                    \
                            /*NOTREACHED*/ (t*) 0                   \
                           )                                        \
                         : (t*) c_ptr                               \
                        )                                           \

#define newObj(tp)      allocate (tp, sizeof (tp))
#define newArray(tp,n)  allocate (tp, sizeof (tp) * (unsigned) (n))

#endif /* __miscutil_h */
