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
#include "SculptPanel.h"


#define SCULPT_SPHERE_RADIUS	1.0f	//Of each sphere dropped.
#define SCULPT_BLEND_RADIUS		0.5f	//How far the smooth union fills the seams between spheres.
#define SCULPT_SMALLEST_CELL	0.05f	//The isosurface octree's cells, smallest
#define SCULPT_LARGEST_CELL		0.8f	//and largest.
#define SCULPT_TEXELS_PER_UNIT	128.0f	//The baked texture's density,
#define SCULPT_COARSE_SHARE		0.25f	//and how much of it a change is first shown at: a sixteenth of the texels, one sample each.
#define SCULPT_FINE_PAUSE		330		//Milliseconds after the last change before it's baked at the whole density.
#define SCULPT_MATERIAL_SEED	1		//The material it starts with is marble of this seed.
#define SCULPT_VIEW_SHARE		550		//Of the page's height the view has, out of SPLIT_CONTAINER_WHOLE: the node editor has the rest.


//Sculpt's page, as Blender's Shading workspace is laid out: what's looked at in a view above (SceneViewLib's CSceneView,
//with Blender's buttons: the middle button's drag turns the view, with Shift it pans, the wheel zooms, R switches between
//Vulkan and the rasteriser and Home puts the view back), under a line saying what's drawn and a line of help; and its
//material's graph in a node editor below (CSculptNodes), a sash between the two.  To their right is a panel
//(CSculptPanel) of what isn't the material's: the shape, what's shown, and the bake's settings.
//
//What's looked at is a shape meshed as an isosurface (ProceduralLib's CMeshShapeEditor on a CMeshEditor, its normals
//from the shape by CMeshShapeNormals), unwrapped (CMeshUnwrap) and the material baked onto it (CMeshBake): the graph's
//surface, or one channel of it as greys (the panel's Show), baked again whenever the graph or the panel changes it.
//The material starts as marble and the shape as a sphere.
//
//The shape is a sphere, a box, the two met, or sculpted: spheres dropped by clicking the left button where the pointer
//is onto the plane of the screen, through what the view turns about.  The spheres are joined by a smooth union, so they
//blend where they meet.  Delete, or X, takes them away.  Clicks drop nothing while another shape is shown.
//
//The materials are made with z up; the view turns about y.  So what's looked at is stood up in the view: its z is the
//view's up, and its -y, its front, faces the view at the start.
//
//A bake at the whole density is a quarter of a second to a second for a sphere (SculptPlan.txt, step 12), too long to
//follow a number being dragged.  So a change, of the shape, the material or the panel, is baked at once at a quarter of
//the density, which is a sixteenth of the work, and at the whole density once nothing has changed for a third of a
//second (the window's PostAfter).
//
//Save writes all of it but the view to one file (a CSculptDocument), and Open reads one, or a file of a material alone;
//ExportMaterial writes the material alone, as another program reads it (ProceduralLib's ReadSolidMaterial).
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
	Ptr<CSculptPanel>		mpPanel;
	Ptr<CScene>				mpScene;
	Ptr<CMeshEditor>		mpMesh;  //Whose stack meshes mpShape.
	Ptr<CMeshShapeEditor>	mpShapeEditor;
	Ptr<CMeshShapeNormals>	mpShapeNormals;  //Its normals, from the same shape.
	Ptr<CMeshUnwrap>		mpUnwrap;  //Its UVs,
	Ptr<CMeshBake>			mpBake;  //and the material baked through them.
	Ptr<CShape>				mpSculpted;  //The spheres dropped so far, NULL for none.
	int32					miSpheres;
	bool					mbInScene;  //The editor's instance is in the scene: it is while there's a shape.
	bool					mbCoarse;  //What's shown is baked at the coarse density, and the whole one is posted.

public:
	void				Init(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours> pFontColours, bool bVulkan = true);
	void				Class(void) override;
	void				Free(void) override;

	Ptr<CSceneView>		GetView(void);
	Ptr<CSculptNodes>	GetNodes(void);
	Ptr<CSculptPanel>	GetPanel(void);
	Ptr<CMeshBake>		GetBake(void);
	Ptr<CShape>			GetShape(void);
	void				PanelChanged(void);
	void				SetMaterial(Ptr<CSolidTexture> pSurface);
	void				WeatherMaterial(void);
	bool				Save(const char* szFilename);
	bool				Open(const char* szFilename);
	bool				ExportMaterial(const char* szFilename);
	void				MaterialChanged(void);
	bool				IsCoarse(void);
	void				BakeFine(void);
	bool				DropSphere(int32 x, int32 y);
	void				Clear(void);
	int32				NumSpheres(void);

	void				Clicked(CSceneView* pcView);
	void				DeleteAction(SComponentInputEvent* psEvent);

protected:
	void				MakeScene(void);
	void				Rebake(bool bCoarse);
	Ptr<CSolidTexture>	GetShown(void);
};


#endif // __SCULPT_PAGE_H__

