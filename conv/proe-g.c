/*
 *		P R O E - G
 *
 * Code to convert ascii output from Pro/Engineer to BRL-CAD
 * The required output is from the Pro/Develop application proe-brl
 * that must be initiated from the "BRL-CAD" option of Pro/Engineer's
 * "EXPORT" menu.  The Pro/develop application may be obtained via
 * anonymous FTP from ftp.brl.mil or via email from "jra@arl.mil"
 *
 *  Author -
 *	John R. Anderson
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Pacakge" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1994 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */

#ifndef lint
static char RCSid[] = "$Header$";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "wdb.h"
#include "../librt/debug.h"
#include "regex.h"

extern char *optarg;
extern int optind,opterr,optopt;
extern int errno;

#define NAME_LENGTH	79
#define NAMESIZE	16	/* from db.h */

static	struct wmember all_head;
static char *input_file;	/* name of the input file */
static char *brlcad_file;	/* name of output file */
static char ret_name[NAMESIZE]; /* unique name built by Build_unique_name() */
static char forced_name[NAME_LENGTH+1];	/* name specified on command line */
static int stl_format=0;	/* Flag, non-zero indocates raw Stereolithography format input */
static int polysolid=1;		/* Flag for polysolid output rather than NMG's */
static int solid_count=0;	/* count of solids converted */
static struct bn_tol tol;	/* Tolerance structure */
static int id_no=1000;		/* Ident numbers */
static int const_id=-1;		/* Constant ident number (assigned to all regions if non-negative) */
static int mat_code=1;		/* default material code */
static int air_no=1;		/* Air numbers */
static int debug=0;		/* Debug flag */
static int cut_count=0;		/* count of assembly cut HAF solids created */
static int do_regex=0;		/* flag to indicate if 'u' option is in effect */
static int do_simplify=0;	/* flag to try to simplify solids */
static regex_t reg_cmp;		/* compiled regular expression */
static char *proe_usage="%s [-darS] [-i initial_ident] [-I constant_ident] [-m material_code] [-u reg_exp] [-x rt_debug_flag] proe_file.brl output.g\n\
	where proe_file.brl is the output from Pro/Engineer's BRL-CAD EXPORT option\n\
	and output.g is the name of a BRL-CAD database file to receive the conversion.\n\
	The -d option prints additional debugging information.\n\
	The -i option sets the initial region ident number (default is 1000).\n\
	The -I option sets the non-negative ident number that will be assigned to all regions (conflicts with -i).\n\
	The -m option sets the integer material code for all the parts. (default is 1)\n\
	The -u option indicates that portions of object names that match the regular expression\n\
		'reg_exp' should be ignored.\n\
	The -a option creates BRL-CAD 'air' regions from everything in the model.\n\
	The -r option indicates that the model should not be re-oriented or scaled,\n\
		but left in the same orientation as it was in Pro/E.\n\
		This is to allow conversion of parts to be included in\n\
		previously converted Pro/E assemblies.\n\
	The -S option indicates that the input file is raw STL (STereoLithography) format.\n\
	The -x option specifies an RT debug flags (see cad/librt/debug.h).\n";
static char *stl_usage="%s [-da] [-N forced_name] [-i initial_ident] [-I constant_ident] [-m material_code] [-c units_str] [-u reg_exp] [-x rt_debug_flag] input.stl output.g\n\
	where input.stl is a STereoLithography file\n\
	and output.g is the name of a BRL-CAD database file to receive the conversion.\n\
	The -c option specifies the units used in the STL file (units_str may be \"in\", \"ft\",... default is \"mm\"\n\
	The -N option specifies a name to use for the object.\n\
	The -d option prints additional debugging information.\n\
	The -i option sets the initial region ident number (default is 1000).\n\
	The -I option sets the ident number that will be assigned to all regions (conflicts with -i).\n\
	The -m option sets the integer material code for all the parts (default is 1).\n\
	The -u option indicates that portions of object names that match the regular expression\n\
		'reg_exp' should be ignored.\n\
	The -a option creates BRL-CAD 'air' regions from everything in the model.\n\
	The -x option specifies an RT debug flags (see cad/librt/debug.h).\n";
static char *usage;
static FILE *fd_in;		/* input file (from Pro/E) */
static FILE *fd_out;		/* Resulting BRL-CAD file */
static struct bu_ptbl null_parts; /* Table of NULL solids */
static float conv_factor=1.0;	/* conversion factor from model units to mm */
static int top_level=1;		/* flag to catch top level assembly or part */
static mat_t re_orient;		/* rotation matrix to put model in BRL-CAD orientation
				 * (+x towards front +z is up ) */
static int do_air=0;		/* When set, all regions are BRL-CAD "air" regions */
static int do_reorient=1;	/* When set, reorient entire model to BRL-CAD style */
static unsigned int obj_count=0; /* Count of parts converted for "stl-g" conversions */
static fastf_t *bot_verts=NULL;	 /* array of vertices for a bot solid */
static int *bot_faces=NULL;	 /* array of ints (indices into bot_verts array) three per face */
static int bot_vsize=0;		/* current size of the bot_verts array */
static int bot_vcurr=0;		/* current bot vertex */
static int bot_fsize=0;		/* current size of the bot_faces array */
static int bot_fcurr=0;		/* current bot face */

/* Size of blocks of vertices to malloc */
#define	BOT_VBLOCK	128

/* Size of blocks of faces to malloc */
#define BOT_FBLOCK	128

struct render_verts
{
	point_t pt;
	struct vertex *v;
};

struct name_conv_list
{
	char brlcad_name[NAMESIZE];
	char solid_name[NAMESIZE];
	char name[80];
	unsigned int obj;
	int solid_use_no;
	int comb_use_no;
	struct name_conv_list *next;
} *name_root=(struct name_conv_list *)NULL;

struct ptc_plane
{
	double  e1[3], e2[3], e3[3], origin[3];
};

struct ptc_cylinder
{
	double  e1[3], e2[3], e3[3], origin[3];
	double  radius;
};

union ptc_surf
{
	struct ptc_plane plane;
	struct ptc_cylinder cylinder;
};

struct ptc_surf_list
{
	struct bu_list	l;
	int type;
	union ptc_surf	surf;
} *surf_root=(struct ptc_surf_list *)NULL;

/* for type in struct ptc_plane and struct ptc_cylinder */
#define	SURF_PLANE	1
#define	SURF_CYLINDER	2

#define	MAX_LINE_LEN	512

#define	UNKNOWN_TYPE	0
#define	ASSEMBLY_TYPE	1
#define	PART_TYPE	2
#define	CUT_SOLID_TYPE	3

char *
Build_unique_name( name )
char *name;
{
	char suff[NAMESIZE];
	struct name_conv_list *ptr;
	int name_len, suff_len;
	int tries=0;

	name_len = strlen( name );
	strncpy( ret_name, name, NAMESIZE );
	ret_name[NAMESIZE-1] = '\0';
	ptr = name_root;
	while( ptr )
	{
		if( !strncmp( ret_name , ptr->brlcad_name , NAMESIZE ) || !strncmp( ret_name , ptr->solid_name , NAMESIZE ) )
		{
			/* this name already exists, build a new one */
			++tries;
			sprintf( suff, "_%d", tries );
			suff_len = strlen( suff );
			if( suff_len >= NAMESIZE-1 )
			{
				bu_log( "Cannot build unique name for '%s'\n", name );
				bu_log( "Conversion aborted\n" );
				exit( 1 );
			}
			if( name_len + suff_len < NAMESIZE )
			{
				strcpy( ret_name, name );
				strcat( ret_name, suff );
			}
			else
			{
				strncpy( ret_name, name, NAMESIZE );
				sprintf( &ret_name[NAMESIZE-suff_len-1], "%s", suff );
			}

			ptr = name_root;
		}

		ptr = ptr->next;
	}

	return( ret_name );
}

static struct name_conv_list *
Add_new_name( name , obj , type )
char *name;
unsigned int obj;
int type;
{
	struct name_conv_list *ptr;
	char tmp_name[NAMESIZE];
	int suffix_insert;

	if( debug )
		bu_log( "Add_new_name( %s, x%x, %d )\n", name, obj, type );

	if( type != ASSEMBLY_TYPE && type != PART_TYPE && type != CUT_SOLID_TYPE )
	{
		bu_log( "Bad type for name (%s) in Add_new_name\n", name );
		rt_bomb( "Add_new_name\n" );
	}


	/* Add a new name */
	ptr = (struct name_conv_list *)bu_malloc( sizeof( struct name_conv_list ) , "Add_new_name: prev->next" );
	ptr->next = (struct name_conv_list *)NULL;
	strncpy( ptr->name , name, 80 );
	ptr->obj = obj;
	if( do_regex && type != CUT_SOLID_TYPE )
	{
		regmatch_t pmatch;

		if( !regexec( &reg_cmp, ptr->name, 1, &pmatch, 0  ) )
		{
			char *c=ptr->brlcad_name;
			int i=(-1);

			/* got a match */
			while( ptr->name[++i] )
			{
				if( i < pmatch.rm_so || i >= pmatch.rm_eo )
					*c++ = ptr->name[i];
			}
			*c++ = '\0';
		}
		else
			strncpy( ptr->brlcad_name , name , NAMESIZE-2 );
		if( debug )
			bu_log( "\tafter reg_ex, name is %s\n", ptr->brlcad_name );
	}
	else if( type == CUT_SOLID_TYPE )
		ptr->brlcad_name[0] = '\0';
	else
		strncpy( ptr->brlcad_name , name , NAMESIZE-2 );
	ptr->brlcad_name[NAMESIZE-2] = '\0';
	ptr->solid_use_no = 0;
	ptr->comb_use_no = 0;

	if( type != CUT_SOLID_TYPE )
	{
		/* make sure brlcad_name is unique */
		strncpy( ptr->brlcad_name, Build_unique_name( name ), NAMESIZE );
	}

	if( type == ASSEMBLY_TYPE )
	{
		ptr->solid_name[0] = '\0';
		return( ptr );
	}
	else if( type == PART_TYPE )
	{
		strcpy( ptr->solid_name , "s." );
		strncpy( &ptr->solid_name[2] , ptr->brlcad_name , NAMESIZE-4 );
		ptr->solid_name[NAMESIZE-1] = '\0';
	}
	else
	{
		strcpy( ptr->solid_name , "s." );
		strncpy( &ptr->solid_name[2] , name , NAMESIZE-4 );
		ptr->solid_name[NAMESIZE-1] = '\0';
	}

	/* make sure solid name is unique */
	suffix_insert = strlen( ptr->solid_name );
	if( suffix_insert > NAMESIZE - 3 )
		suffix_insert = NAMESIZE - 3;

	strncpy( tmp_name, ptr->solid_name, NAMESIZE );
	strncpy( ptr->solid_name, Build_unique_name( ptr->solid_name ), NAMESIZE );
	return( ptr );
}

static char *
Get_unique_name( name , obj , type )
char *name;
unsigned int obj;
int type;
{
	struct name_conv_list *ptr,*prev;

	if( name_root == (struct name_conv_list *)NULL )
	{
		/* start new list */
		name_root = Add_new_name( name , obj , type );
		ptr = name_root;
	}
	else
	{
		int found=0;

		prev = (struct name_conv_list *)NULL;
		ptr = name_root;
		while( ptr && !found )
		{
			if( obj == ptr->obj )
				found = 1;
			else
			{
				prev = ptr;
				ptr = ptr->next;
			}
		}

		if( !found )
		{
			prev->next = Add_new_name( name , obj , type );
			ptr = prev->next;
		}
	}

	return( ptr->brlcad_name );
}

static char *
Get_solid_name( name , obj )
char *name;
unsigned int obj;
{
	struct name_conv_list *ptr;

	ptr = name_root;

	while( ptr && obj != ptr->obj )
		ptr = ptr->next;

	if( !ptr )
		ptr = Add_new_name( name , 0 , PART_TYPE );

	return( ptr->solid_name );
}

static void
Convert_assy( line )
char line[MAX_LINE_LEN];
{
	struct wmember head;
	struct wmember *wmem;
	char line1[MAX_LINE_LEN];
	char name[80];
	unsigned int obj;
	char memb_name[80];
	unsigned int memb_obj;
	char *brlcad_name;
	float mat_col[4];
	float junk;
	int start;
	int i;

	if( rt_g.debug & DEBUG_MEM_FULL )
	{
		bu_log( "Barrier check at start of Convert_assy:\n" );
		if( bu_mem_barriercheck() )
			rt_bomb( "Barrier check failed!!!\n" );
	}

	BU_LIST_INIT( &head.l );

	start = (-1);
	/* skip leading blanks */
	while( isspace( line[++start] ) && line[start] != '\0' );
	if( strncmp( &line[start] , "assembly" , 8 ) && strncmp( &line[start] , "ASSEMBLY" , 8 ) )
	{
		bu_log( "PROE-G: Convert_assy called for non-assembly:\n%s\n" , line );
		return;
	}

	/* skip blanks before name */
	start += 7;
	while( isspace( line[++start] ) && line[start] != '\0' );

	/* get name */
	i = (-1);
	start--;
	while( !isspace( line[++start] ) && line[start] != '\0' && line[start] != '\n' )
		name[++i] = line[start];
	name[++i] = '\0';

	/* get object pointer */
	sscanf( &line[start] , "%x %f" , &obj, &junk );

	bu_log( "Converting Assembly: %s\n" , name );

	if( debug )
		bu_log( "Convert_assy: %s x%x\n" , name , obj );

	while( fgets( line1, MAX_LINE_LEN, fd_in ) )
	{
		/* skip leading blanks */
		start = (-1);
		while( isspace( line1[++start] ) && line[start] != '\0' );

		if( !strncmp( &line1[start] , "endassembly" , 11 ) || !strncmp( &line1[start] , "ENDASSEMBLY" , 11 ) )
		{

			brlcad_name = Get_unique_name( name , obj , ASSEMBLY_TYPE );
			if( debug )
			{
				struct wmember *wp;

				bu_log( "\tmake assembly ( %s)\n" , brlcad_name );
				for( BU_LIST_FOR( wp, wmember, &head.l ) )
					bu_log( "\t%c %s\n", wp->wm_op, wp->wm_name );
			}
			else
				bu_log( "\tUsing name: %s\n", brlcad_name );

			mk_lcomb( fd_out , brlcad_name , &head , 0 ,
			(char *)NULL , (char *)NULL , (unsigned char *)NULL , 0 );
			break;
		}
		else if( !strncmp( &line1[start] , "member" , 6 ) || !strncmp( &line1[start] , "MEMBER" , 6 ) )
		{
			start += 5;
			while( isspace( line1[++start] ) && line1[start] != '\0' );
			i = (-1);
			start--;
			while( !isspace( line1[++start] ) && line1[start] != '\0' && line1[start] != '\n' )
				memb_name[++i] = line1[start];
			memb_name[++i] = '\0';


			sscanf( &line1[start] , "%x" , &memb_obj );

			brlcad_name = Get_unique_name( memb_name , memb_obj , PART_TYPE );
			if( debug )
				bu_log( "\tmember (%s)\n" , brlcad_name );
			wmem = mk_addmember( brlcad_name , &head , WMOP_UNION );
		}
		else if( !strncmp( &line1[start] , "matrix" , 6 ) || !strncmp( &line1[start] , "MATRIX" , 6 ) )
		{
			int i,j;
			double scale,inv_scale;

			for( j=0 ; j<4 ; j++ )
			{
				fgets( line1, MAX_LINE_LEN, fd_in );
				sscanf( line1 , "%f %f %f %f" , &mat_col[0] , &mat_col[1] , &mat_col[2] , &mat_col[3] );
				for( i=0 ; i<4 ; i++ )
					wmem->wm_mat[4*i+j] = mat_col[i];
			}

			/* convert this matrix to seperate scale factor into element #15 */
/*			scale = MAGNITUDE( &wmem->wm_mat[0] ); */
			scale = pow( bn_mat_det3( wmem->wm_mat ), 1.0/3.0 );
			if( debug )
			{
				bn_mat_print( brlcad_name, wmem->wm_mat );
				bu_log( "\tscale = %g, conv_factor = %g\n", scale, conv_factor );
			}
			if( scale != 1.0 )
			{
				inv_scale = 1.0/scale;
				for( j=0 ; j<3 ; j++ )
					HSCALE( &wmem->wm_mat[j*4], &wmem->wm_mat[j*4], inv_scale )

				if( top_level)
					wmem->wm_mat[15] *= (inv_scale/conv_factor);
				else
					wmem->wm_mat[15] *= inv_scale;
			}
			else if( top_level )
				wmem->wm_mat[15] /= conv_factor;

			if( top_level && do_reorient )
			{
				/* apply re_orient transformation here */
				if( debug )
				{
					bu_log( "Applying re-orient matrix to member %s\n", brlcad_name );
					bn_mat_print( "re-orient matrix", re_orient );
				}
				bn_mat_mul2( re_orient, wmem->wm_mat );
			}
			if( debug )
				bn_mat_print( "final matrix", wmem->wm_mat );
		}
		else
		{
			bu_log( "Unrecognized line in assembly (%s)\n%s\n" , name , line1 );
		}
	}

	if( rt_g.debug & DEBUG_MEM_FULL )
	{
		bu_log( "Barrier check at end of Convet_assy:\n" );
		if( bu_mem_barriercheck() )
			rt_bomb( "Barrier check failed!!!\n" );
	}

	top_level = 0;

}

static void
do_modifiers( line1, start, head, name, min, max )
char *line1;
int *start;
struct wmember *head;
char *name;
point_t min, max;
{
	struct wmember *wmem;
	int i;

	while( strncmp( &line1[*start], "endmodifiers", 12 ) && strncmp( &line1[*start], "ENDMODIFIERS", 12 ) )
	{
		if( !strncmp( &line1[*start], "plane", 5 ) || !strncmp( &line1[*start], "PLANE", 5 ) )
		{
			struct name_conv_list *ptr;
			char haf_name[NAMESIZE+1];
			fastf_t dist;
			fastf_t tmp_dist;
			point_t origin;
			plane_t plane;
			vect_t e1,e2;
			double u_min,u_max,v_min,v_max;
			double x,y,z;
			int orient;
			point_t arb_pt[8];
			point_t rpp_corner;

			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf %lf", &x, &y, &z );
			VSET( origin, x, y, z );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf %lf", &x, &y, &z );
			VSET( e1, x, y, z );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf %lf", &x, &y, &z );
			VSET( e2, x, y, z );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf %lf", &x, &y, &z );
			VSET( plane, x, y, z );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf", &u_min, &v_min );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%lf %lf", &u_max, &v_max );
			fgets( line1, MAX_LINE_LEN, fd_in );
			sscanf( line1, "%d", &orient );

			plane[H] = VDOT( plane, origin );

			VJOIN2( arb_pt[0], origin, u_min, e1, v_min, e2 );
			VJOIN2( arb_pt[1], origin, u_max, e1, v_min, e2 );
			VJOIN2( arb_pt[2], origin, u_max, e1, v_max, e2 );
			VJOIN2( arb_pt[3], origin, u_min, e1, v_max, e2 );

			/* find max distance to corner of enclosing RPP */
			dist = 0.0;
			VSET( rpp_corner, min[X], min[Y], min[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, min[X], min[Y], max[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, min[X], max[Y], min[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, min[X], max[Y], max[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, max[X], min[Y], min[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, max[X], min[Y], max[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, max[X], max[Y], min[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			VSET( rpp_corner, max[X], max[Y], max[Z] );
			tmp_dist = DIST_PT_PLANE( rpp_corner, plane ) * (fastf_t)orient;
			if( tmp_dist > dist )
				dist = tmp_dist;

			for( i=0 ; i<4 ; i++ )
			{
				VJOIN1( arb_pt[i+4], arb_pt[i], dist*(fastf_t)orient, plane );
			}

			if( top_level )
			{
				for( i=0 ; i<8 ; i++ )
					VSCALE( arb_pt[i], arb_pt[i], conv_factor )
			}

			cut_count++;

			sprintf( haf_name, "cut.%d", cut_count );
			ptr = Add_new_name( haf_name, 0, CUT_SOLID_TYPE );
			if( mk_arb8( fd_out, ptr->solid_name, (fastf_t *)arb_pt ) )
				bu_log( "Failed to create ARB8 solid for Assembly cut in part %s\n", name );
			else
			{
				/* Add this cut to the region */
				wmem = mk_addmember( ptr->solid_name, head,
						WMOP_SUBTRACT );

				if( top_level && do_reorient )
				{
					/* apply re_orient transformation here */
					if( debug )
					{
						bu_log( "Applying re-orient matrix to solid %s\n", ptr->solid_name );
						bn_mat_print( "re-orient matrix", re_orient );
					}
					bn_mat_mul2( re_orient, wmem->wm_mat );
				}
				
			}
		}
		fgets( line1, MAX_LINE_LEN, fd_in );
		(*start) = (-1);
		while( isspace( line1[++(*start)] ) );
	}
}

int
Add_vert( x, y, z )
fastf_t x, y, z;
{
	int i;
	fastf_t *v;
	point_t new_v;
	vect_t diff;
	fastf_t dist_sq;

	VSET( new_v, x, y, z );

	/* first search for this vertex in list */
	for( i=0 ; i<bot_vcurr ; i++ )
	{
		v = &bot_verts[i*3];
		VSUB2( diff, v, new_v );
		dist_sq = MAGSQ( diff );
		if( dist_sq <= tol.dist_sq )
			return( i );
	}

	/* didn't find it, so add a new vertex to the list */
	if( !bot_verts )
	{
		bot_verts = (fastf_t *)bu_malloc( 3 * BOT_VBLOCK * sizeof( fastf_t ), "bot_verts" );
		bot_vsize = BOT_VBLOCK;
		bot_vcurr = 0;
	}
	else if( bot_vcurr >= bot_vsize )
	{
		/* increase size of vertex array */
		bot_vsize += BOT_VBLOCK;
		bot_verts = (fastf_t *)bu_realloc( (void *)bot_verts, bot_vsize * 3 * sizeof( fastf_t ), "bot_verts increase" );
	}

	VMOVE( &bot_verts[bot_vcurr * 3], new_v );
	return( bot_vcurr++ );
}

void
Add_face( face )
int face[3];
{
	if( !bot_faces )
	{
		bot_faces = (int *)bu_malloc( 3 * BOT_FBLOCK * sizeof( int ), "bot_faces" );
		bot_fsize = BOT_FBLOCK;
		bot_fcurr = 0;
	}
	else if( bot_fcurr >= bot_fsize )
	{
		bot_fsize += BOT_FBLOCK;
		bot_faces = (int *)bu_realloc( (void *)bot_faces, 3 * bot_fsize * sizeof( int ), "bot_faces increase" );
	}

	VMOVE( &bot_faces[3*bot_fcurr], face );
	bot_fcurr++;
}

static void
Convert_part( line )
char line[MAX_LINE_LEN];
{
	char line1[MAX_LINE_LEN];
	char name[NAME_LENGTH + 1];
	unsigned int obj=0;
	char *solid_name;
	int tmp_count;
	int start;
	int i;
	int face_count=0;
	int degenerate_count=0;
	int small_count=0;
	float colr[3]={0.5, 0.5, 0.5};
	unsigned char color[3]={ 128, 128, 128 };
	char *brlcad_name;
	struct wmember head;
	struct wmember *wmem;
	vect_t normal;
	int solid_in_region=0;
	int solid_is_written=0;
	point_t part_max,part_min;	/* Part RPP */

	if( rt_g.debug & DEBUG_MEM_FULL )
		bu_prmem( "At start of Conv_prt():\n" );

	if( rt_g.debug & DEBUG_MEM_FULL )
	{
		bu_log( "Barrier check at start of Convet_part:\n" );
		if( bu_mem_barriercheck() )
			rt_bomb( "Barrier check failed!!!\n" );
	}


	bot_vcurr = 0;
	bot_fcurr = 0;
	BU_LIST_INIT( &head.l );
	VSETALL( part_min, MAX_FASTF );
	VSETALL( part_max, -MAX_FASTF );

	start = (-1);
	/* skip leading blanks */
	while( isspace( line[++start] ) && line[start] != '\0' );
	if( strncmp( &line[start] , "solid" , 5 ) && strncmp( &line[start] , "SOLID" , 5 ) )
	{
		bu_log( "Convert_part: Called for non-part\n%s\n" , line );
		return;
	}

	/* skip blanks before name */
	start += 4;
	while( isspace( line[++start] ) && line[start] != '\0' );

	if( line[start] != '\0' )
	{
		/* get name */
		i = (-1);
		start--;
		while( !isspace( line[++start] ) && line[start] != '\0' && line[start] != '\n' )
			name[++i] = line[start];
		name[++i] = '\0';

		/* get object id */
		sscanf( &line[start] , "%x" , &obj );
	}
	else if( stl_format && forced_name[0] != '\0' )
		strcpy( name, forced_name );
	else if( stl_format ) /* build a name from the file name */
	{
		char tmp_str[512];
		char *ptr;
		int len, suff_len;

		obj_count++;
		obj = obj_count;

		/* copy the file name into our work space */
		strncpy( tmp_str, input_file, 512 );
		tmp_str[511] = '\0';

		/* eliminate a trailing ".stl" */
		len = strlen( tmp_str );
		if( len > 4 )
		{
			if( !strncmp( &tmp_str[len-4], ".stl", 4 ) )
				tmp_str[len-4] = '\0';
		}

		/* skip over all characters prior to the last '/' */
		ptr = strrchr( tmp_str, '/' );
		if( !ptr )
			ptr = tmp_str;
		else
			ptr++;

		/* now copy what is left to the name */
		strncpy( name, ptr, NAMESIZE-1 );
		name[NAMESIZE-1] = '\0';
		sprintf( tmp_str, "_%d", obj_count );
		len = strlen( name );
		suff_len = strlen( tmp_str );
		if( len + suff_len < NAMESIZE-3 )
			strcat( name, tmp_str );
		else
			sprintf( &name[NAMESIZE-suff_len-4], tmp_str );
	}
	else
		strcpy( name, "noname" );

	bu_log( "Converting Part: %s\n" , name );

	if( debug )
		bu_log( "Conv_part %s x%x\n" , name , obj );

	solid_count++;
	solid_name = Get_solid_name( name , obj );

	bu_log( "\tUsing solid name: %s\n" , solid_name );

	if( rt_g.debug & DEBUG_MEM || rt_g.debug & DEBUG_MEM_FULL )
		bu_prmem( "At start of Convert_part()" );

	while( fgets( line1, MAX_LINE_LEN, fd_in ) != NULL )
	{
		start = (-1);
		while( isspace( line1[++start] ) );
		if( !strncmp( &line1[start] , "endsolid" , 8 ) || !strncmp( &line1[start] , "ENDSOLID" , 8 ) )
			break;
		else if( !strncmp( &line1[start] , "color" , 5 ) || !strncmp( &line1[start] , "COLOR" , 5 ) )
		{
			sscanf( &line1[start+5] , "%f%f%f" , &colr[0] , &colr[1] , &colr[2] );
			for( i=0 ; i<3 ; i++ )
				color[i] = (int)(colr[i] * 255.0);
		}
		else if( !strncmp( &line1[start] , "normal" , 6 ) || !strncmp( &line1[start] , "NORMAL" , 6 ) )
		{
			float x,y,z;

			start += 6;
			sscanf( &line1[start] , "%f%f%f" , &x , &y , &z );
			VSET( normal , x , y , z );
		}
		else if( !strncmp( &line1[start] , "facet" , 5 ) || !strncmp( &line1[start] , "FACET" , 5 ) )
		{
			VSET( normal , 0.0 , 0.0 , 0.0 );

			start += 4;
			while( line1[++start] && isspace( line1[start] ) );

			if( line1[start] )
			{
				if( !strncmp( &line1[start] , "normal" , 6 ) || !strncmp( &line1[start] , "NORMAL" , 6 ) )
				{
					float x,y,z;

					start += 6;
					sscanf( &line1[start] , "%f%f%f" , &x , &y , &z );
					VSET( normal , x , y , z );
				}
			}
		}
		else if( !strncmp( &line1[start] , "outer loop" , 10 ) || !strncmp( &line1[start] , "OUTER LOOP" , 10 ) )
		{
			int endloop=0;
			int vert_no=0;
			int tmp_face[3];

			while( !endloop )
			{
				if( fgets( line1, MAX_LINE_LEN, fd_in ) == NULL )
					bu_bomb( "Unexpected EOF while reading a loop in a part!!!\n" );
				
				start = (-1);
				while( isspace( line1[++start] ) );

				if( !strncmp( &line1[start] , "endloop" , 7 ) || !strncmp( &line1[start] , "ENDLOOP" , 7 ) )
					endloop = 1;
				else if ( !strncmp( &line1[start] , "vertex" , 6 ) || !strncmp( &line1[start] , "VERTEX" , 6 ) )
				{
					float x,y,z;

					sscanf( &line1[start+6] , "%f%f%f" , &x , &y , &z );
					if( top_level )
					{
						x *= conv_factor;
						y *= conv_factor;
						z *= conv_factor;
					}

					if( vert_no > 2 )
					{
						int n;

						bu_log( "Non-triangular loop:\n" );
						for( n=0 ; n<3 ; n++ )
							bu_log( "\t( %g %g %g )\n", V3ARGS( &bot_verts[tmp_face[n]] ) );

						bu_log( "\t( %g %g %g )\n", x, y, z );
					}
					tmp_face[vert_no++] = Add_vert( x, y, z );

				}
				else
					bu_log( "Unrecognized line: %s\n", line1 );
			}

			/* check for degenerate faces */
			if( tmp_face[0] == tmp_face[1] )
			{
				degenerate_count++;
				continue;
			}

			if( tmp_face[0] == tmp_face[2] )
			{
				degenerate_count++;
				continue;
			}

			if( tmp_face[1] == tmp_face[2] )
			{
				degenerate_count++;
				continue;
			}

			if( debug )
			{
				int n;

				bu_log( "Making Face:\n" );
				for( n=0 ; n<3; n++ )
					bu_log( "\tvertex #%d: ( %g %g %g )\n", tmp_face[n], V3ARGS( &bot_verts[3*tmp_face[n]] ) );
			}

			Add_face( tmp_face );
			face_count++;
		}
		else if( !strncmp( &line1[start], "modifiers", 9 ) || !strncmp( &line1[start], "MODIFIERS", 9 ) )
		{
			if( face_count )
			{
				wmem = mk_addmember( solid_name , &head , WMOP_UNION );
				if( top_level && do_reorient )
				{
					/* apply re_orient transformation here */
					if( debug )
					{
						bu_log( "Applying re-orient matrix to solid %s\n", solid_name );
						bn_mat_print( "re-orient matrix", re_orient );
					}
					bn_mat_mul2( re_orient, wmem->wm_mat );
				}
				solid_in_region = 1;
			}
			do_modifiers( line1, &start, &head, name, part_min, part_max );
		}
	}

	/* Check if this part has any solid parts */
	if( face_count == 0 )
	{
		char *save_name;

		bu_log( "\t%s has no solid parts, ignoring\n" , name );
		if( degenerate_count )
			bu_log( "\t%d faces were degenerate\n", degenerate_count );
		if( small_count )
			bu_log( "\t%d faces were too small\n", small_count );
		save_name = (char *)bu_malloc( NAMESIZE*sizeof( char ), "save_name" );
		brlcad_name = Get_unique_name( name , obj , PART_TYPE );
		strncpy( save_name, brlcad_name, NAMESIZE );
		bu_ptbl_ins( &null_parts, (long *)save_name );
		return;
	}
	else
	{
		if( degenerate_count )
			bu_log( "\t%d faces were degenerate\n", degenerate_count );
		if( small_count )
			bu_log( "\t%d faces were too small\n", small_count );
	}

	mk_bot( fd_out, solid_name, RT_BOT_SOLID, RT_BOT_CCW, 0, bot_vcurr, bot_fcurr, bot_verts, bot_faces, NULL, NULL );

	if( face_count && !solid_in_region )
	{
		wmem = mk_addmember( solid_name , &head , WMOP_UNION );
		if( top_level && do_reorient )
		{
			/* apply re_orient transformation here */
			if( debug )
			{
				bu_log( "Applying re-orient matrix to solid %s\n", solid_name );
				bn_mat_print( "re-orient matrix", re_orient );
			}
			bn_mat_mul2( re_orient, wmem->wm_mat );
		}
	}
	brlcad_name = Get_unique_name( name , obj , PART_TYPE );

	if( do_air )
	{
		bu_log( "\tMaking air region (%s)\n" , brlcad_name );

		mk_lrcomb( fd_out, brlcad_name, &head, 1, (char *)NULL, (char *)NULL,
		color, 0, air_no, 0, 100, 0 );
		air_no++;
	}
	else
	{
		bu_log( "\tMaking region (%s)\n" , brlcad_name );

		if( const_id >= 0 )
		{
			mk_lrcomb( fd_out, brlcad_name, &head, 1, (char *)NULL, (char *)NULL,
			color, const_id, 0, mat_code, 100, 0 );
			if( stl_format && face_count )
				(void)mk_addmember( brlcad_name, &all_head, WMOP_UNION );
		}
		else
		{
			mk_lrcomb( fd_out, brlcad_name, &head, 1, (char *)NULL, (char *)NULL,
			color, id_no, 0, mat_code, 100, 0 );
			if( stl_format && face_count )
				(void)mk_addmember( brlcad_name, &all_head, WMOP_UNION );
			id_no++;
		}
	}

	if( rt_g.debug & DEBUG_MEM_FULL )
	{
		bu_log( "Barrier check at end of Convert_part:\n" );
		if( bu_mem_barriercheck() )
			rt_bomb( "Barrier check failed!!!\n" );
	}

	top_level = 0;

	return;

empty_model:
	{
		char *save_name;

		bu_log( "\t%s is empty, ignoring\n" , name );
		save_name = (char *)bu_malloc( NAMESIZE*sizeof( char ), "save_name" );
		brlcad_name = Get_unique_name( name , obj , PART_TYPE );
		strncpy( save_name, brlcad_name, NAMESIZE );
		bu_ptbl_ins( &null_parts, (long *)save_name );
		return;
	}

}

static void
Convert_input()
{
	char line[ MAX_LINE_LEN ];

	if( !stl_format )
	{
		if( !fgets( line, MAX_LINE_LEN, fd_in ) )
			return;

		sscanf( line, "%f", &conv_factor );
	}

	if( !do_reorient && !stl_format )
		conv_factor = 1.0;

	while( fgets( line, MAX_LINE_LEN, fd_in ) != NULL )
	{
		if( !strncmp( line , "assembly" , 8 ) || !strncmp( line , "ASSEMBLY" , 8 ) )
			Convert_assy( line );
		else if( !strncmp( line , "solid" , 5 ) || !strncmp( line , "SOLID" , 5 ) )
			Convert_part( line );
		else
			bu_log( "Unrecognized line:\n%s\n" , line );
	}
}

static void
Rm_nulls()
{
	struct db_i *dbip;
	int i;	

	dbip = db_open( brlcad_file, "rw" );
	if( dbip == DBI_NULL )
	{
		bu_log( "Cannot db_open %s\n", brlcad_file );
		bu_log( "References to NULL parts not removed\n" );
		return;
	}

	if( debug || BU_PTBL_END( &null_parts )  )
	{
		bu_log( "Deleting references to the following null parts:\n" );
		for( i=0 ; i<BU_PTBL_END( &null_parts ) ; i++ )
		{
			char *save_name;

			save_name = (char *)BU_PTBL_GET( &null_parts, i );
			bu_log( "\t%s\n" , save_name );
		}
	}

	db_scan(dbip, (int (*)())db_diradd, 1, NULL);
	for( i=0 ; i<RT_DBNHASH ; i++ )
	{
		struct directory *dp;

		for( dp=dbip->dbi_Head[i] ; dp!=DIR_NULL ; dp=dp->d_forw )
		{
			struct rt_tree_array	*tree_list;
			struct rt_db_internal	intern;
			struct rt_comb_internal	*comb;
			int j;
			int node_count,actual_count;
			int changed=0;

			/* skip solids */
			if( dp->d_flags & DIR_SOLID )
				continue;

top:
			if( rt_db_get_internal( &intern, dp, dbip, (matp_t)NULL ) < 1 )
			{
				bu_log( "Cannot get internal form of combination %s\n", dp->d_namep );
				continue;
			}
			comb = (struct rt_comb_internal *)intern.idb_ptr;
			RT_CK_COMB( comb );
			if( comb->tree && db_ck_v4gift_tree( comb->tree ) < 0 )
			{
				db_non_union_push( comb->tree );
				if( db_ck_v4gift_tree( comb->tree ) < 0 )
				{
					bu_log( "Cannot flatten tree (%s) for editing\n", dp->d_namep );
					continue;
				}
			}
			node_count = db_tree_nleaves( comb->tree );
			if( node_count > 0 )
			{
				tree_list = (struct rt_tree_array *)bu_calloc( node_count,
					sizeof( struct rt_tree_array ), "tree list" );
				actual_count = (struct rt_tree_array *)db_flatten_tree( tree_list, comb->tree, OP_UNION ) - tree_list;
				if( actual_count > node_count )  bu_bomb("Rm_nulls() array overflow!");
				if( actual_count < node_count )  bu_log("WARNING Rm_nulls() array underflow! %d < %d", actual_count, node_count);
			}
			else
			{
				tree_list = (struct rt_tree_array *)NULL;
				actual_count = 0;
			}


			for( j=0; j<actual_count; j++ )
			{
				int k;
				int found=0;

				for( k=0 ; k<BU_PTBL_END( &null_parts ) ; k++ )
				{
					char *save_name;

					save_name = (char *)BU_PTBL_GET( &null_parts, k );
					if( !strncmp( save_name, tree_list[j].tl_tree->tr_l.tl_name, NAMESIZE ) )
					{
						found = 1;
						break;
					}
				}
				if( found )
				{
					/* This is a NULL part, delete the reference */
/*					if( debug ) */
						bu_log( "Deleting reference to null part (%s) from combination %s\n",
							tree_list[j].tl_tree->tr_l.tl_name, dp->d_namep );

					db_free_tree( tree_list[j].tl_tree );

					for( k=j+1 ; k<actual_count ; k++ )
						tree_list[k-1] = tree_list[k]; /* struct copy */

					actual_count--;
					j--;
					changed = 1;
				}
			}

			if( changed )
			{
				char name[NAMESIZE+1];
				int flags;

				strncpy( name, dp->d_namep, NAMESIZE );
				flags = dp->d_flags;

				if( actual_count )
					comb->tree = (union tree *)db_mkgift_tree( tree_list, actual_count, (struct db_tree_state *)NULL );
				else
					comb->tree = (union tree *)NULL;

				if( db_delete( dbip, dp ) || db_dirdelete( dbip, dp ) )
				{
					bu_log( "Failed to delete combination (%s)\n", dp->d_namep );
					rt_comb_ifree( &intern );
					continue;
				}
				if( (dp=db_diradd( dbip, name, -1, 0, flags, NULL)) == DIR_NULL )
				{
					bu_log( "Could not add modified '%s' to directory\n", dp->d_namep );
					rt_comb_ifree( &intern );
					continue;
				}

				if( rt_db_put_internal( dp, dbip, &intern ) < 0 )
				{
					bu_log( "Unable to write modified combination '%s' to database\n", dp->d_namep );
					rt_comb_ifree( &intern );
					continue;
				}
			}
			bu_free( (char *)tree_list, "tree_list" );
		}
	}
	db_close( dbip );
}

/*
 *			M A I N
 */
int
main(argc, argv)
int	argc;
char	*argv[];
{
	register int c;

        /* XXX These need to be improved */
        tol.magic = BN_TOL_MAGIC;
        tol.dist = 0.00001;
        tol.dist_sq = tol.dist * tol.dist;
        tol.perp = 1e-6;
        tol.para = 1 - tol.perp;

	bu_ptbl_init( &null_parts, 64, " &null_parts");

	forced_name[0] = '\0';

	if( strstr( argv[0], "stl-g" ) )
	{
		/* this code was called as stl-g */
		stl_format = 1;
		do_reorient = 0;
		conv_factor = 1.0;	/* default */
		usage = stl_usage;
	}
	else
		usage = proe_usage;

	if( argc < 2 )
	{
		bu_log( usage, argv[0]);
		exit(1);
	}

	/* Get command line arguments. */
	while ((c = getopt(argc, argv, "Si:I:m:rsdax:u:N:c:")) != EOF) {
		switch (c) {
		case 'c':	/* convert from units */
			conv_factor = bu_units_conversion( optarg );
			if( conv_factor == 0.0 )
			{
				bu_log( "Illegal units: (%s)\n", optarg );
				bu_bomb( "Illegal units!!\n" );
			}
			else
				bu_log( "Converting units from %s to mm (conversion factor is %g)\n", optarg, conv_factor );
			break;
		case 'N':	/* force a name on this object */
			strncpy( forced_name, optarg, NAME_LENGTH );
			if( strlen( optarg ) > NAME_LENGTH )
				forced_name[NAME_LENGTH] = '\0';
			break;

		case 'S':	/* raw stl_format format */
			stl_format = 1;
			do_reorient = 0;
			break;
		case 'i':
			id_no = atoi( optarg );
			break;
		case  'I':
			const_id = atoi( optarg );
			if( const_id < 0 )
			{
				bu_log( "Illegal value for '-I' option, must be zero or greater!!!\n" );
				bu_log( usage, argv[0] );
				bu_bomb( "Illegal value for option '-I'\n" );
			}
			break;
		case 'm':
			mat_code = atoi( optarg );
			break;
		case 'd':
			debug = 1;
			break;
		case 'x':
			sscanf( optarg, "%x", &rt_g.debug );
			bu_printb( "librt rt_g.debug", rt_g.debug, DEBUG_FORMAT );
			bu_log("\n");
			break;
		case 'u':
			do_regex = 1;
			if( regcomp( &reg_cmp, optarg, REG_BASIC ) )
			{
				bu_log( "Bad regular expression (%s)\n", optarg );
				bu_log( usage, argv[0] );
				exit( 1 );
			}
			break;
		case 'a':
			do_air = 1;
			break;
		case 'r':
			do_reorient = 0;
			break;
		case 's':
			do_simplify = 1;
			break;
		default:
			bu_log( usage, argv[0]);
			exit(1);
			break;
		}
	}

	input_file = argv[optind];
	if( (fd_in=fopen( input_file, "r")) == NULL )
	{
		bu_log( "Cannot open input file (%s)\n" , input_file );
		perror( argv[0] );
		exit( 1 );
	}
	optind++;
	brlcad_file = argv[optind];
	if( (fd_out=fopen( brlcad_file, "w")) == NULL )
	{
		bu_log( "Cannot open BRL-CAD file (%s)\n" , brlcad_file );
		perror( argv[0] );
		exit( 1 );
	}

	if( stl_format )
		mk_id_units( fd_out , "Conversion from Stereolithography format" , "mm" );
	else
		mk_id_units( fd_out , "Conversion from Pro/Engineer" , "in" );

	/* Create re-orient matrix */
	bn_mat_angles( re_orient, 0.0, 90.0, 90.0 );

	BU_LIST_INIT( &all_head.l );

	Convert_input();

	if( stl_format )
	{
		/* make a top level group */
		mk_lcomb( fd_out, "all", &all_head, 0, (char *)NULL, (char *)NULL, (unsigned char *)NULL, 0 );
	}

	fclose( fd_in );
	fclose( fd_out );

	/* Remove references to null parts */
	Rm_nulls();
}
