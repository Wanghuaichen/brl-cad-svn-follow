/*
 *			F B L O C A L . H
 *
 *  Author -
 *	Gary S. Moss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986-2004 by the United States Army.
 *	All rights reserved.
 *
 *  $Header$
 */

#ifndef INCL_FBLOCAL
#define INCL_FBLOCAL

#include "machine.h"

#define Malloc_Bomb( _bytes_ ) \
		fb_log( "\"%s\"(%d) : allocation of %d bytes failed.\n", \
				__FILE__, __LINE__, _bytes_ )

#ifndef _LOCAL_
/* Useful for ADB when debugging, set to nothing */
#define _LOCAL_ static
#endif

#endif /* INCL_FBLOCAL */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
