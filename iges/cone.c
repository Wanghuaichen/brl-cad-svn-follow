/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "./iges_struct.h"
#include "./iges_extern.h"
#include "wdb.h"

cone( entityno )
int entityno;
{ 
	fastf_t		rad1=0.0;
	fastf_t		rad2;
	fastf_t		*inp, *outp;
	point_t		base;		/* center point of base */
	vect_t		height;
	vect_t		hdir;		/* direction in which to grow height */
	vect_t		a, avec;	/* one base radius vector */
	vect_t		b, bvec;	/* another base radius vector */
	vect_t		c, cvec;	/* one nose radius vector */
	vect_t		d, dvec;	/* another nose radius vector */
	fastf_t		scale=0.0;
	fastf_t		x1;
	fastf_t		y1;
	fastf_t		z1;
	fastf_t		x2;
	fastf_t		y2;
	fastf_t		z2;
	int		sol_num;		/* IGES solid type number */
	void		vec_ortho();

	/* Default values */
	x1 = 0.0;
	y1 = 0.0;
	z1 = 0.0;
	x2 = 0.0;
	y2 = 0.0;
	z2 = 1.0;
	rad2 = 0.0;

	/* Acquiring Data */
	if( dir[entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}
	Readrec( dir[entityno]->param );
	Readint( &sol_num , "" );
	Readcnv( &scale , "" );
	Readcnv( &rad1 , "" );
	Readcnv( &rad2 , "" );
	Readcnv( &x1 , "" );
	Readcnv( &y1 , "" );
	Readcnv( &z1 , "" );
	Readcnv( &x2 , "" );
	Readcnv( &y2 , "" );
	Readcnv( &z2 , "" );

	if( scale <= 0.0 || (rad1 <= 0.0 && rad2 <= 0.0) )
	{
		printf( "Illegal parameters for entity D%07d (%s)\n" ,
				dir[entityno]->direct , dir[entityno]->name );
		return(0);
	}


	/*
	 * Making the necessaries. First an id is made for the new entity, then
	 * the x, y, z coordinates for its vertices are converted to a point with
	 * VSET(), and x2, y2, z2 are combined with the scalar height to make a
	 * direction vector.  Now it is necessary to use this information to
	 * construct a, b, c, and d vectors.  These represent the two radius
	 * vectors for the base and the nose respectively.
	 * Finally the libwdb routine that makes an analogous BRL-CAD
	 * solid is called.
	 */

	VSET(base, x1, y1, z1);		/* the center pt of base plate */
	VSET(hdir, x2, y2, z2);
	VUNITIZE(hdir);

	/* Multiply the hdir * scale to obtain height. */

	VSCALE(height, hdir, scale);

	/* Now make the a, b, c, and d vectors. */

	inp = hdir;
	outp = a;
	vec_ortho(outp, inp);
	VUNITIZE(a);
	VCROSS(b, hdir, a);
	VSCALE(avec, a, rad1);
	VSCALE(bvec, b, rad1);
	outp = c;
	vec_ortho(outp, inp);
	VUNITIZE(c);
	VCROSS(d, hdir, c);
	VSCALE(cvec, c, rad2);
	VSCALE(dvec, d, rad2);
		
	mk_tgc(fdout, dir[entityno]->name, base, height, avec, bvec, cvec, dvec);

	return( 1 );
} 
