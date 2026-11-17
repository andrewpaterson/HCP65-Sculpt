/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela SculptLib

Codaphela SculptLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela SculptLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela SculptLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#ifndef __SCULPT_DOCUMENT_H__
#define __SCULPT_DOCUMENT_H__
#include "StandardLib/Object.h"
#include "SupportLib/Shape.h"
#include "SculptGraph.h"


#define SCULPT_DOCUMENT_NAME	"Sculpt"	//What the object a Sculpt file holds is called.


//What a Sculpt file holds: the material's graph, its nodes where they were, the spheres dropped, and the panel's
//settings.  It's the named object of the file (WriteNamedObjectFile), there only while it's written and read.  A file
//that holds a material alone (ProceduralLib's WriteSolidMaterial) is opened too, as a graph to arrange.
class CSculptDocument : public CObject
{
CONSTRUCTABLE(CSculptDocument);
DESTRUCTABLE(CSculptDocument);
public:
	Ptr<CSculptGraph>	mpGraph;
	Ptr<CShape>			mpSculpted;  //NULL for no spheres.
	int32				miSpheres;
	int32				miShape;  //An ESculptShape,
	int32				miShow;  //and an ESculptShow.
	int32				miSamples;
	float32				mfRelief;
	SFloat32Vec3		msLight;

	void	Init(void);
	void	Class(void) override;
	void	Free(void) override;
};


void	AddSculptConstructors(void);  //For every class a Sculpt file may hold.


#endif // __SCULPT_DOCUMENT_H__

