/*                    R A N D S P H . C
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

/** @addtogroup rnd */
/** @{ */
/** @file libbn/randsph.c
 *
 * @brief
 * These routines implement the Marsaglia(1972) method for picking random
 * points on a sphere: https://projecteuclid.org/euclid.aoms/1177692644
 */

#include "common.h"

#include <stdio.h>
#include "vmath.h"
#include "bn/rand.h"
#include "bn/randmt.h"

HIDDEN void
_bn_unit_sph_sample(point_t pnt)
{
    fastf_t px, py, pz, S;
    fastf_t V1, V2;
    int have_rand_1 = 0;
    int have_rand_2 = 0;

    /* Get our two random numbers */
    while (!have_rand_1 || !have_rand_2) {
	/* need range from -1 to 1, but bn_randmt is from 0 to 1.  Use
	 * an initial bn_randmt to decide the sign, and then generate
	 * another number that constitutes the actual value.  Because
	 * we don't test for == 0.5, use while loops to make absolutely
	 * sure an assignment takes place. */
	while (!have_rand_1) {
	    fastf_t V1flag = bn_randmt();
	    if (V1flag > 0.5) {V1 = bn_randmt(); have_rand_1++;}
	    if (V1flag < 0.5) {V1 = -1*bn_randmt(); have_rand_1++;}
	}
	while (!have_rand_2) {
	    fastf_t V2flag = bn_randmt();
	    if (V2flag > 0.5) {V2 = bn_randmt(); have_rand_2++;}
	    if (V2flag < 0.5) {V2 = -1*bn_randmt(); have_rand_2++;}
	}
	/* Check that V1^2+V2^2 < 1 */
	S = V1*V1 + V2*V2;
	if (S >= 1) {
	    have_rand_1 = 0;
	    have_rand_2 = 0;
	}
    }

    /* Given the random numbers, generate the xyz points on the
     * unit sphere */
    S  = V1*V1 + V2*V2;
    px = 2 * V1 * sqrt(1 - S);
    py = 2 * V2 * sqrt(1 - S);
    pz = 1 - 2 * S;

    pnt[0] = px;
    pnt[1] = py;
    pnt[2] = pz;
}

void
bn_sph_sample(point_t sample, const point_t center, const fastf_t radius)
{
    _bn_unit_sph_sample(sample);

    /* If we've got a non-unit sph radius, scale the point */
    if (!NEAR_EQUAL(radius, 1.0, SMALL_FASTF)) {
	VSCALE(sample, sample, radius);
    }

    /* If we've got a non-zero sph center, translate the point */
    if (!VNEAR_ZERO(sample, SMALL_FASTF)) {
	VADD2(sample, sample, center);
    }
}

/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
