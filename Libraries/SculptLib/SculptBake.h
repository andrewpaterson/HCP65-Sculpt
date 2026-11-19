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
#ifndef __SCULPT_BAKE_H__
#define __SCULPT_BAKE_H__
#include "BaseLib/Jobs.h"
#include "StandardLib/Object.h"
#include "SupportLib/MeshBake.h"
#include "SupportLib/MeshEditor.h"
#include "SupportLib/Shape.h"
#include "SupportLib/SolidTexture.h"


#define SCULPT_SMALLEST_CELL	0.05f	//The isosurface octree's cells, smallest
#define SCULPT_LARGEST_CELL		0.8f	//and largest.


//Where a bake is, between the thread that described it and a baker's.
enum ESculptBakeState
{
	SBS_Described,	//Nobody has been asked to make it.
	SBS_Waiting,	//A baker has it, and hasn't started.
	SBS_Baking,
	SBS_Baked,		//Its mesh is made.
	SBS_Dropped,	//A baker was given another before this one was made, and didn't start it or stopped it: it has no mesh.
};


//What Sculpt's page shows, described: a shape meshed as an isosurface (ProceduralLib's CMeshShapeEditor) with the
//shape's normals (CMeshShapeNormals), unwrapped (CMeshUnwrap) and a texture baked onto it (CMeshBake).  MakeEditor is
//an editor whose stack of those modifiers makes it, and Apply gives such a stack this bake's settings.
//
//Make makes its mesh on the thread that calls it, in that thread's object system: a worker's (StandardLib's
//ObjectsWorker.h), whose mesh the main object system then adopts, is how a CSculptBaker makes one without the page's
//thread waiting.  For that the shape and the texture must be ones nothing changes while it's made: the page gives a
//copy of its material's graph.  mpMesh is the mesh once the state is SBS_Baked.
//
//Stop, from another thread while it's made, has Make give up as soon as the bake notices (CMeshBake::SetStop, which
//is most of a bake at the whole density): it is then SBS_Dropped, with no mesh.
//
//The state and the stop are read and written by both threads (atomically); everything else is set before a baker is
//given it, and mpMesh by the baker before it says SBS_Baked.
class CSculptBake : public CObject
{
CONSTRUCTABLE(CSculptBake);
DESTRUCTABLE(CSculptBake);
public:
	Ptr<CShape>			mpShape;
	Ptr<CSolidTexture>	mpTexture;
	float32				mfTexelsPerUnit;
	int					miSamples;  //CMeshBake's: the most each way in a texel.
	float32				mfRelief;
	SFloat32Vec3		msLight;  //A unit.
	bool				mbCoarse;  //At less than the whole density, to show a change as it's made: the page's to know.
	Ptr<CMesh>			mpMesh;

protected:
	int32				miState;
	int32				miStop;  //Not zero once Stop is called.

public:
	void	Init(Ptr<CShape> pShape, Ptr<CSolidTexture> pTexture, float32 fTexelsPerUnit, int iSamples, float32 fRelief, SFloat32Vec3* psLight, bool bCoarse);
	void	Class(void) override;
	void	Free(void) override;

	static Ptr<CMeshEditor>	MakeEditor(void);
	void					Apply(CMeshEditor* pcEditor);
	void					Make(CJobs* pcJobs);  //On pcJobs' threads too, if it isn't NULL (CMeshBake::SetJobs).
	void					Stop(void);

	ESculptBakeState	GetState(void);
	void				SetState(ESculptBakeState eState);
	bool				IsBaked(void);
	bool				IsDropped(void);
};


#endif // __SCULPT_BAKE_H__
