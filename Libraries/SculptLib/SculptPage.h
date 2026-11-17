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
#ifndef __SCULPT_PAGE_H__
#define __SCULPT_PAGE_H__
#include "SupportLib/FontColours.h"
#include "SupportLib/MeshBake.h"
#include "SupportLib/MeshEditor.h"
#include "SupportLib/MeshUnwrap.h"
#include "SupportLib/Scene.h"
#include "SupportLib/Shape.h"
#include "WindowLib/Dialog.h"
#include "WindowLib/GridContainer.h"
#include "WindowLib/Label.h"
#include "ProceduralLib/MeshShapeEditor.h"
#include "ProceduralLib/MeshShapeNormals.h"
#include "SceneViewLib/SceneView.h"


#define SCULPT_SPHERE_RADIUS	1.0f	//Of each sphere dropped.
#define SCULPT_BLEND_RADIUS		0.5f	//How far the smooth union fills the seams between spheres.
#define SCULPT_SMALLEST_CELL	0.05f	//The isosurface octree's cells, smallest
#define SCULPT_LARGEST_CELL		0.8f	//and largest.
#define SCULPT_TEXELS_PER_UNIT	128.0f	//The baked texture's density.
#define SCULPT_CHECKER_SIZE		0.25f	//The checker's cubes, in world units.


//Sculpt's page: what's being made, in a view (SceneViewLib's CSceneView, with Blender's buttons: the middle button's drag
//turns the view, with Shift it pans, the wheel zooms, R switches between Vulkan and the rasteriser and Home puts the view
//back), under a line saying what's drawn and a line of help.
//
//What's made is a shape of spheres meshed as one isosurface (ProceduralLib's CMeshShapeEditor on a CMeshEditor, its
//normals from the shape by CMeshShapeNormals), unwrapped (CMeshUnwrap) and a solid checker baked onto it (CMeshBake).
//Clicking the left button drops a sphere where the pointer is onto the plane of the screen, through what the view turns
//about: at the start that's z = 0, the view looking along -z.  The spheres are joined by a smooth union, so they blend
//where they meet.  Delete, or X, takes the spheres away.
class CSculptPage : public CGridContainer
{
CONSTRUCTABLE(CSculptPage);
DESTRUCTABLE(CSculptPage);
protected:
	Ptr<CLabel>				mpStatus;
	Ptr<CLabel>				mpHelp;
	Ptr<CSceneView>			mpView;
	Ptr<CScene>				mpScene;
	Ptr<CMeshEditor>		mpEditor;  //Whose stack meshes mpShape.
	Ptr<CMeshShapeEditor>	mpShapeEditor;
	Ptr<CMeshShapeNormals>	mpShapeNormals;  //Its normals, from the same shape.
	Ptr<CMeshUnwrap>		mpUnwrap;  //Its UVs,
	Ptr<CMeshBake>			mpBake;  //and the checker baked through them.
	Ptr<CShape>				mpShape;  //The spheres so far, NULL for none.
	int32					miSpheres;
	bool					mbInScene;  //The editor's instance is in the scene: it's added with the first sphere.

public:
	void			Init(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours> pFontColours, bool bVulkan = true);
	void			Class(void) override;
	void			Free(void) override;

	Ptr<CSceneView>	GetView(void);
	bool			DropSphere(int32 x, int32 y);
	void			Clear(void);
	int32			NumSpheres(void);

	void			Clicked(CSceneView* pcView);
	void			DeleteAction(SComponentInputEvent* psEvent);

protected:
	void			MakeScene(void);
	void			Remesh(void);
};


#endif // __SCULPT_PAGE_H__

