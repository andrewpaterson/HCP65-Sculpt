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
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SupportLib/MeshBake.h"
#include "SculptPanel.h"


const char* gaszSculptShapes[SS_NumShapes] = { "Sphere", "Box", "Sphere and Box", "Sculpted" };
const char* gaszSculptShows[SSH_NumShows] = { "Baked", "Colour", "Height", "Shine", "Gloss", "Glow" };
const char* gaszSculptLight[3] = { "Light X", "Light Y", "Light Z" };


//////////////////////////////////////////////////////////////////////////
//
// A sphere, baked, with the settings a CMeshBake starts with.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::Init(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme)
{
	SFloat32Vec3	sLight;
	size			i;

	PreInit();

	CGridContainer::Init(pWindow);
	miNumRows = 0;
	mcChanged.Init();

	AddHeading(pTheme, "Shape");
	mpShape = OMalloc<CComboBox>(pWindow, pTheme);
	for (i = 0; i < SS_NumShapes; i++)
	{
		mpShape->AddItem(gaszSculptShapes[i]);
	}
	mpShape->Select(SS_Sphere);
	mpShape->GetSelectionEvent()->Add(this, &CSculptPanel::Chosen);
	AddRow(mpShape);

	AddHeading(pTheme, "Show");
	mpShow = OMalloc<CComboBox>(pWindow, pTheme);
	for (i = 0; i < SSH_NumShows; i++)
	{
		mpShow->AddItem(gaszSculptShows[i]);
	}
	mpShow->Select(SSH_Baked);
	mpShow->GetSelectionEvent()->Add(this, &CSculptPanel::Chosen);
	AddRow(mpShow);

	AddHeading(pTheme, "Bake");
	mpSamples = AddNumber(pTheme, "Samples", 1, MESH_BAKE_SAMPLES);
	mpSamples->SetWhole(true);
	mpRelief = AddNumber(pTheme, "Relief", 0, 2);
	for (i = 0; i < 3; i++)
	{
		mapLight[i] = AddNumber(pTheme, gaszSculptLight[i], -1, 1);
	}
	sLight.Init(MESH_BAKE_LIGHT_X, MESH_BAKE_LIGHT_Y, MESH_BAKE_LIGHT_Z);
	SetBake(MESH_BAKE_SAMPLES, 1.0f, &sLight);

	//What room is left is under the rows, not shared out between them.
	AddComponent(OMalloc<CLabel>(pWindow, pTheme->GetFont(), ""), 0, miNumRows, 1, 1, 1, 1);

	SetInsets(4, 6, 4, 6);
	SetDesiredSize(SCULPT_PANEL_WIDTH, -1);
	SetAlignment(CSH_Right, CSV_Stretch);

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::Class(void)
{
	CGridContainer::Class();

	M_Pointer(mpShape);
	M_Pointer(mpShow);
	M_Pointer(mpSamples);
	M_Pointer(mpRelief);
	M_Pointer(mapLight[0]);
	M_Pointer(mapLight[1]);
	M_Pointer(mapLight[2]);
	U_Int32(miNumRows);
	U_Unknown(CComponentEvent<CSculptPanel>, mcChanged);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::Free(void)
{
	mcChanged.Kill();
	CGridContainer::Free();
}


//////////////////////////////////////////////////////////////////////////
//
// A row under the last, as wide as the panel.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::AddRow(Ptr<CComponent> pComponent)
{
	pComponent->SetAlignment(CSH_Stretch, CSV_Center);
	AddComponent(pComponent, 0, miNumRows, 1, 1, 1, 0);
	miNumRows++;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::AddHeading(Ptr<CWindowTheme> pTheme, const char* szText)
{
	AddRow(OMalloc<CLabel>(mpWindow, pTheme->GetFont(), szText));
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CNumberField> CSculptPanel::AddNumber(Ptr<CWindowTheme> pTheme, const char* szName, float64 fMinimum, float64 fMaximum)
{
	Ptr<CNumberField>	pNumber;

	pNumber = OMalloc<CNumberField>(mpWindow, pTheme, szName, fMinimum);
	pNumber->SetRange(fMinimum, fMaximum);
	pNumber->GetChangeEvent()->Add(this, &CSculptPanel::NumberChanged);
	AddRow(pNumber);
	return pNumber;
}


ESculptShape CSculptPanel::GetShape(void) { return (ESculptShape)mpShape->GetSelected(); }
void CSculptPanel::SetShape(ESculptShape eShape) { mpShape->Select(eShape); }
ESculptShow CSculptPanel::GetShow(void) { return (ESculptShow)mpShow->GetSelected(); }
void CSculptPanel::SetShow(ESculptShow eShow) { mpShow->Select(eShow); }
int32 CSculptPanel::GetSamples(void) { return (int32)mpSamples->GetValue(); }
float32 CSculptPanel::GetRelief(void) { return (float32)mpRelief->GetValue(); }
CComponentEvent<CSculptPanel>* CSculptPanel::GetChangeEvent(void) { return &mcChanged; }


//////////////////////////////////////////////////////////////////////////
//
// Where the light is from the surface, as its three numbers have it: not
// a unit.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::GetLight(SFloat32Vec3* psLight)
{
	psLight->Init((float32)mapLight[0]->GetValue(), (float32)mapLight[1]->GetValue(), (float32)mapLight[2]->GetValue());
}


//////////////////////////////////////////////////////////////////////////
//
// The bake's settings shown, without calling the change event.
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::SetBake(int32 iSamples, float32 fRelief, SFloat32Vec3* psLight)
{
	mpSamples->SetValue(iSamples);
	mpRelief->SetValue(fRelief);
	mapLight[0]->SetValue(psLight->x);
	mapLight[1]->SetValue(psLight->y);
	mapLight[2]->SetValue(psLight->z);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::Chosen(CComboBox* pcChoice)
{
	mcChanged.Call(this);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptPanel::NumberChanged(CNumberField* pcNumber)
{
	mcChanged.Call(this);
}

