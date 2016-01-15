/*                       S U R F A C E T R E E . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2016 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** @addtogroup brep_surfacetree
 *
 * @brief
 * Curve Tree.
 *
 */
/** @{ */
/** @file brep/surfacetree.h */

#ifndef BREP_SURFACETREE_H
#define BREP_SURFACETREE_H

#include "common.h"
#ifdef __cplusplus
#include <list>
#include <queue>
#endif
#include "brep/defines.h"
#include "brep/curvetree.h"
#include "brep/bbnode.h"

#ifdef __cplusplus

__BEGIN_DECLS

extern "C++" {
    namespace brlcad {

	/**
	 * SurfaceTree declaration
	 */
	class BREP_EXPORT SurfaceTree {
	    private:
		bool m_removeTrimmed;

	    public:
		SurfaceTree();
		SurfaceTree(const ON_BrepFace *face, bool removeTrimmed = true, int depthLimit = BREP_MAX_FT_DEPTH, double within_distance_tol = BREP_EDGE_MISS_TOLERANCE);
		~SurfaceTree();

		CurveTree *ctree;

		BBNode *getRootNode() const;

		/**
		 * Calculate, using the surface bounding volume hierarchy, a uv
		 * estimate for the closest point on the surface to the point in
		 * 3-space.
		 */
		ON_2dPoint getClosestPointEstimate(const ON_3dPoint &pt);
		ON_2dPoint getClosestPointEstimate(const ON_3dPoint &pt, ON_Interval &u, ON_Interval &v);

		/**
		 * Return surface
		 */
		const ON_Surface *getSurface();
		int getSurfacePoint(const ON_3dPoint &pt, ON_2dPoint &uv, const ON_3dPoint &from, double tolerance = BREP_SAME_POINT_TOLERANCE) const;

		/**
		 * Return just the leaves of the surface tree
		 */
		void getLeaves(std::list<BBNode *> &out_leaves);
		int depth();

	    private:
		bool isFlat(ON_Plane frames[]);
		bool isStraight(ON_Plane frames[]);
		bool isFlatU(ON_Plane frames[]);
		bool isFlatV(ON_Plane frames[]);
		BBNode *subdivideSurface(const ON_Surface *localsurf, const ON_Interval &u, const ON_Interval &v, ON_Plane frames[], int depth, int depthLimit, int prev_knot, double within_distance_tol);
		BBNode *surfaceBBox(const ON_Surface *localsurf, bool leaf, ON_Plane frames[], const ON_Interval &u, const ON_Interval &v, double within_distance_tol);

		const ON_BrepFace *m_face;
		BBNode *m_root;
		std::queue<ON_Plane *> *f_queue;
	};

    } /* namespace brlcad */
} /* extern C++ */

__END_DECLS

#endif

/** @} */

#endif  /* BREP_SURFACETREE_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */