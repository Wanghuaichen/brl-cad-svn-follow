/*
 *			B O O L . C
 *
 * Ray Tracing program, Boolean region evaluator.
 *
 *  Note to developers -
 *	Do not use the hit_point field in these routines, as
 *	for some solid types it has been filled in by the g_xxx_shot()
 *	routine, and for other solid types it may not have been.
 *	In particular, copying a garbage hit_point from a structure which
 *	has not yet been filled in, into a structure which won't be
 *	filled in again, gives wrong results.
 *	Thanks to Keith Bowman for finding this obscure bug.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1985-1996 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./debug.h"

/* Boolean values.  Not easy to change, but defined symbolicly */
#define FALSE	0
#define TRUE	1

RT_EXTERN(void rt_grow_boolstack, (struct resource *resp) );

/*
 *			R T _ W E A V E 0 S E G
 *
 *  If a zero thickness segment abutts another partition,
 *  it will be fused in, later.
 *
 *  If it is free standing, then it will remain as a
 *  zero thickness partition, which probably signals
 *  going through some solid an odd number of times,
 *  or hitting an NMG wire edge or NMG lone vertex.
 */
void
rt_weave0seg( segp, PartHdp, ap )
struct seg		*segp;
struct partition	*PartHdp;
struct application	*ap;
{
	register struct partition *pp;
	struct resource		*res = ap->a_resource;
	struct rt_i		*rtip = ap->a_rt_i;
	FAST fastf_t		tol_dist;

	tol_dist = rtip->rti_tol.dist;

	RT_CK_PT_HD(PartHdp);
	RT_CK_RTI(ap->a_rt_i);
	RT_CK_RESOURCE(res);
	RT_CK_RTI(rtip);

	if(rt_g.debug&DEBUG_PARTITION)  {
		bu_log(
		"rt_boolweave:  Zero thickness seg: %s (%.18e,%.18e) %d,%d\n",
		segp->seg_stp->st_name,
		segp->seg_in.hit_dist,
		segp->seg_out.hit_dist,
		segp->seg_in.hit_surfno,
		segp->seg_out.hit_surfno );
	}

	if( PartHdp->pt_forw == PartHdp )  rt_bomb("rt_weave0seg() with empty partition list\n");

	/* See if this segment ends before start of first partition */
	if( segp->seg_out.hit_dist < PartHdp->pt_forw->pt_inhit->hit_dist )  {
		GET_PT_INIT( rtip, pp, res );
		bu_ptbl_ins_unique( &pp->pt_seglist, (long *)segp );
		pp->pt_inseg = segp;
		pp->pt_inhit = &segp->seg_in;
		pp->pt_outseg = segp;
		pp->pt_outhit = &segp->seg_out;
		APPEND_PT( pp, PartHdp );
		if(rt_g.debug&DEBUG_PARTITION) bu_log("0-len segment ends before start of first partition.\n");
		return;
	}

	/*
	 *  Cases:  seg at start of pt, in middle of pt, at end of pt,
	 *  or past end of pt but before start of next pt.
	 *  XXX For the first 3 cases, we might want to make a new 0-len pt,
	 *  XXX especially as the NMG ray-tracer starts reporting wire hits.
	 */
	for( pp=PartHdp->pt_forw; pp != PartHdp; pp=pp->pt_forw ) {
		if( NEAR_ZERO( segp->seg_in.hit_dist  - pp->pt_inhit->hit_dist, tol_dist ) ||
		    NEAR_ZERO( segp->seg_out.hit_dist - pp->pt_inhit->hit_dist, tol_dist )
		)  {
			if(rt_g.debug&DEBUG_PARTITION) bu_log("0-len segment ends right at start of existing partition.\n");
			return;
		}
		if( NEAR_ZERO( segp->seg_in.hit_dist  - pp->pt_outhit->hit_dist, tol_dist ) ||
		    NEAR_ZERO( segp->seg_out.hit_dist - pp->pt_outhit->hit_dist, tol_dist )
		)  {
			if(rt_g.debug&DEBUG_PARTITION) bu_log("0-len segment ends right at end of existing partition.\n");
			return;
		}
		if( segp->seg_out.hit_dist <= pp->pt_outhit->hit_dist &&
		    segp->seg_in.hit_dist >= pp->pt_inhit->hit_dist )  {
			if(rt_g.debug&DEBUG_PARTITION) bu_log("0-len segment in the middle of existing partition.\n");
			return;
		}
		if( pp->pt_forw == PartHdp ||
		    segp->seg_out.hit_dist < pp->pt_forw->pt_inhit->hit_dist )  {
		    	struct partition	*npp;
			if(rt_g.debug&DEBUG_PARTITION) bu_log("0-len segment after existing partition, but before next partition.\n");
			GET_PT_INIT( rtip, npp, res );
			bu_ptbl_ins_unique( &npp->pt_seglist, (long *)segp );
			npp->pt_inseg = segp;
			npp->pt_inhit = &segp->seg_in;
			npp->pt_outseg = segp;
			npp->pt_outhit = &segp->seg_out;
			APPEND_PT( npp, pp );
			return;
		}
	}
	rt_bomb("rt_weave0seg() fell out of partition loop?\n");
}

/*
 *			R T _ B O O L W E A V E
 *
 *  Weave a chain of segments into an existing set of partitions.
 *  The edge of each partition is an inhit or outhit of some solid (seg).
 *
 *  NOTE:  When the final partitions are completed, it is the users
 *  responsibility to honor the inflip and outflip flags.  They can
 *  not be flipped here because an outflip=1 edge and an inflip=0 edge
 *  following it may in fact be the same edge.  This could be dealt with
 *  by giving the partition struct a COPY of the inhit and outhit rather
 *  than a pointer, but that's more cycles than the neatness is worth.
 *
 * Inputs -
 *	Pointer to first segment in seg chain.
 *	Pointer to head of circular doubly-linked list of
 *	partitions of the original ray.
 *
 * Outputs -
 *	Partitions, queued on doubly-linked list specified.
 *
 * Notes -
 *	It is the responsibility of the CALLER to free the seg chain,
 *	as well as the partition list that we return.
 */
void
rt_boolweave( out_hd, in_hd, PartHdp, ap )
struct seg		*out_hd;
struct seg		*in_hd;
struct partition	*PartHdp;
struct application	*ap;
{
	register struct seg *segp;
	register struct partition *pp;
	struct resource		*res = ap->a_resource;
	struct rt_i		*rtip = ap->a_rt_i;
	FAST fastf_t		diff;
	FAST fastf_t	tol_dist;

	RT_CK_PT_HD(PartHdp);

	tol_dist = rtip->rti_tol.dist;

	RT_CK_RTI(ap->a_rt_i);
	RT_CK_RESOURCE(res);
	RT_CK_RTI(rtip);

	if(rt_g.debug&DEBUG_PARTITION)  {
		rt_pr_partitions( rtip, PartHdp, "-----------------BOOL_WEAVE" );
	}

	while( BU_LIST_NON_EMPTY( &(in_hd->l) ) ) {
		register struct partition	*newpp = PT_NULL;
		register struct seg		*lastseg = RT_SEG_NULL;
		register struct hit		*lasthit = HIT_NULL;
		LOCAL int			lastflip = 0;

		segp = BU_LIST_FIRST( seg, &(in_hd->l) );
		RT_CHECK_SEG(segp);
		RT_CK_HIT(&(segp->seg_in));
		RT_CK_RAY(segp->seg_in.hit_rayp);
		RT_CK_HIT(&(segp->seg_out));
		RT_CK_RAY(segp->seg_out.hit_rayp);
		if(rt_g.debug&DEBUG_PARTITION)  {
			point_t		pt;

			rt_pr_seg(segp);
			rt_pr_hit(" In", &segp->seg_in );
			VJOIN1( pt, ap->a_ray.r_pt, segp->seg_in.hit_dist, ap->a_ray.r_dir );
			/* XXX needs indentation added here */
			VPRINT(" IPoint", pt );

			rt_pr_hit("Out", &segp->seg_out );
			VJOIN1( pt, ap->a_ray.r_pt, segp->seg_out.hit_dist, ap->a_ray.r_dir );
			/* XXX needs indentation added here */
			VPRINT(" OPoint", pt );
		}
		if( segp->seg_stp->st_bit >= rtip->nsolids) rt_bomb("rt_boolweave: st_bit");

		BU_LIST_DEQUEUE( &(segp->l) );
		BU_LIST_INSERT( &(out_hd->l), &(segp->l) );

		/* Make nearly zero be exactly zero */
		if( NEAR_ZERO( segp->seg_in.hit_dist, tol_dist ) )
			segp->seg_in.hit_dist = 0;
		if( NEAR_ZERO( segp->seg_out.hit_dist, tol_dist ) )
			segp->seg_out.hit_dist = 0;

		/* Totally ignore things behind the start position */
		if( segp->seg_out.hit_dist < -10.0 )
			continue;

		if( !(segp->seg_in.hit_dist >= -INFINITY &&
		    segp->seg_out.hit_dist <= INFINITY) )  {
		    	bu_log("rt_boolweave:  Defective %s segment %s (%.18e,%.18e) %d,%d\n",
		    		rt_functab[segp->seg_stp->st_id].ft_name,
				segp->seg_stp->st_name,
				segp->seg_in.hit_dist,
				segp->seg_out.hit_dist,
				segp->seg_in.hit_surfno,
				segp->seg_out.hit_surfno );
			continue;
		}
		if( segp->seg_in.hit_dist > segp->seg_out.hit_dist )  {
		    	bu_log("rt_boolweave:  Inside-out %s segment %s (%.18e,%.18e) %d,%d\n",
		    		rt_functab[segp->seg_stp->st_id].ft_name,
				segp->seg_stp->st_name,
				segp->seg_in.hit_dist,
				segp->seg_out.hit_dist,
				segp->seg_in.hit_surfno,
				segp->seg_out.hit_surfno );
			continue;
		}

		/*
		 * Weave this segment into the existing partitions,
		 * creating new partitions as necessary.
		 */
		if( PartHdp->pt_forw == PartHdp )  {
			/* No partitions yet, simple! */
			GET_PT_INIT( rtip, pp, res );
			bu_ptbl_ins_unique( &pp->pt_seglist, (long *)segp );
			pp->pt_inseg = segp;
			pp->pt_inhit = &segp->seg_in;
			pp->pt_outseg = segp;
			pp->pt_outhit = &segp->seg_out;
			APPEND_PT( pp, PartHdp );
			if(rt_g.debug&DEBUG_PARTITION) bu_log("No partitions yet, segment forms first partition\n");
			goto done_weave;
		}

		if( ap->a_no_booleans )  {
			lastseg = segp;
			lasthit = &segp->seg_in;
			lastflip = 0;
			/* Just sort in ascending in-dist order */
			for( pp=PartHdp->pt_forw; pp != PartHdp; pp=pp->pt_forw ) {
				if( lasthit->hit_dist < pp->pt_inhit->hit_dist )  {
					if(rt_g.debug&DEBUG_PARTITION)  {
						bu_log("Insert nobool seg before next pt\n");
					}
					GET_PT_INIT( rtip, newpp, res );
					bu_ptbl_ins_unique( &newpp->pt_seglist, (long *)segp );
					newpp->pt_inseg = segp;
					newpp->pt_inhit = &segp->seg_in;
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					INSERT_PT( newpp, pp );
					goto done_weave;
				}
			}
			if(rt_g.debug&DEBUG_PARTITION)  {
				bu_log("Append nobool seg at end of list\n");
			}
			GET_PT_INIT( rtip, newpp, res );
			bu_ptbl_ins_unique( &newpp->pt_seglist, (long *)segp );
			newpp->pt_inseg = segp;
			newpp->pt_inhit = &segp->seg_in;
			newpp->pt_outseg = segp;
			newpp->pt_outhit = &segp->seg_out;
			INSERT_PT( newpp, PartHdp );
			goto done_weave;
		}

		/* Check for zero-thickness segment, within tol */
		diff = segp->seg_in.hit_dist - segp->seg_out.hit_dist;
		if( NEAR_ZERO( diff, tol_dist ) )  {
			rt_weave0seg( segp, PartHdp, ap );
			goto done_weave;
		}

		if( segp->seg_in.hit_dist >= PartHdp->pt_back->pt_outhit->hit_dist )  {
			/*
			 * Segment starts exactly at last partition's end,
			 * or beyond last partitions end.  Make new partition.
			 */
			if(rt_g.debug&DEBUG_PARTITION)  {
				bu_log("seg starts beyond last partition end. (%g,%g) Appending new partition\n",
					PartHdp->pt_back->pt_inhit->hit_dist,
					PartHdp->pt_back->pt_outhit->hit_dist);
			}
			GET_PT_INIT( rtip, pp, res );
			bu_ptbl_ins_unique( &pp->pt_seglist, (long *)segp );
			pp->pt_inseg = segp;
			pp->pt_inhit = &segp->seg_in;
			pp->pt_outseg = segp;
			pp->pt_outhit = &segp->seg_out;
			APPEND_PT( pp, PartHdp->pt_back );
			goto done_weave;
		}

		lastseg = segp;
		lasthit = &segp->seg_in;
		lastflip = 0;
		for( pp=PartHdp->pt_forw; pp != PartHdp; pp=pp->pt_forw ) {

			diff = lasthit->hit_dist - pp->pt_outhit->hit_dist;
			if( diff > tol_dist )  {
				/* Seg starts beyond the END of the
				 * current partition.
				 *	PPPP
				 *	        SSSS
				 * Advance to next partition.
				 */
				if(rt_g.debug&DEBUG_PARTITION)  {
					bu_log("seg start beyond partition end, skipping.  (%g,%g)\n",
						pp->pt_inhit->hit_dist,
						pp->pt_outhit->hit_dist);
				}
				continue;
			}
			if(rt_g.debug&DEBUG_PARTITION)  rt_pr_pt(rtip, pp);
			if( diff > -(tol_dist) )  {
				/*
				 * Seg starts almost "precisely" at the
				 * end of the current partition.
				 *	PPPP
				 *	    SSSS
				 * FUSE an exact match of the endpoints,
				 * advance to next partition.
				 */
				lasthit->hit_dist = pp->pt_outhit->hit_dist;
				if(rt_g.debug&DEBUG_PARTITION)  {
					bu_log("seg start fused to partition end, diff=%g\n", diff);
				}
				continue;
			}

			/*
			 *  diff < ~~0
			 *  Seg starts before current partition ends
			 *	PPPPPPPPPPP
			 *	  SSSS...
			 */
			diff = lasthit->hit_dist - pp->pt_inhit->hit_dist;
			if( diff > tol_dist )  {
				/*
				 * lasthit->hit_dist > pp->pt_inhit->hit_dist
				 * pp->pt_inhit->hit_dist < lasthit->hit_dist
				 *
				 *  Segment starts after partition starts,
				 *  but before the end of the partition.
				 *  Note:  pt_seglist will be updated in equal_start.
				 *	PPPPPPPPPPPP
				 *	     SSSS...
				 *	newpp|pp
				 */
				RT_DUP_PT( rtip, newpp, pp, res );
				/* new partition is the span before seg joins partition */
				pp->pt_inseg = segp;
				pp->pt_inhit = &segp->seg_in;
				pp->pt_inflip = 0;
				newpp->pt_outseg = segp;
				newpp->pt_outhit = &segp->seg_in;
				newpp->pt_outflip = 1;
				INSERT_PT( newpp, pp );
				if(rt_g.debug&DEBUG_PARTITION) bu_log("seg starts within p. Split p at seg start, advance.\n");
				goto equal_start;
			}
			if( diff > -(tol_dist) )  {
				/*
				 * Make a subtle but important distinction here.
				 * Even though the two distances are "equal"
				 * within tolerance, they are not exactly
				 * the same.  If the new segment is slightly
				 * closer to the ray origin, then use it's
				 * IN point.
				 * This is an attempt to reduce the deflected
				 * normals sometimes seen along the edges of
				 * e.g. a cylinder unioned with an ARB8,
				 * where the ray hits the top of the cylinder
				 * and the *side* face of the ARB8 rather
				 * than the top face of the ARB8.
				 */
				diff = segp->seg_in.hit_dist - pp->pt_inhit->hit_dist;
				if( NEAR_ZERO(diff, tol_dist) &&
				    diff < 0 )  {
					if(rt_g.debug&DEBUG_PARTITION) bu_log("changing partition start point to segment start point\n");
					pp->pt_inseg = segp;
					pp->pt_inhit = &segp->seg_in;
					pp->pt_inflip = 0;
				}
equal_start:
				if(rt_g.debug&DEBUG_PARTITION) bu_log("equal_start\n");
				/*
				 * Segment and partition start at
				 * (roughly) the same point.
				 * When fuseing 2 points together
				 * i.e., when NEAR_ZERO(diff,tol) is true,
				 * the two points MUST be forced to become
				 * exactly equal!
				 */
				diff = segp->seg_out.hit_dist - pp->pt_outhit->hit_dist;
				if( diff > tol_dist )  {
					/*
					 * Seg & partition start at roughly
					 * the same spot,
					 * seg extends beyond partition end.
					 *	PPPP
					 *	SSSSSSSS
					 *	pp  |  newpp
					 */
					bu_ptbl_ins_unique( &pp->pt_seglist, (long *)segp );
					lasthit = pp->pt_outhit;
					lastseg = pp->pt_outseg;
					lastflip = 1;
					if(rt_g.debug&DEBUG_PARTITION) bu_log("seg spans partition and extends beyond it\n");
					continue;
				}
				if( diff > -(tol_dist) )  {
					/*
					 *  diff ~= 0
					 * Segment and partition start & end
					 * (nearly) together.
					 *	PPPP
					 *	SSSS
					 */
					bu_ptbl_ins_unique( &pp->pt_seglist, (long *)segp );
					if(rt_g.debug&DEBUG_PARTITION) bu_log("same start&end\n");
					goto done_weave;
				} else {
					/*
					 *  diff < ~0
					 *
					 *  Segment + Partition start together,
					 *  segment ends before partition ends.
					 *	PPPPPPPPPP
					 *	SSSSSS
					 *	newpp| pp
					 */
					RT_DUP_PT( rtip, newpp, pp, res );
					/* new partition contains segment */
					bu_ptbl_ins_unique( &newpp->pt_seglist, (long *)segp );
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					newpp->pt_outflip = 0;
					pp->pt_inseg = segp;
					pp->pt_inhit = &segp->seg_out;
					pp->pt_inflip = 1;
					INSERT_PT( newpp, pp );
					if(rt_g.debug&DEBUG_PARTITION) bu_log("start together, seg shorter than partition\n");
					goto done_weave;
				}
				/* NOTREACHED */
			} else {
				/*
				 *  diff < ~~0
				 *
				 * Seg starts before current partition starts,
				 * but after the previous partition ends.
				 *	SSSSSSSS...
				 *	     PPPPP...
				 *	newpp|pp
				 */
				GET_PT_INIT( rtip, newpp, res );
				bu_ptbl_ins_unique( &newpp->pt_seglist, (long *)segp );
				newpp->pt_inseg = lastseg;
				newpp->pt_inhit = lasthit;
				newpp->pt_inflip = lastflip;
				diff = segp->seg_out.hit_dist - pp->pt_inhit->hit_dist;
				if( diff < -(tol_dist) )  {
					/*
					 *  diff < ~0
					 * Seg starts and ends before current
					 * partition, but after previous
					 * partition ends (diff < 0).
					 *		SSSS
					 *	pppp		PPPPP...
					 *		newpp	pp
					 */
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					newpp->pt_outflip = 0;
					INSERT_PT( newpp, pp );
					if(rt_g.debug&DEBUG_PARTITION) bu_log("seg between 2 partitions\n");
					goto done_weave;
				}
				if( diff < tol_dist )  {
					/*
					 *  diff ~= 0
					 *
					 * Seg starts before current
					 * partition starts, and ends at or
					 * near the start of the partition.
					 * (diff == 0).  FUSE the points.
					 *	SSSSSS
					 *	     PPPPP
					 *	newpp|pp
					 * NOTE: only copy hit point, not
					 * normals or norm private stuff.
					 */
					newpp->pt_outseg = segp;
					newpp->pt_outhit = &segp->seg_out;
					newpp->pt_outhit->hit_dist = pp->pt_inhit->hit_dist;
					newpp->pt_outflip = 0;
					INSERT_PT( newpp, pp );
					if(rt_g.debug&DEBUG_PARTITION) bu_log("seg ends at partition start, fuse\n");
					goto done_weave;
				}
				/*
				 *  Seg starts before current partition
				 *  starts, and ends after the start of the
				 *  partition.  (diff > 0).
				 *	SSSSSSSSSS
				 *	      PPPPPPP
				 *	newpp| pp | ...
				 */
				newpp->pt_outseg = pp->pt_inseg;
				newpp->pt_outhit = pp->pt_inhit;
				newpp->pt_outflip = 1;
				lastseg = pp->pt_inseg;
				lasthit = pp->pt_inhit;
				lastflip = newpp->pt_outflip;
				INSERT_PT( newpp, pp );
				if(rt_g.debug&DEBUG_PARTITION) bu_log("insert seg before p start, ends after p ends.  Making new partition for initial portion.\n");
				goto equal_start;
			}
			/* NOTREACHED */
		}

		/*
		 *  Segment has portion which extends beyond the end
		 *  of the last partition.  Tack on the remainder.
		 *  	PPPPP
		 *  	     SSSSS
		 */
		if(rt_g.debug&DEBUG_PARTITION) bu_log("seg extends beyond partition end\n");
		GET_PT_INIT( rtip, newpp, res );
		bu_ptbl_ins_unique( &newpp->pt_seglist, (long *)segp );
		newpp->pt_inseg = lastseg;
		newpp->pt_inhit = lasthit;
		newpp->pt_inflip = lastflip;
		newpp->pt_outseg = segp;
		newpp->pt_outhit = &segp->seg_out;
		APPEND_PT( newpp, PartHdp->pt_back );

done_weave:	; /* Sorry about the goto's, but they give clarity */
		if(rt_g.debug&DEBUG_PARTITION)
			rt_pr_partitions( rtip, PartHdp, "After weave" );
	}
}

/*
 *			R T _ D E F O V E R L A P
 *
 *  Default handler for overlaps in rt_boolfinal().
 *  Returns -
 *	 0	to eliminate partition with overlap entirely
 *	 1	to retain partition in output list, claimed by reg1
 *	 2	to retain partition in output list, claimed by reg2
 *
 *  This is now simply a one-line wrapper that calls _rt_defoverlap(),
 *  requesting verbosity.
 */
int
rt_defoverlap (ap, pp, reg1, reg2, pheadp)

register struct application	*ap;
register struct partition	*pp;
struct region			*reg1;
struct region			*reg2;
struct partition		*pheadp;

{
    return (_rt_defoverlap(ap, pp, reg1, reg2, pheadp, 1));
}

/*
 *			R T _ O V E R L A P _ Q U I E T L Y
 *
 *  Silent version of rt_defoverlap().
 *  Returns -
 *	 0	to eliminate partition with overlap entirely
 *	 1	to retain partition in output list, claimed by reg1
 *	 2	to retain partition in output list, claimed by reg2
 *
 */
int
rt_overlap_quietly (ap, pp, reg1, reg2, pheadp)

register struct application	*ap;
register struct partition	*pp;
struct region			*reg1;
struct region			*reg2;
struct partition		*pheadp;

{
    return (_rt_defoverlap(ap, pp, reg1, reg2, pheadp, 0));
}

/*
 *			_ R T _ D E F O V E R L A P
 *
 *  The guts of the default overlap callback.
 *  Returns -
 *	 0	to eliminate partition with overlap entirely
 *	 1	to retain partition in output list, claimed by reg1
 *	 2	to retain partition in output list, claimed by reg2
 */
HIDDEN int
_rt_defoverlap( ap, pp, reg1, reg2, pheadp, verbose )
register struct application	*ap;
register struct partition	*pp;
struct region			*reg1;
struct region			*reg2;
struct partition		*pheadp;
register int			verbose;
{
	RT_CK_AP(ap);
	RT_CK_PT(pp);
	RT_CK_REGION(reg1);
	RT_CK_REGION(reg2);

	/*
	 *  Apply heuristics as to which region should claim partition.
	 */
	if( reg1->reg_aircode != 0 )  {
		/* reg1 was air, replace with reg2 */
		return 2;
	}
	if( pp->pt_back != pheadp ) {
		/* Repeat a prev region, if that is a choice */
		if( pp->pt_back->pt_regionp == reg1 )
			return 1;
		if( pp->pt_back->pt_regionp == reg2 )
			return 2;
	}

	/* To provide some consistency from ray to ray, use lowest bit # */
	if( reg1->reg_bit < reg2->reg_bit )
		return 1;
	return 2;
}

/*
 *			R T _ D E F A U L T _ M U L T I O V E R L A P
 *
 *  Default version of a_multioverlap().
 *
 *  Resolve the overlap of multiple regions withing a single partition.
 *  There are no null pointers in the table (they have been compressed
 *  out by our caller).  Consider BU_PTBL_LEN(regiontable) overlapping
 *  regions, and reduce to zero or one "claiming" regions, by
 *  setting pointers in the bu_ptbl of non-claiming regions to NULL.
 *
 *  This default routine reproduces the behavior of BRL-CAD Release 5.0
 *  by considerign the regions pairwise and calling the old a_overlap().
 *
 *  An application which knew how to handle multiple overlapping air
 *  regions would provide its own very different version of this routine
 *  as the a_multioverlap() handler.
 *
 *  This routine is for resolving overlaps only, and should not print
 *  any messages in normal operation; a_logoverlap() is for logging.
 */
void
rt_default_multioverlap( ap, pp, regiontable, InputHdp )
struct application	*ap;
struct partition	*pp;
struct bu_ptbl		*regiontable;
struct partition	*InputHdp;
{
	LOCAL struct region *lastregion = (struct region *)NULL;
	int	n_regions;
	int	n_fastgen = 0;
	int	code;
	int	i;

	RT_CK_AP(ap);
	RT_CK_PARTITION(pp);
	BU_CK_PTBL(regiontable);
	RT_CK_PT_HD(InputHdp);

   	if( ap->a_overlap == RT_AFN_NULL )
   		ap->a_overlap = rt_defoverlap;

	/* Count number of FASTGEN regions */
	n_regions = BU_PTBL_LEN(regiontable);
	for( i = n_regions-1; i >= 0; i-- )  {
		struct region *regp = (struct region *)BU_PTBL_GET(regiontable, i);
		RT_CK_REGION(regp);
		if( regp->reg_is_fastgen )  n_fastgen++;
	}

	if( n_fastgen >= 2 )  {
		/* Resolve all FASTGEN overlaps first. */
		bu_log("I see %d FASTGEN overlaps in this partition\n", n_fastgen);
	}

	lastregion = (struct region *)BU_PTBL_GET(regiontable, 0);
	RT_CK_REGION(lastregion);

	for( i=1; i < BU_PTBL_LEN(regiontable); i++ )  {
		struct region *regp = (struct region *)BU_PTBL_GET(regiontable, i);
		RT_CK_REGION(regp);

		/*
		 * Two or more regions claim this partition
		 */
		if( lastregion->reg_aircode != 0 && regp->reg_aircode == 0 )  {
			/* last region is air, replace with solid regp */
			goto code2;
		} else if( lastregion->reg_aircode == 0 && regp->reg_aircode != 0 )  {
			/* last region solid, regp is air, keep last */
			goto code1;
		} else if( lastregion->reg_aircode != 0 &&
		    regp->reg_aircode != 0 &&
		    regp->reg_aircode == lastregion->reg_aircode )  {
		    	/* both are same air, keep last */
			goto code1;
		}


		/*
		 *  To support ray bundles, find partition with the lower
		 *  contributing ray number (closer to center of bundle),
		 *  and retain that one.
		 */
		{
			int	r1 = rt_tree_max_raynum( lastregion->reg_treetop, pp );
			int	r2 = rt_tree_max_raynum( regp->reg_treetop, pp );
			/* Only use this algorithm if one is not the main ray */
			if( r1 > 0 || r2 > 0 )  {
/* if(rt_g.debug&DEBUG_PARTITION) */
bu_log("Potential overlay along ray bundle: r1=%d, r2=%d, resolved to %s\n", r1, r2,
(r1<r2)?lastregion->reg_name:regp->reg_name);
				if( r1 < r2 )
					goto code1;	/* keep lastregion */
				goto code2;		/* keep regp */
			}
		}

	   	/*
	   	 *  Hand overlap to old-style application-specific
	   	 *  overlap handler, or default.
		 *	0 = destroy partition,
		 *	1 = keep part, claiming region=lastregion
		 *	2 = keep part, claiming region=regp
	   	 */
		code = ap->a_overlap(ap, pp, lastregion, regp, InputHdp);

		/* Implement the policy in "code" */
		if( code == 0 )  {
			/*
			 *  Destroy the whole partition.
			 */
			if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_default_multioverlap:  overlap code=0, partition=x%x deleted\n", pp);
			bu_ptbl_reset(regiontable);
			return;
		} else if( code == 1 ) {
code1:
			/* Keep partition, claiming region = lastregion */
			if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_default_multioverlap:  overlap code=%d, p retained in region=%s\n",
				code, lastregion->reg_name );
			BU_PTBL_CLEAR_I(regiontable, i);
		} else {
code2:
			/* Keep partition, claiming region = regp */
			bu_ptbl_zero(regiontable, (long *)lastregion);
			lastregion = regp;
			if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_default_multioverlap:  overlap code=%d, p retained in region=%s\n",
				code, lastregion->reg_name );
		}
	}
}

/*
 *			R T _ S I L E N T _ L O G O V E R L A P
 *
 *  If an application doesn't want any logging from LIBRT, it should
 *  just set ap->a_logoverlap = rt_silent_logoverlap.
 */
void
rt_silent_logoverlap( ap, pp, regiontable, InputHdp )
struct application	*ap;
CONST struct partition	*pp;
CONST struct bu_ptbl	*regiontable;
CONST struct partition	*InputHdp;
{
	RT_CK_AP(ap);
	RT_CK_PT(pp);
	BU_CK_PTBL(regiontable);
	return;
}


/*
 *			R T _ D E F A U L T _ L O G O V E R L A P
 *
 *  Log a multiplicity of overlaps within a single partition.
 *  This function is intended for logging only, and a_multioverlap()
 *  is intended for resolving the overlap, only.
 *  This function can be replaced by an application setting a_logoverlap().
 */
void
rt_default_logoverlap( ap, pp, regiontable, InputHdp )
struct application	*ap;
CONST struct partition	*pp;
CONST struct bu_ptbl	*regiontable;
CONST struct partition	*InputHdp;
{
	point_t	pt;
	static long count = 0;		/* Not PARALLEL, shouldn't hurt */
	register fastf_t depth;
	int		i;
	struct bu_vls	str;

	RT_CK_AP(ap);
	RT_CK_PT(pp);
	BU_CK_PTBL(regiontable);

	/* Attempt to control tremendous error outputs */
	if( ++count > 100 )  {
		if( (count%100) != 3 )  return;
		bu_log("(overlaps omitted)\n");
	}

	/*
	 * Print all verbiage in one call to bu_log(),
	 * so that messages will be grouped together in parallel runs.
	 */
	bu_vls_init(&str);
	bu_vls_extend(&str, 80*8 );
	bu_vls_putc(&str, '\n' );

	/* List all the regions which evaluated to TRUE in this partition */
	for( i=0; i < BU_PTBL_LEN(regiontable); i++ )  {
		struct region *regp = (struct region *)BU_PTBL_GET(regiontable, i);

		if( regp == REGION_NULL )  continue;
		RT_CK_REGION(regp);

		bu_vls_printf(&str, "OVERLAP%d: %s\n", i+1, regp->reg_name);
	}

	/* List all the information common to this whole partition */
	bu_vls_printf(&str, "OVERLAPa: dist=(%g,%g) isol=%s osol=%s\n",
		pp->pt_inhit->hit_dist, pp->pt_outhit->hit_dist,
		pp->pt_inseg->seg_stp->st_name,
		pp->pt_outseg->seg_stp->st_name);

	depth = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;
	VJOIN1( pt, ap->a_ray.r_pt, pp->pt_inhit->hit_dist,
		ap->a_ray.r_dir );

	bu_vls_printf(&str, "OVERLAPb: depth %.5fmm at (%g, %g, %g) x%d y%d lvl%d\n",
		depth, pt[X], pt[Y], pt[Z],
		ap->a_x, ap->a_y, ap->a_level );

	bu_log("%s", bu_vls_addr(&str));
	bu_vls_free(&str);

#if 0
	rt_pr_partitions( ap->a_rt_i, pheadp, "Entire ray containing overlap");
#endif

}

/*
 *			R T _ B O O L F I N A L
 *
 *
 *  Consider each partition on the sorted & woven input partition list.
 *  If the partition ends before this box's start, discard it immediately.
 *  If the partition begins beyond this box's end, return.
 *
 *  Next, evaluate the boolean expression tree for all regions that have
 *  some presence in the partition.
 *
 * If 0 regions result, continue with next partition.
 * If 1 region results, a valid hit has occured, so transfer
 * the partition from the Input list to the Final list.
 * If 2 or more regions claim the partition, then an overlap exists.
 * If the overlap handler gives a non-zero return, then the overlapping
 * partition is kept, with the region ID being the first one encountered.
 * Otherwise, the partition is eliminated from further consideration. 
 *
 *  All partitions in the indicated range of the ray are evaluated.
 *  All partitions which really exist (booleval is true) are appended
 *  to the Final partition list.
 *  All partitions on the Final partition list have completely valid
 *  entry and exit information, except for the last partition's exit
 *  information when a_onehit!=0 and a_onehit is odd.
 *
 *  The flag a_onehit is interpreted as follows:
 *
 *  If a_onehit = 0, then the ray is traced to +infinity, and all
 *  hit points in the final partition list are valid.
 *
 *  If a_onehit != 0, the ray is traced through a_onehit hit points.
 *  (Recall that each partition has 2 hit points, entry and exit).
 *  Thus, if a_onehit is odd, the value of pt_outhit.hit_dist in
 *  the last partition may be incorrect;  this should not mater because
 *  the application specifically said it only wanted pt_inhit there.
 *  This is most commonly seen when a_onehit = 1, which is useful for
 *  lighting models.  Not having to correctly determine the exit point
 *  can result in a significant savings of computer time.
 *  If a_onehit is negative, it indicates the number of non-air hits needed.
 *
 *  Returns -
 *	0	If more partitions need to be done
 *	1	Requested number of hits are available in FinalHdp
 *
 *  The caller must free whatever is in both partition chains.
 *
 *
 *  NOTES for code improvements -
 *
 *  With a_onehit != 0, it is difficult to stop at the 'enddist' value
 *  (or the a_ray_length value), and always get correct results.
 *  Need to take into account some additional factors:
 *
 *  1)  A region shouldn't be evaluated until all it's solids have been
 *	intersected, to prevent the "CERN" problem of out points being
 *	wrong because subtracted solids aren't intersected yet.
 *
 *	Maybe "all" solids don't have to be intersected, but some strong
 *	statements are needed along these lines.
 *
 *	A region is definitely ready to be evaluated
 *	IF all it's solids have been intersected.
 *
 *  2)  A partition shouldn't be evaluated until all the regions within it
 *	are ready to be evaluated.
 */
int
rt_boolfinal( InputHdp, FinalHdp, startdist, enddist, regiontable, ap, solidbits )
struct partition *InputHdp;
struct partition *FinalHdp;
fastf_t startdist, enddist;
struct bu_ptbl	*regiontable;
struct application *ap;
CONST struct bu_bitv	*solidbits;
{
	LOCAL struct region *lastregion = (struct region *)NULL;
	LOCAL struct region *TrueRg[2];
	register struct partition *pp;
	register int	claiming_regions;
	int		hits_avail = 0;
	int		hits_needed;
	int		ret = 0;
	int		indefinite_outpt = 0;
	char		*reason = (char *)NULL;
	fastf_t		diff;

#define HITS_TODO	(hits_needed - hits_avail)

	RT_CK_PT_HD(InputHdp);
	RT_CK_PT_HD(FinalHdp);
	BU_CK_PTBL(regiontable);
	RT_CK_RTI(ap->a_rt_i);
	BU_CK_BITV(solidbits);

	if(rt_g.debug&DEBUG_PARTITION)  {
		bu_log("\nrt_boolfinal(%g,%g) x%d y%d lvl%d START\n",
			startdist, enddist,
			ap->a_x, ap->a_y, ap->a_level );
	}

	if( !ap->a_multioverlap )
		ap->a_multioverlap = rt_default_multioverlap;

	if( !ap->a_logoverlap )
		ap->a_logoverlap = rt_default_logoverlap;

	if( enddist <= 0 )  {
		reason = "not done, behind start point";
		ret = 0;
		goto out;
	}

	if( ap->a_onehit < 0 )
		hits_needed = -ap->a_onehit;
	else
		hits_needed = ap->a_onehit;

	if( ap->a_onehit != 0 )  {
		register struct partition *npp = FinalHdp->pt_forw;

		for(; npp != FinalHdp; npp = npp->pt_forw )  {
			if( npp->pt_inhit->hit_dist < 0.0 )
				continue;
			if( ap->a_onehit < 0 && npp->pt_regionp->reg_aircode != 0 )
				continue;	/* skip air hits */
			hits_avail += 2;	/* both in and out listed */
		}
		if( hits_avail >= hits_needed )  {
			reason = "a_onehit request satisfied at outset";
			ret = 1;
			goto out;
		}
	}

	if( ap->a_no_booleans )  {
		while( (pp = InputHdp->pt_forw) != InputHdp )  {
			RT_CK_PT(pp);
			DEQUEUE_PT(pp);
			pp->pt_regionp = (struct region *)
				BU_PTBL_GET(&pp->pt_inseg->seg_stp->st_regions, 0);
			RT_CK_REGION(pp->pt_regionp);
			if(rt_g.debug&DEBUG_PARTITION)  {
				rt_pr_pt( ap->a_rt_i, pp );
			}
			INSERT_PT( pp, FinalHdp );
		}
		ret = 0;
		reason = "No a_onehit processing in a_no_booleans mode";
		goto out;
	}

	pp = InputHdp->pt_forw;
	while( pp != InputHdp )  {
		RT_CK_PT(pp);
		claiming_regions = 0;
		if(rt_g.debug&DEBUG_PARTITION)  {
			bu_log("\nrt_boolfinal(%g,%g) x%d y%d lvl%d, next input pp\n",
				startdist, enddist,
				ap->a_x, ap->a_y, ap->a_level );
			rt_pr_pt( ap->a_rt_i, pp );
		}
		RT_CHECK_SEG(pp->pt_inseg);
		RT_CHECK_SEG(pp->pt_outseg);

		/* Force "very thin" partitions to have exactly zero thickness. */
		diff = pp->pt_inhit->hit_dist - pp->pt_outhit->hit_dist;
		if( NEAR_ZERO( diff, ap->a_rt_i->rti_tol.dist ) )  {
			if(rt_g.debug&DEBUG_PARTITION)  bu_log(
				"rt_boolfinal:  Zero thickness partition, solids %s %s (%.18e,%.18e) x%d y%d lvl%d\n",
				pp->pt_inseg->seg_stp->st_name,
				pp->pt_outseg->seg_stp->st_name,
				pp->pt_inhit->hit_dist,
				pp->pt_outhit->hit_dist,
				ap->a_x, ap->a_y, ap->a_level );
			pp->pt_outhit->hit_dist = pp->pt_inhit->hit_dist;
		}


		/* Sanity checks on sorting. */
		if( pp->pt_inhit->hit_dist > pp->pt_outhit->hit_dist )  {
			bu_log("rt_boolfinal: inverted partition %.8x x%d y%d lvl%d\n",
				pp,
				ap->a_x, ap->a_y, ap->a_level );
			rt_pr_partitions( ap->a_rt_i, InputHdp, "With problem" );
		}
		if( pp->pt_forw != InputHdp &&
		    pp->pt_outhit->hit_dist != pp->pt_forw->pt_inhit->hit_dist )  {
			diff = pp->pt_outhit->hit_dist - pp->pt_forw->pt_inhit->hit_dist;
			if( NEAR_ZERO( diff, ap->a_rt_i->rti_tol.dist ) )  {
				if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_boolfinal:  fusing 2 partitions x%x x%x\n",
					pp, pp->pt_forw );
				pp->pt_forw->pt_inhit->hit_dist = pp->pt_outhit->hit_dist;
			} else if( diff > 0 )  {
				bu_log("rt_boolfinal:  sorting defect %e > %e! x%d y%d lvl%d\n",
					pp->pt_outhit->hit_dist,
					pp->pt_forw->pt_inhit->hit_dist,
					ap->a_x, ap->a_y, ap->a_level );
				if( !(rt_g.debug & DEBUG_PARTITION) )
					rt_pr_partitions( ap->a_rt_i, InputHdp, "With DEFECT" );
				ret = 0;
				reason = "ERROR, sorting defect, give up";
				goto out;
			}
		}

		/*
		 *  If partition is behind ray start point, discard it.
		 *
		 *  Partition may start before current box starts, because
		 *  it's still waiting for all it's solids to be shot.
		 */
		if( pp->pt_outhit->hit_dist <= 0.001 /* milimeters */ )  {
			register struct partition *zap_pp;
			if(rt_g.debug&DEBUG_PARTITION)bu_log(
				"discarding partition x%x behind ray start, out dist=%g\n",
				pp, pp->pt_outhit->hit_dist);
			zap_pp = pp;
			pp = pp->pt_forw;
			DEQUEUE_PT(zap_pp);
			FREE_PT(zap_pp, ap->a_resource);
			continue;
		}

		/*
		 *  If partition begins beyond current box end,
		 *  the state of the partition is not fully known yet,
		 *  and new partitions might be added in front of this one,
		 *  so stop now.
		 */
		diff = pp->pt_inhit->hit_dist - enddist;
		if( diff > ap->a_rt_i->rti_tol.dist )  {
			if(rt_g.debug&DEBUG_PARTITION)bu_log(
				"partition begins %g beyond current box end, returning\n", diff);
			reason = "partition begins beyond current box end";
			ret = 0;
			goto out;
		}

		/*
		 *  If partition ends somewhere beyond the end of the current
		 *  box, the condition of the outhit information is not fully
		 *  known yet.
		 *  The partition might be broken or shortened by subsequent
		 *  segments, not discovered until entering future boxes.
		 */
		diff = pp->pt_outhit->hit_dist - enddist;
		if( diff > ap->a_rt_i->rti_tol.dist )  {
			if(rt_g.debug&DEBUG_PARTITION)bu_log(
				"partition ends beyond current box end\n");
			if( ap->a_onehit == 0 )  {
				ret = 0;
				reason = "a_onehit=0, trace remaining boxes";
				goto out;
			}
			/* pt_outhit may not be correct */
			indefinite_outpt = 1;
		} else {
			indefinite_outpt = 0;
		}

		/* XXX a_ray_length checking should be done here, not in rt_shootray() */

		/* Start with a clean slate when evaluating this partition */
		bu_ptbl_reset(regiontable);

		/*
		 *  For each segment's solid that lies in this partition,
		 *  add the list of regions that refer to that solid
		 *  into the "regiontable" array.
		 */
		{
			struct seg **segpp;
			for( BU_PTBL_FOR(segpp, (struct seg **), &pp->pt_seglist) )  {
				struct soltab	*stp = (*segpp)->seg_stp;
				RT_CK_SOLTAB(stp);
				bu_ptbl_cat_uniq( regiontable, &stp->st_regions );
			}
		}

		if(rt_g.debug&DEBUG_PARTITION)  {
			struct region **regpp;
			bu_log("Region table for this partition:\n");
			for( BU_PTBL_FOR( regpp, (struct region **), regiontable ) )  {
				register struct region *regp;

				regp = *regpp;
				RT_CK_REGION(regp);
				bu_log("%9lx %s\n", (long)regp, regp->reg_name);
			}
		}

		if( indefinite_outpt )  {
			if(rt_g.debug&DEBUG_PARTITION)bu_log(
				"indefinite out point, checking partition eligibility for early evaluation.\n");
			/*
			 *  More hits still needed.  HITS_TODO > 0.
			 *  If every solid in every region participating
			 *  in this partition has been intersected,
			 *  then it is OK to evaluate it now.
			 */
			if( !rt_bool_partition_eligible(regiontable, solidbits, pp) )  {
				ret = 0;
				reason = "Partition not yet eligible for evaluation";
				goto out;
			}
			if(rt_g.debug&DEBUG_PARTITION)bu_log(
				"Partition is eligibile for evaluation.\n");
		}

		/* Evaluate the boolean trees of any regions involved */
		{
			struct region **regpp;
			for( BU_PTBL_FOR( regpp, (struct region **), regiontable ) )  {
				register struct region *regp;

				regp = *regpp;
				RT_CK_REGION(regp);
				if(rt_g.debug&DEBUG_PARTITION)  {
					rt_pr_tree_val( regp->reg_treetop, pp, 2, 0 );
					rt_pr_tree_val( regp->reg_treetop, pp, 1, 0 );
					rt_pr_tree_val( regp->reg_treetop, pp, 0, 0 );
					bu_log("%.8x=bit%d, %s: ",
						regp, regp->reg_bit,
						regp->reg_name );
				}
				if( regp->reg_all_unions )  {
					if(rt_g.debug&DEBUG_PARTITION) bu_log("TRUE (all union)\n");
					claiming_regions++;
					lastregion = regp;
					continue;
				}
				if( rt_booleval( regp->reg_treetop, pp, TrueRg,
				    ap->a_resource ) == FALSE )  {
					if(rt_g.debug&DEBUG_PARTITION) bu_log("FALSE\n");
				    	/* Null out non-claiming region's pointer */
				    	*regpp = REGION_NULL;
					continue;
				}
				/* This region claims partition */
				if(rt_g.debug&DEBUG_PARTITION) bu_log("TRUE (eval)\n");
				claiming_regions++;
				lastregion = regp;
			}
		}
		if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_boolfinal:  claiming_regions=%d\n",
			claiming_regions);
		if( claiming_regions == 0 )  {
			if(rt_g.debug&DEBUG_PARTITION)bu_log("rt_boolfinal moving past partition x%x\n", pp);
			pp = pp->pt_forw;		/* onwards! */
			continue;
		}

		if( claiming_regions > 1 )  {
			/*
			 *  There is an overlap between two or more regions,
			 *  invoke multi-overlap handler.
			 */
			if(rt_g.debug&DEBUG_PARTITION)  bu_log("rt_boolfinal:  invoking a_multioverlap() pp=x%x\n", pp);
			bu_ptbl_rm( regiontable, (long *)NULL );
			ap->a_logoverlap( ap, pp, regiontable, InputHdp );
			ap->a_multioverlap( ap, pp, regiontable, InputHdp );

			/* Count number of remaining regions, s/b 0 or 1 */
			claiming_regions = 0;
			{
				register struct region **regpp;
				for( BU_PTBL_FOR( regpp, (struct region **), regiontable ) )  {
					if( *regpp != REGION_NULL )  {
						claiming_regions++;
						lastregion = *regpp;
					}
				}
			}

			/*
			 *  If claiming_regions == 0, discard partition.
			 *  If claiming_regions > 1, signal error and discard.
			 *  There is nothing further we can do to fix it.
			 */
			if( claiming_regions > 1)  {
				bu_log("rt_boolfinal() a_multioverlap() failed to resolve overlap, discarding bad partition:\n");
				rt_pr_pt( ap->a_rt_i, pp );
			}

			if( claiming_regions != 1 )  {
				register struct partition	*zap_pp;

				if(rt_g.debug&DEBUG_PARTITION)bu_log("rt_boolfinal discarding overlap partition x%x\n", pp);
				zap_pp = pp;
				pp = pp->pt_forw;		/* onwards! */
				DEQUEUE_PT( zap_pp );
				FREE_PT( zap_pp, ap->a_resource );
				continue;
			}
		}

		/*
		 *  claiming_regions == 1
		 *
		 *  Remove this partition from the input queue, and
		 *  append it to the result queue.
		 */
		{
			register struct partition	*newpp;
			register struct partition	*lastpp;

			newpp = pp;
			pp=pp->pt_forw;				/* onwards! */
			DEQUEUE_PT( newpp );
			RT_CHECK_SEG(newpp->pt_inseg);		/* sanity */
			RT_CHECK_SEG(newpp->pt_outseg);		/* sanity */
			/* Record the "owning" region.  pt_regionp = NULL before here. */
			newpp->pt_regionp = lastregion;

			/*  See if this new partition extends the previous
			 *  last partition, "exactly" matching.
			 */
			lastpp = FinalHdp->pt_back;
			if( lastpp != FinalHdp &&
			    lastregion == lastpp->pt_regionp &&
			    NEAR_ZERO( newpp->pt_inhit->hit_dist -
				lastpp->pt_outhit->hit_dist,
				ap->a_rt_i->rti_tol.dist )
			)  {
				/* same region, extend last final partition */
				RT_CK_PT(lastpp);
				RT_CHECK_SEG(lastpp->pt_inseg);	/* sanity */
				RT_CHECK_SEG(lastpp->pt_outseg);/* sanity */
				if(rt_g.debug&DEBUG_PARTITION)bu_log("rt_boolfinal collapsing %x %x\n", lastpp, newpp);
				lastpp->pt_outhit = newpp->pt_outhit;
				lastpp->pt_outflip = newpp->pt_outflip;
				lastpp->pt_outseg = newpp->pt_outseg;

				/* Don't lose the fact that the two solids
				 * of this partition contributed.
				 */
				bu_ptbl_ins_unique( &lastpp->pt_seglist, (long *)newpp->pt_inseg );
				bu_ptbl_ins_unique( &lastpp->pt_seglist, (long *)newpp->pt_outseg );

				FREE_PT( newpp, ap->a_resource );
				newpp = lastpp;
			}  else  {
				APPEND_PT( newpp, lastpp );
				if( !(ap->a_onehit < 0 && newpp->pt_regionp->reg_aircode != 0) )
					hits_avail += 2;
			}

			RT_CHECK_SEG(newpp->pt_inseg);		/* sanity */
			RT_CHECK_SEG(newpp->pt_outseg);		/* sanity */
		}

		/* See if it's worthwhile breaking out of partition loop early */
		if( ap->a_onehit != 0 && HITS_TODO <= 0 )  {
			ret = 1;
			if( pp == InputHdp )
				reason = "a_onehit satisfied at bottom";
			else
				reason = "a_onehit satisfied early";
			goto out;
		}
	}
	if( ap->a_onehit != 0 && HITS_TODO <= 0 )  {
		ret = 1;
		reason = "a_onehit satisfied at end";
	} else {
		ret = 0;
		reason = "more partitions needed";
	}
out:
	if( rt_g.debug&DEBUG_PARTITION )  {
		bu_log("rt_boolfinal() ret=%d, %s\n", ret, reason);
		rt_pr_partitions( ap->a_rt_i, FinalHdp, "rt_boolfinal: Final partition list at return:" );
		rt_pr_partitions( ap->a_rt_i, InputHdp, "rt_boolfinal: Input/pending partition list at return:" );
		bu_log("rt_boolfinal() ret=%d, %s\n", ret, reason);
	}

	/* Sanity check */
	if( rt_g.debug && ap->a_onehit == 0 &&
	    InputHdp->pt_forw != InputHdp && enddist >= INFINITY )  {
		bu_log("rt_boolfinal() ret=%d, %s\n", ret, reason);
		rt_pr_partitions( ap->a_rt_i, FinalHdp, "rt_boolfinal: Final partition list at return:" );
		rt_pr_partitions( ap->a_rt_i, InputHdp, "rt_boolfinal: Input/pending partition list at return:" );
		rt_bomb("rt_boolfinal() failed to process InputHdp list\n");
	}

	return ret;
}

/*
 *  			R T _ B O O L E V A L
 *  
 *  Using a stack to recall state, evaluate a boolean expression
 *  without recursion.
 *
 *  For use with XOR, a pointer to the "first valid subtree" would
 *  be a useful addition, for rt_boolregions().
 *
 *  Returns -
 *	!0	tree is TRUE
 *	 0	tree is FALSE
 *	-1	tree is in error (GUARD)
 */
int
rt_booleval( treep, partp, trueregp, resp )
register union tree *treep;	/* Tree to evaluate */
struct partition *partp;	/* Partition to evaluate */
struct region	**trueregp;	/* XOR true (and overlap) return */
struct resource	*resp;		/* resource pointer for this CPU */
{
	static union tree tree_not;		/* for OP_NOT nodes */
	static union tree tree_guard;		/* for OP_GUARD nodes */
	static union tree tree_xnop;		/* for OP_XNOP nodes */
	register union tree **sp;
	register int ret;
	register union tree **stackend;

	RT_CK_TREE(treep);
	RT_CK_PT(partp);
	RT_CK_RESOURCE(resp);
	if( treep->tr_op != OP_XOR )
		trueregp[0] = treep->tr_regionp;
	else
		trueregp[0] = trueregp[1] = REGION_NULL;
	while( (sp = resp->re_boolstack) == (union tree **)0 )
		rt_grow_boolstack( resp );
	stackend = &(resp->re_boolstack[resp->re_boolslen]);
	*sp++ = TREE_NULL;
stack:
	switch( treep->tr_op )  {
	case OP_NOP:
		ret = 0;
		goto pop;
	case OP_SOLID:
		{
			register struct soltab *seek_stp = treep->tr_a.tu_stp;
			register struct seg **segpp;
			for( BU_PTBL_FOR( segpp, (struct seg **), &partp->pt_seglist ) )  {
				if( (*segpp)->seg_stp == seek_stp )  {
					ret = 1;
					goto pop;
				}
			}
			ret = 0;
		}
		goto pop;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		*sp++ = treep;
		if( sp >= stackend )  {
			register int off = sp - resp->re_boolstack;
			rt_grow_boolstack( resp );
			sp = &(resp->re_boolstack[off]);
			stackend = &(resp->re_boolstack[resp->re_boolslen]);
		}
		treep = treep->tr_b.tb_left;
		goto stack;
	default:
		bu_log("rt_booleval:  bad stack op x%x\n",treep->tr_op);
		return(TRUE);	/* screw up output */
	}
pop:
	if( (treep = *--sp) == TREE_NULL )
		return(ret);		/* top of tree again */
	/*
	 *  Here, each operation will look at the operation just
	 *  completed (the left branch of the tree generally),
	 *  and rewrite the top of the stack and/or branch
	 *  accordingly.
	 */
	switch( treep->tr_op )  {
	case OP_SOLID:
		bu_log("rt_booleval:  pop SOLID?\n");
		return(TRUE);	/* screw up output */
	case OP_UNION:
		if( ret )  goto pop;	/* TRUE, we are done */
		/* lhs was false, rewrite as rhs tree */
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_INTERSECT:
		if( !ret )  {
			ret = FALSE;
			goto pop;
		}
		/* lhs was true, rewrite as rhs tree */
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_SUBTRACT:
		if( !ret )  goto pop;	/* FALSE, we are done */
		/* lhs was true, rewrite as NOT of rhs tree */
		/* We introduce the special NOT operator here */
		tree_not.tr_op = OP_NOT;
		*sp++ = &tree_not;
		treep = treep->tr_b.tb_right;
		goto stack;
	case OP_NOT:
		/* Special operation for subtraction */
		ret = !ret;
		goto pop;
	case OP_XOR:
		if( ret )  {
			/* lhs was true, rhs better not be, or we have
			 * an overlap condition.  Rewrite as guard node
			 * followed by rhs.
			 */
			if( treep->tr_b.tb_left->tr_regionp )
				trueregp[0] = treep->tr_b.tb_left->tr_regionp;
			tree_guard.tr_op = OP_GUARD;
			treep = treep->tr_b.tb_right;
			*sp++ = treep;		/* temp val for guard node */
			*sp++ = &tree_guard;
		} else {
			/* lhs was false, rewrite as xnop node and
			 * result of rhs.
			 */
			tree_xnop.tr_op = OP_XNOP;
			treep = treep->tr_b.tb_right;
			*sp++ = treep;		/* temp val for xnop */
			*sp++ = &tree_xnop;
		}
		goto stack;
	case OP_GUARD:
		/*
		 *  Special operation for XOR.  lhs was true.
		 *  If rhs subtree was true, an
		 *  overlap condition exists (both sides of the XOR are
		 *  TRUE).  Return error condition.
		 *  If subtree is false, then return TRUE (from lhs).
		 */
		if( ret )  {
			/* stacked temp val: rhs */
			if( sp[-1]->tr_regionp )
				trueregp[1] = sp[-1]->tr_regionp;
			return(-1);	/* GUARD error */
		}
		ret = TRUE;
		sp--;			/* pop temp val */
		goto pop;
	case OP_XNOP:
		/*
		 *  Special NOP for XOR.  lhs was false.
		 *  If rhs is true, take note of it's regionp.
		 */
		sp--;			/* pop temp val */
		if( ret )  {
			if( (*sp)->tr_regionp )
				trueregp[0] = (*sp)->tr_regionp;
		}
		goto pop;
	default:
		bu_log("rt_booleval:  bad pop op x%x\n",treep->tr_op);
		return(TRUE);	/* screw up output */
	}
	/* NOTREACHED */
}

/*
 *			R T _ F D I F F
 *  
 *  Compares two floating point numbers.  If they are within "epsilon"
 *  of each other, they are considered the same.
 *  NOTE:  This is a "fuzzy" difference.  It is important NOT to
 *  use the results of this function in compound comparisons,
 *  because a return of 0 makes no statement about the PRECISE
 *  relationships between the two numbers.  Eg,
 *	if( rt_fdiff( a, b ) <= 0 )
 *  is poison!
 *
 *  Returns -
 *  	-1	if a < b
 *  	 0	if a ~= b
 *  	+1	if a > b
 */
int
rt_fdiff( a, b )
double a, b;
{
	FAST double diff;
	FAST double d;
	register int ret;

	/* d = Max(Abs(a),Abs(b)) */
	d = (a >= 0.0) ? a : -a;
	if( b >= 0.0 )  {
		if( b > d )  d = b;
	} else {
		if( (-b) > d )  d = (-b);
	}
	if( d <= 1.0e-6 )  {
		ret = 0;	/* both nearly zero */
		goto out;
	}
	if( d >= INFINITY )  {
		if( a == b )  {
			ret = 0;
			goto out;
		}
		if( a < b )  {
			ret = -1;
			goto out;
		}
		ret = 1;
		goto out;
	}
	if( (diff = a - b) < 0.0 )  diff = -diff;
	if( diff < 0.001 )  {
		ret = 0;	/* absolute difference is small, < 1/1000mm */
		goto out;
	}
	if( diff < 0.000001 * d )  {
		ret = 0;	/* relative difference is small, < 1ppm */
		goto out;
	}
	if( a < b )  {
		ret = -1;
		goto out;
	}
	ret = 1;
out:
	if(rt_g.debug&DEBUG_FDIFF) bu_log("rt_fdiff(%.18e,%.18e)=%d\n", a, b, ret);
	return(ret);
}

/*
 *			R T _ R E L D I F F
 *
 * Compute the relative difference of two floating point numbers.
 *
 * Returns -
 *	0.0 if exactly the same, otherwise
 *	ratio of difference, relative to the larger of the two (range 0.0-1.0)
 */
double
rt_reldiff( a, b )
double	a, b;
{
	FAST fastf_t	d;
	FAST fastf_t	diff;

	/* d = Max(Abs(a),Abs(b)) */
	d = (a >= 0.0) ? a : -a;
	if( b >= 0.0 )  {
		if( b > d )  d = b;
	} else {
		if( (-b) > d )  d = (-b);
	}
	if( d==0.0 )
		return( 0.0 );
	if( (diff = a - b) < 0.0 )  diff = -diff;
	return( diff / d );
}

/*
 *			R T _ G R O W _ B O O L S T A C K
 *
 *  Increase the size of re_boolstack to double the previous size.
 *  Depend on rt_realloc() to copy the previous data to the new area
 *  when the size is increased.
 *  Return the new pointer for what was previously the last element.
 */
void
rt_grow_boolstack( resp )
register struct resource	*resp;
{
	if( resp->re_boolstack == (union tree **)0 || resp->re_boolslen <= 0 )  {
		resp->re_boolslen = 128;	/* default len */
		resp->re_boolstack = (union tree **)bu_malloc(
			sizeof(union tree *) * resp->re_boolslen,
			"initial boolstack");
	} else {
		resp->re_boolslen <<= 1;
		resp->re_boolstack = (union tree **)rt_realloc(
			(char *)resp->re_boolstack,
			sizeof(union tree *) * resp->re_boolslen,
			"extend boolstack" );
	}
}

/*
 *			R T _ P A R T I T I O N _ L E N
 *
 *  Return the length of a partition linked list.
 */
int
rt_partition_len( partheadp )
register struct partition	*partheadp;
{
	register struct partition	*pp;
	register long	count = 0;

	pp = partheadp->pt_forw;
	if( pp == PT_NULL )
		return(0);		/* Header not initialized yet */
	for( ; pp != partheadp; pp = pp->pt_forw )  {
		if( pp->pt_magic != 0 )  {
			/* Partitions on the free queue have pt_magic = 0 */
			RT_CK_PT(pp);
		}
		if( ++count > 1000000 )  rt_bomb("partition length > 10000000 elements\n");
	}
	return( (int)count );
}


/*
 *			R T _ T R E E _ T E S T _ R E A D Y
 *
 *  Test to see if a region is ready to be evaluated over a given
 *  partition, i.e. if all the prerequisites have been satisfied.
 *
 *  Returns -
 *	!0	Region is ready for (correct) evaluation.
 *	0	Region is not ready
 */
int
rt_tree_test_ready( tp, solidbits, regionp, pp )
register CONST union tree	*tp;
register CONST struct bu_bitv	*solidbits;
register CONST struct region	*regionp;
register CONST struct partition	*pp;
{
	RT_CK_TREE(tp);
	BU_CK_BITV(solidbits);
	RT_CK_REGION(regionp);
	RT_CK_PT(pp);

	switch( tp->tr_op )  {
	case OP_NOP:
		return 1;

	case OP_SOLID:
		if( BU_BITTEST( solidbits, tp->tr_a.tu_stp->st_bit ) )
			return 1;	/* This solid's been shot, segs are valid. */
		/*
		 *  This solid has not been shot yet.
		 */
		return 0;

	case OP_NOT:
		return !rt_tree_test_ready( tp->tr_b.tb_left, solidbits, regionp, pp );

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		if( !rt_tree_test_ready( tp->tr_b.tb_left, solidbits, regionp, pp ) )
			return 0;
		return rt_tree_test_ready( tp->tr_b.tb_right, solidbits, regionp, pp );

	default:
		rt_bomb("rt_tree_test_ready: bad op\n");
	}
	return 0;
}

/*
 *			R T _ B O O L _ P A R T I T I O N _ E L I G I B L E
 *
 *  If every solid in every region participating in this
 *  ray-partition has already been intersected with the ray,
 *  then this partition can be safely evaluated.
 *
 *  Returns -
 *	!0	Partition is ready for (correct) evaluation.
 *	0	Partition is not ready
 */
int
rt_bool_partition_eligible(regiontable, solidbits, pp)
register CONST struct bu_ptbl	*regiontable;
register CONST struct bu_bitv	*solidbits;
register CONST struct partition	*pp;
{
	struct region **regpp;

	BU_CK_PTBL(regiontable);
	BU_CK_BITV(solidbits);
	RT_CK_PT(pp);

	for( BU_PTBL_FOR( regpp, (struct region **), regiontable ) )  {
		register struct region *regp;

		regp = *regpp;
		RT_CK_REGION(regp);

		/* Check region prerequisites */
		if( !rt_tree_test_ready( regp->reg_treetop, solidbits, regp, pp) )  {
			return 0;
		}
	}
	return 1;
}


/*
 *			R T _ T R E E _ M A X _ R A Y N U M
 *
 *  Find the maximum value of the raynum (seg_rayp->index)
 *  encountered in the segments contributing to this region.
 *
 *  Returns -
 *	#	Maximum ray index
 *	-1	If no rays are contributing segs for this region.
 */
int
rt_tree_max_raynum( tp, pp )
register CONST union tree	*tp;
register CONST struct partition	*pp;
{
	RT_CK_TREE(tp);
	RT_CK_PARTITION(pp);

	switch( tp->tr_op )  {
	case OP_NOP:
		return -1;

	case OP_SOLID:
		{
			register struct soltab *seek_stp = tp->tr_a.tu_stp;
			register struct seg **segpp;
			for( BU_PTBL_FOR( segpp, (struct seg **), &pp->pt_seglist ) )  {
				if( (*segpp)->seg_stp != seek_stp )  continue;
				return (*segpp)->seg_in.hit_rayp->index;
			}
		}
		/* Maybe it hasn't been shot yet, or ray missed */
		return -1;

	case OP_NOT:
		return rt_tree_max_raynum( tp->tr_b.tb_left, pp );

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		{
			int a = rt_tree_max_raynum( tp->tr_b.tb_left, pp );
			int b = rt_tree_max_raynum( tp->tr_b.tb_right, pp );
			if( a > b )  return a;
			return b;
		}
	default:
		bu_bomb("rt_tree_max_raynum: bad op\n");
	}
	return 0;
}
