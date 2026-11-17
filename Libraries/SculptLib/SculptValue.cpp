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
#include "SolidShow.h"
#include "SculptNodes.h"
#include "SculptValue.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::Init(Ptr<CSculptGraph> pGraph, Ptr<CSculptNode> pNode, size uiInput, CSculptNodes* pcNodes)
{
	PreInit();

	mpGraph = pGraph;
	mpNode = pNode;
	muiInput = uiInput;
	mpcNodes = pcNodes;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::Class(void)
{
	M_Pointer(mpGraph);
	M_Pointer(mpNode);
	U_Size(muiInput);
	M_Pointer(mpNumber);
	M_Pointer(mpColour);
	U_Pointer(mpcNodes);
}


//////////////////////////////////////////////////////////////////////////
//
// The components may outlive this (their box holds them): they're no
// longer listened to.
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::Free(void)
{
	if (mpNumber.IsNotNull())
	{
		mpNumber->GetChangeEvent()->Remove(this);
	}
	if (mpColour.IsNotNull())
	{
		mpColour->GetChangeEvent()->Remove(this);
	}
}


Ptr<CSculptNode> CSculptValue::GetNode(void) { return mpNode; }
size CSculptValue::GetInput(void) { return muiInput; }
Ptr<CNumberField> CSculptValue::GetNumber(void) { return mpNumber; }
Ptr<CColourField> CSculptValue::GetColour(void) { return mpColour; }


//////////////////////////////////////////////////////////////////////////
//
// The flat texture the input points at: NULL while a node is linked to
// it.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CSolidFlat> CSculptValue::GetFlat(void)
{
	return Ptr<CSolidFlat>(dynamic_cast<CSolidFlat*>(&mpGraph->GetInput(&mpNode, muiInput)));
}


//////////////////////////////////////////////////////////////////////////
//
// The component the value is shown and changed with: a number field
// for an input said to be a number from 0 to 1, a colour field
// otherwise.  The output's surface is a colour.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComponent> CSculptValue::Make(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel)
{
	SSolidFieldShow*	psShow;

	psShow = mpNode->IsOutput() ? NULL : FindSolidFieldShow(&mpNode->mpTexture, SolidInputField(&mpNode->mpTexture, muiInput));
	if ((psShow != NULL) && (psShow->eShow == SFS_Range))
	{
		mpNumber = OMalloc<CNumberField>(pWindow, pTheme, szLabel, 0.0);
		mpNumber->SetRange(psShow->fMinimum, psShow->fMaximum);
		mpNumber->GetChangeEvent()->Add(this, &CSculptValue::NumberChanged);
		Show();
		return mpNumber;
	}
	mpColour = OMalloc<CColourField>(pWindow, pTheme, szLabel, 0.0f, 0.0f, 0.0f);
	mpColour->GetChangeEvent()->Add(this, &CSculptValue::ColourChanged);
	Show();
	return mpColour;
}


//////////////////////////////////////////////////////////////////////////
//
// The component shows what the input's flat texture is now: its colour,
// or its colour's brightness.  Nothing, while a node is linked.
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::Show(void)
{
	Ptr<CSolidFlat>		pFlat;

	pFlat = GetFlat();
	if (pFlat.IsNull())
	{
		return;
	}
	if (mpNumber.IsNotNull())
	{
		mpNumber->SetValue(pFlat->msSample.Value());
	}
	if (mpColour.IsNotNull())
	{
		mpColour->SetColour(pFlat->msSample.sColour.x, pFlat->msSample.sColour.y, pFlat->msSample.sColour.z);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The flat texture's red, green and blue: its other channels are as they
// were.
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::SetColour(float32 fRed, float32 fGreen, float32 fBlue)
{
	Ptr<CSolidFlat>		pFlat;

	pFlat = GetFlat();
	if (pFlat.IsNotNull())
	{
		pFlat->msSample.sColour.x = fRed;
		pFlat->msSample.sColour.y = fGreen;
		pFlat->msSample.sColour.z = fBlue;
		mpcNodes->FieldChanged();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// A number is a grey of that brightness.
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::NumberChanged(CNumberField* pcNumber)
{
	float32	fValue;

	fValue = (float32)pcNumber->GetValue();
	SetColour(fValue, fValue, fValue);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptValue::ColourChanged(CColourField* pcColour)
{
	float32*	pafColour;

	pafColour = pcColour->GetColour();
	SetColour(pafColour[0], pafColour[1], pafColour[2]);
}

