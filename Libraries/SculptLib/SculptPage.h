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
#include "WindowLib/SplitContainer.h"
#include "ProceduralLib/MeshShapeEditor.h"
#include "ProceduralLib/MeshShapeNormals.h"
#include "SceneViewLib/SceneView.h"
#include "SculptNodes.h"


#define SCULPT_SPHERE_RADIUS	1.0f	//Of each sphere dropped.
#define SCULPT_BLEND_RADIUS		0.5f	//How far the smooth union fills the seams between spheres.
#define SCULPT_SMALLEST_CELL	0.05f	//The isosurface octree's cells, smallest
#define SCULPT_LARGEST_CELL		0.8f	//and largest.
#define SCULPT_TEXELS_PER_UNIT	128.0f	//The baked texture's density.
#define SCULPT_MATERIAL_SEED		1		//The material it starts with is marble of this seed.
#define SCULPT_VIEW_SHARE		550		//Of the page's height the view has, out of SPLIT_CONTAINER_WHOLE: the node editor has the rest.


//Sculpt's page, as Blender's Shading workspace is laid out: what's being made in a view above (SceneViewLib's CSceneView,
//with Blender's buttons: the middle button's drag turns the view, with Shift it pans, the wheel zooms, R switches between
//Vulkan and the rasteriser and Home puts the view back), under a line saying what's drawn and a line of help; and its
//material's graph in a node editor below (CSculptNodes), a sash between the two.
//
//What's made is a shape of spheres meshed as one isosurface (ProceduralLib's CMeshShapeEditor on a CMeshEditor, its
//normals from the shape by CMeshShapeNormals), unwrapped (CMeshUnwrap) and the material baked onto it (CMeshBake): the
//graph's surface, baked again whenever the graph changes it.  The material starts as marble.
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
	Ptr<CSplitContainer>	mpSplit;  //The view over the node editor.
	Ptr<CNodeEditor>		mpEditor;
	Ptr<CSculptNodes>		mpNodes;  //The material's graph, in mpEditor.
	Ptr<CScene>				mpScene;
	Ptr<CMeshEditor>		mpMesh;  //Whose stack meshes mpShape.
	Ptr<CMeshShapeEditor>	mpShapeEditor;
	Ptr<CMeshShapeNormals>	mpShapeNormals;  //Its normals, from the same shape.
	Ptr<CMeshUnwrap>		mpUnwrap;  //Its UVs,
	Ptr<CMeshBake>			mpBake;  //and the material baked through them.
	Ptr<CShape>				mpShape;  //The spheres so far, NULL for none.
	int32					miSpheres;
	bool					mbInScene;  //The editor's instance is in the scene: it's added with the first sphere.

public:
	void				Init(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours> pFontColours, bool bVulkan = true);
	void				Class(void) override;
	void				Free(void) override;

	Ptr<CSceneView>		GetView(void);
	Ptr<CSculptNodes>	GetNodes(void);
	void				SetMaterial(Ptr<CSolidTexture> pSurface);
	void				MaterialChanged(void);
	bool				DropSphere(int32 x, int32 y);
	void				Clear(void);
	int32				NumSpheres(void);

	void				Clicked(CSceneView* pcView);
	void				DeleteAction(SComponentInputEvent* psEvent);

protected:
	void				MakeScene(void);
	void				Remesh(void);
};


#endif // __SCULPT_PAGE_H__

