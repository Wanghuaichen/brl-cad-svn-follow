/*
 *			G - E U C L I D . C
 *
 *  Program to convert a BRL-CAD model (in a .g file) to a Euclid "decoded" facetted model
 *  by calling on the NMG booleans.
 *
 *  Author -
 *	John R. Anderson
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */

#ifndef lint
static char RCSid[] = "$Header$";
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "../librt/debug.h"

RT_EXTERN(union tree *do_region_end, (struct db_tree_state *tsp, struct db_full_path *pathp, union tree *curtree));
RT_EXTERN( struct face *nmg_find_top_face , (struct shell *s , long *flags ));

static char	usage[] = "Usage: %s [-v] [-d] [-xX lvl] [-a abs_tol] [-r rel_tol] [-n norm_tol] [-o out_file] brlcad_db.g object(s)\n";

static int	NMG_debug;		/* saved arg of -X, for longjmp handling */
static int	verbose;
static int	debug_plots;		/* Make debugging plots */
static int	ncpu = 1;		/* Number of processors */
static int	curr_id;		/* Current region ident code */
static int	face_count;		/* Count of faces output for a region id */
static char	*out_file = NULL;	/* Output filename */
static FILE	*fp_out;		/* Output file pointer */
static struct nmg_ptbl		idents;	/* Table of region ident numbers */
static struct db_i		*dbip;
static struct rt_tess_tol	ttol;
static struct rt_tol		tol;
static struct model		*the_model;

static struct db_tree_state	tree_state;	/* includes tol & model */

static int	regions_tried = 0;
static int	regions_converted = 0;
static int	regions_written = 0;

struct facets
{
	struct loopuse *lu;
	struct loopuse *outer_loop;
	fastf_t diag_len;
	int facet_type;
};

#define V3RPP1_IN_RPP2( _lo1 , _hi1 , _lo2 , _hi2 )	( \
	(_lo1)[X] >= (_lo2)[X] && (_hi1)[X] <= (_hi2)[X] && \
	(_lo1)[Y] >= (_lo2)[Y] && (_hi1)[Y] <= (_hi2)[Y] && \
	(_lo1)[Z] >= (_lo2)[Z] && (_hi1)[Z] <= (_hi2)[Z] )

static int
select_region( tsp, pathp, curtree )
register struct db_tree_state	*tsp;
struct db_full_path	*pathp;
union tree		*curtree;
{
	if(verbose )
		rt_log( "select_region: curr_id = %d, tsp->ts_regionid = %d\n" , curr_id , tsp->ts_regionid);

	if( tsp->ts_regionid == curr_id )
		return( 0 );
	else
		return( -1 );
}

static int
get_reg_id( tsp, pathp, curtree )
register struct db_tree_state	*tsp;
struct db_full_path	*pathp;
union tree		*curtree;
{
	if( verbose )
		rt_log( "get_reg_id: Adding id %d to list\n" , tsp->ts_regionid );
	nmg_tbl( &idents , TBL_INS_UNIQUE , (long *)tsp->ts_regionid );
	return( -1 );
}

static union tree *
region_stub( tsp, pathp, curtree )
register struct db_tree_state	*tsp;
struct db_full_path	*pathp;
union tree		*curtree;
{
	rt_log( "region stub called, this shouldn't happen\n" );
	rt_bomb( "region_stub\n" );
}

static union tree *
leaf_stub( tsp, pathp, ep, id )
struct db_tree_state    *tsp;
struct db_full_path     *pathp;
struct rt_external      *ep;
int                     id;
{
	rt_log( "leaf stub called, this shouldn't happen\n" );
	rt_bomb( "leaf_stub\n" );
}

static void
Write_euclid_face( lu , facet_type , regionid , face_number )
CONST struct loopuse *lu;
CONST int facet_type;
CONST int regionid;
CONST int face_number;
{
	struct faceuse *fu;
	struct edgeuse *eu;
	plane_t plane;
	int vertex_count=0;

	NMG_CK_LOOPUSE( lu );

	if( verbose )
		rt_log( "Write_euclid_face: lu=x%x, facet_type=%d, regionid=%d, face_number=%d\n",
			lu,facet_type,regionid,face_number );

	if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
		return;

	if( *lu->up.magic_p != NMG_FACEUSE_MAGIC )
		return;

	for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
		vertex_count++;

	fprintf( fp_out , "%10d%3d     0.    1%5d" , regionid , facet_type , vertex_count );

	vertex_count = 0;
	for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )
	{
		struct vertex *v;

		NMG_CK_EDGEUSE( eu );
		v = eu->vu_p->v_p;
		NMG_CK_VERTEX( v );
		fprintf( fp_out , "%10d%8.1f%8.1f%8.1f" , ++vertex_count , V3ARGS( v->vg_p->coord ) );
	}

	fu = lu->up.fu_p;
	NMG_CK_FACEUSE( fu );
	NMG_GET_FU_PLANE( plane , fu );
	fprintf( fp_out , "%10d%15.5f%15.5f%15.5f%15.5f" , face_number , V4ARGS( plane ) );
}

/*	Routine to write an nmgregion in the Euclid "decoded" format */
static void
Write_euclid_region( r , tsp )
struct nmgregion *r;
struct db_tree_state *tsp;
{
	struct shell *s;
	struct facets *faces;
	int i,j;

	NMG_CK_REGION( r );

	if( verbose )
		rt_log( "Write_euclid_region: r=x%x\n" , r );

	/* if bounds haven't been calculated, do it now */
	if( r->ra_p == NULL )
		nmg_region_a( r , &tol );

	/* Check if region extents are beyond the limitations of the format */
	for( i=X ; i<ELEMENTS_PER_PT ; i++ )
	{
		if( r->ra_p->min_pt[i] < (-999999.0) )
		{
			rt_log( "g-euclid: Coordinates too large (%g) for Euclid format\n" , r->ra_p->min_pt[i] );
			return;
		}
		if( r->ra_p->max_pt[i] > 9999999.0 )
		{
			rt_log( "g-euclid: Coordinates too large (%g) for Euclid format\n" , r->ra_p->max_pt[i] );
			return;
		}
	}

	/* write out each face in the region */
	for( RT_LIST_FOR( s , shell , &r->s_hd ) )
	{
		struct faceuse *fu;

		for( RT_LIST_FOR( fu , faceuse , &s->fu_hd ) )
		{
			struct loopuse *lu;
			int face_type=0;
			int no_of_loops=0;
			int no_of_holes=0;

			if( fu->orientation != OT_SAME )
				continue;

			/* count the loops in this face */
			for( RT_LIST_FOR( lu , loopuse , &fu->lu_hd ) )
			{
				if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
					continue;

				no_of_loops++;
			}

			if( !no_of_loops )
				continue;

			faces = (struct facets *)rt_calloc( no_of_loops , sizeof( struct facets ) , "g-euclid: faces" );

			i = 0;
			for( RT_LIST_FOR( lu , loopuse , &fu->lu_hd ) )
			{
				if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
					continue;

				faces[i].lu = lu;
				if( lu->orientation == OT_OPPOSITE )
					faces[i].facet_type = 1; /* this is a hole */
				else
					faces[i].facet_type = (-1); /* TBD */

				faces[i].outer_loop = NULL;
				i++;
			}

			/* determine type of face 
			 * 0 -> simple facet (no holes )
			 * 1 -> a hole
			 * 2 -> a facet that will have holes
			 */

			for( i=0 ; i<no_of_loops ; i++ )
			{
				if( faces[i].facet_type == 1 )
					no_of_holes++;
			}

			if( !no_of_holes )
			{
				/* no holes, so each loop is a simple face (type 0) */
				for( i=0 ; i<no_of_loops ; i++ )
					faces[i].facet_type = 0;
			}
			else if( no_of_loops == no_of_holes + 1 )
			{
				struct loopuse *outer_lu;

				/* only one outer loop, so find it */
				for( i=0 ; i<no_of_loops ; i++ )
				{
					if( faces[i].facet_type == (-1) )
					{
						outer_lu = faces[i].lu;
						faces[i].facet_type = 2;
						break;
					}
				}

				/* every hole must have this same outer_loop */
				for( i=0 ; i<no_of_loops ; i++ )
				{
					if( faces[i].facet_type == 1 )
						faces[i].outer_loop = outer_lu;
				}
			}
			else
			{
				int loop1,loop2;
				int outer_loop_count;

				/* must determine which holes go with which outer loops */
				for( loop1=0 ; loop1<no_of_loops ; loop1++ )
				{
					if( faces[loop1].facet_type != 1 )
						continue;
rt_log( "Hole Loop:\n" );
nmg_pr_lu_briefly( faces[loop1].lu , (char *)NULL );

					/* loop1 is a hole look for loops containing loop1 */
					outer_loop_count = 0;
					for( loop2=0 ; loop2<no_of_loops ; loop2++ )
					{
						int class;

						if( faces[loop2].facet_type == 1 )
							continue;

						class = nmg_classify_lu_lu( faces[loop1].lu,
								faces[loop2].lu , &tol );
rt_log( "Possible outer loop classified %s (%d):\n" , nmg_class_name( class ) , class );
nmg_pr_lu_briefly( faces[loop2].lu , (char *)NULL );

						if( class != NMG_CLASS_AinB )
							continue;

						/* loop1 is inside loop2, possible outer loop */
						faces[loop2].facet_type = (-2);
						outer_loop_count++;
					}
rt_log( "outer_loop_count = %d\n" , outer_loop_count );

					if( outer_loop_count > 1 )
					{
						/* must choose outer loop from a list of candidates
						 * if any of these candidates contain one of the
						 * other candidates, the outer one can be eliminated
						 * as a possible choice */
						for( loop2=0 ; loop2<no_of_loops ; loop2++ )
						{
							if( faces[loop2].facet_type != (-2) )
								continue;

							for( i=0 ; i<no_of_loops ; i++ )
							{
								if( faces[i].facet_type != (-2) )
									continue;

								if( nmg_classify_lu_lu( faces[i].lu,
									faces[loop2].lu , &tol ) )
								{
									if( faces[i].facet_type != (-2) )
										continue;

									faces[loop2].facet_type = (-1);
									outer_loop_count--;
								}
							}
						}
					}

					if( outer_loop_count != 1 )
					{
						rt_log( "Failed to find outer loop for hole in component %d\n" , tsp->ts_regionid );
						goto outt;
					}

					for( i=0 ; i<no_of_loops ; i++ )
					{
						if( faces[i].facet_type == (-2) )
						{
							faces[i].facet_type = 2;
							faces[loop1].outer_loop = faces[i].lu;
						}
					}
				}

				/* Check */
				for( i=0 ; i<no_of_loops ; i++ )
				{
					if( faces[i].facet_type < 0 )
					{
						/* all holes have been placed 
						 * so these must be simple faces
						 */
						faces[i].facet_type = 0;
					}

					if( faces[i].facet_type == 1 && faces[i].outer_loop == NULL )
					{
						rt_log( "Failed to find outer loop for hole in component %d\n" , tsp->ts_regionid );
						goto outt;
					}
				}
			}
			/* output faces with holes first */
			for( i=0 ; i<no_of_loops ; i++ )
			{
				struct loopuse *outer_loop;

				if( faces[i].facet_type != 2 )
					continue;

				outer_loop = faces[i].lu;
				Write_euclid_face( outer_loop , 2 , tsp->ts_regionid , ++face_count );

				/* output holes for this face */
				for( j=0 ; j<no_of_loops ; j++ )
				{
					if( j == i )
						continue;

					if( faces[j].outer_loop == outer_loop )
						Write_euclid_face( faces[j].lu , 1 , tsp->ts_regionid , ++face_count );
				}
			}
			/* output simple faces */
			for( i=0 ; i<no_of_loops ; i++ )
			{
				if( faces[i].facet_type != 0 )
					continue;
				Write_euclid_face( faces[i].lu , 0 , tsp->ts_regionid , ++face_count );
			}

			rt_free( (char *)faces , "g-euclid: faces" );
		}
	}

	regions_written++;

   outt:
	rt_free( (char *)faces , "g-euclid: faces" );
	return;
}

/*
 *			M A I N
 */
int
main(argc, argv)
int	argc;
char	*argv[];
{
	char		*dot;
	int		i,j,ret;
	register int	c;
	double		percent;

#ifdef BSD
	setlinebuf( stderr );
#else
#	if defined( SYSV ) && !defined( sgi ) && !defined(CRAY2) && \
	 !defined(n16)
		(void) setvbuf( stderr, (char *) NULL, _IOLBF, BUFSIZ );
#	endif
#	if defined(sgi) && defined(mips)
		if( setlinebuf( stderr ) != 0 )
			perror("setlinebuf(stderr)");
#	endif
#endif

#if MEMORY_LEAK_CHECKING
	rt_g.debug |= DEBUG_MEM_FULL;
#endif

	ttol.magic = RT_TESS_TOL_MAGIC;
	/* Defaults, updated by command line options. */
	ttol.abs = 0.0;
	ttol.rel = 0.01;
	ttol.norm = 0.0;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	the_model = nmg_mm();
	tree_state = rt_initial_tree_state;	/* struct copy */
	tree_state.ts_m = &the_model;
	tree_state.ts_tol = &tol;
	tree_state.ts_ttol = &ttol;

	/* Initialize ident table */
	nmg_tbl( &idents , TBL_INIT , NULL );

	/* XXX For visualization purposes, in the debug plot files */
	{
		extern fastf_t	nmg_eue_dist;	/* librt/nmg_plot.c */
		/* XXX This value is specific to the Bradley */
		nmg_eue_dist = 2.0;
	}

	RT_LIST_INIT( &rt_g.rtg_vlfree );	/* for vlist macros */

	/* Get command line arguments. */
	while ((c = getopt(argc, argv, "a:dn:o:r:s:vx:P:X:")) != EOF) {
		switch (c) {
		case 'a':		/* Absolute tolerance. */
			ttol.abs = atof(optarg);
			break;
		case 'd':
			debug_plots = 1;
			break;
		case 'n':		/* Surface normal tolerance. */
			ttol.norm = atof(optarg);
			break;
		case 'o':		/* Output file name */
			out_file = optarg;
			break;
		case 'r':		/* Relative tolerance. */
			ttol.rel = atof(optarg);
			break;
		case 'v':
			verbose++;
			break;
		case 'P':
			ncpu = atoi( optarg );
			rt_g.debug = 1;	/* XXX DEBUG_ALLRAYS -- to get core dumps */
			break;
		case 'x':
			sscanf( optarg, "%x", &rt_g.debug );
			break;
		case 'X':
			sscanf( optarg, "%x", &rt_g.NMG_debug );
			NMG_debug = rt_g.NMG_debug;
			break;
		default:
			fprintf(stderr, usage, argv[0]);
			exit(1);
			break;
		}
	}

	if (optind+1 >= argc) {
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}

	/* Open brl-cad database */
	if ((dbip = db_open( argv[optind] , "r")) == DBI_NULL)
	{
		rt_log( "Cannot open %s\n" , argv[optind] );
		perror(argv[0]);
		exit(1);
	}
	db_scan(dbip, (int (*)())db_diradd, 1);

	if( out_file == NULL )
		fp_out = stdout;
	else
	{
		if ((fp_out = fopen( out_file , "w")) == NULL)
		{
			rt_log( "Cannot open %s\n" , out_file );
			perror( argv[0] );
			return 2;
		}
	}
	optind++;

	fprintf( fp_out , "$03" );

	/* First produce an unordered list of region ident codes */
	ret = db_walk_tree(dbip, argc-optind, (CONST char **)(&argv[optind]),
		1,			/* ncpu */
		&tree_state,
		get_reg_id,			/* put id in table */
		region_stub,
		leaf_stub );


	/* Process regions in ident order */
	curr_id = 0;
	for( i=0 ; i<NMG_TBL_END( &idents ) ; i++ )
	{
		int next_id = 99999999;
		for( j=0 ; j<NMG_TBL_END( &idents ) ; j++ )
		{
			int test_id;

			test_id = (int)NMG_TBL_GET( &idents , j );
			if( test_id > curr_id && test_id < next_id )
				next_id = test_id;
		}
		curr_id = next_id;
		face_count = 0;

		rt_log( "Processing id %d\n" , curr_id );

		/* Walk indicated tree(s).  Each region will be output separately */
		tree_state = rt_initial_tree_state;	/* struct copy */
		tree_state.ts_m = &the_model;
		tree_state.ts_tol = &tol;
		tree_state.ts_ttol = &ttol;

		ret = db_walk_tree(dbip, argc-optind, (CONST char **)(&argv[optind]),
			1,			/* ncpu */
			&tree_state,
			select_region,
			do_region_end,
			nmg_booltree_leaf_tess);	/* in librt/nmg_bool.c */
	}

	percent = 0;
	if( regions_tried > 0 )
		percent = ((double)regions_converted * 100) / regions_tried;
	printf( "Tried %d regions, %d converted successfully.  %g%%\n",
		regions_tried, regions_converted, percent );
	percent = 0;
	if( regions_tried > 0 )
		percent = ((double)regions_written * 100) / regions_tried;
	printf( "                  %d written successfully. %g%%\n",
		regions_written, percent );

	/* Release dynamic storage */
	rt_vlist_cleanup();
	db_close(dbip);

#if MEMORY_LEAK_CHECKING
	rt_prmem("After complete G-EUCLID conversion");
#endif

	return 0;
}

/*
*			D O _ R E G I O N _ E N D
*
*  Called from db_walk_tree().
*
*  This routine must be prepared to run in parallel.
*/
union tree *do_region_end(tsp, pathp, curtree)
register struct db_tree_state	*tsp;
struct db_full_path	*pathp;
union tree		*curtree;
{
	extern FILE		*fp_fig;
	struct nmgregion	*r;
	struct rt_list		vhead;

	if( verbose )
		rt_log( "do_region_end: regionid = %d\n" , tsp->ts_regionid );

	RT_CK_TESS_TOL(tsp->ts_ttol);
	RT_CK_TOL(tsp->ts_tol);
	NMG_CK_MODEL(*tsp->ts_m);

	RT_LIST_INIT(&vhead);

	if (rt_g.debug&DEBUG_TREEWALK || verbose) {
		char	*sofar = db_path_to_string(pathp);
		rt_log("\ndo_region_end(%d %d%%) %s\n",
			regions_tried,
			regions_tried>0 ? (regions_converted * 100) / regions_tried : 0,
			sofar);
		rt_free(sofar, "path string");
	}

	if (curtree->tr_op == OP_NOP)
		return  curtree;

	regions_tried++;
	/* Begin rt_bomb() protection */
	if( ncpu == 1 && RT_SETJUMP )
	{
		/* Error, bail out */
		RT_UNSETJUMP;		/* Relinquish the protection */

		/* Sometimes the NMG library adds debugging bits when
		 * it detects an internal error, before rt_bomb().
		 */
		rt_g.NMG_debug = NMG_debug;	/* restore mode */

		/* Release any intersector 2d tables */
		nmg_isect2d_final_cleanup();

		/* Release the tree memory & input regions */
		db_free_tree(curtree);		/* Does an nmg_kr() */

		/* Get rid of (m)any other intermediate structures */
		if( (*tsp->ts_m)->magic == NMG_MODEL_MAGIC )
			nmg_km(*tsp->ts_m);
		else
			rt_log("WARNING: tsp->ts_m pointer corrupted, ignoring it.\n");

		/* Now, make a new, clean model structure for next pass. */
		*tsp->ts_m = nmg_mm();
	
		goto out;
	}
	if( verbose )
		rt_log( "\tEvaluating region\n" );
	r = nmg_booltree_evaluate(curtree, tsp->ts_tol);	/* librt/nmg_bool.c */
	RT_UNSETJUMP;		/* Relinquish the protection */
	regions_converted++;
	if (r != 0)
	{
		/* Write the region to the EUCLID file */
		Write_euclid_region( r , tsp );

		nmg_kr( r );
	}

	/*
	 *  Dispose of original tree, so that all associated dynamic
	 *  memory is released now, not at the end of all regions.
	 *  A return of TREE_NULL from this routine signals an error,
	 *  so we need to cons up an OP_NOP node to return.
	 */
	db_free_tree(curtree);		/* Does an nmg_kr() */

out:
	GETUNION(curtree, tree);
	curtree->tr_op = OP_NOP;
	return(curtree);
}
