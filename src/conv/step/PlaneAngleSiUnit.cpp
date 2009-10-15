/*                 PlaneAngleSiUnit.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file PlaneAngleSiUnit.cpp
 *
 * Routines to convert STEP "PlaneAngleSiUnit" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "PlaneAngleSiUnit.h"

#define CLASSNAME "PlaneAngleSiUnit"
#define ENTITYNAME "Plane_Angle_Si_Unit"
string PlaneAngleSiUnit::entityname = Factory::RegisterClass(ENTITYNAME,(FactoryMethod)PlaneAngleSiUnit::Create);

PlaneAngleSiUnit::PlaneAngleSiUnit() {
	step = NULL;
	id = 0;
}

PlaneAngleSiUnit::PlaneAngleSiUnit(STEPWrapper *sw,int STEPid) {
	step = sw;
	id = STEPid;
}

PlaneAngleSiUnit::~PlaneAngleSiUnit() {
}

bool
PlaneAngleSiUnit::Load(STEPWrapper *sw,SCLP23(Application_instance) *sse) {
	step=sw;
	id = sse->STEPfile_id;


	// load base class attributes
	if ( !PlaneAngleUnit::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::Unit." << endl;
		return false;
	}
	if ( !SiUnit::Load(step,sse) ) {
		cout << CLASSNAME << ":Error loading base class ::Unit." << endl;
		return false;
	}

	return true;
}

void
PlaneAngleSiUnit::Print(int level) {
	TAB(level); cout << CLASSNAME << ":" << "(";
	cout << "ID:" << STEPid() << ")" << endl;

	TAB(level); cout << "Inherited Attributes:" << endl;
	PlaneAngleUnit::Print(level+1);
	SiUnit::Print(level+1);

}
STEPEntity *
PlaneAngleSiUnit::Create(STEPWrapper *sw, SCLP23(Application_instance) *sse) {
	Factory::OBJECTS::iterator i;
	if ((i = Factory::FindObject(sse->STEPfile_id)) == Factory::objects.end()) {
		PlaneAngleSiUnit *object = new PlaneAngleSiUnit(sw,sse->STEPfile_id);

		Factory::AddObject(object);

		if (!object->Load(sw, sse)) {
			cerr << CLASSNAME << ":Error loading class in ::Create() method." << endl;
			delete object;
			return NULL;
		}
		return static_cast<STEPEntity *>(object);
	} else {
		return (*i).second;
	}
}
