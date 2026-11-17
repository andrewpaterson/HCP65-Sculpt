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
#include "SculptNodes.h"
#include "SculptRamp.h"


const char* gaszSculptRampExtras[SCULPT_RAMP_EXTRAS] = { "Height", "Shine", "Gloss", "Glow" };


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::Init(Ptr<CSolidRamp> pRamp, CSculptNodes* pcNodes)
{
	PreInit();

	mpRamp = pRamp;
	mpcNodes = pcNodes;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::Class(void)
{
	M_Pointer(mpRamp);
	M_Pointer(mpBar);
	M_Pointer(mapExtras[0]);
	M_Pointer(mapExtras[1]);
	M_Pointer(mapExtras[2]);
	M_Pointer(mapExtras[3]);
	U_Pointer(mpcNodes);
}


//////////////////////////////////////////////////////////////////////////
//
// The components may outlive this (their box holds them): they're no
// longer listened to.
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::Free(void)
{
	size	i;

	if (mpBar.IsNotNull())
	{
		mpBar->GetChangeEvent()->Remove(this);
		mpBar->GetChoiceEvent()->Remove(this);
	}
	for (i = 0; i < SCULPT_RAMP_EXTRAS; i++)
	{
		if (mapExtras[i].IsNotNull())
		{
			mapExtras[i]->GetChangeEvent()->Remove(this);
		}
	}
}


Ptr<CSolidRamp> CSculptRamp::GetRamp(void) const { return mpRamp; }
Ptr<CColourRampBar> CSculptRamp::GetBar(void) const { return mpBar; }
Ptr<CNumberField> CSculptRamp::GetExtra(size uiExtra) { return mapExtras[uiExtra]; }


//////////////////////////////////////////////////////////////////////////
//
// A sample's height, shine, gloss or glow.
//
//////////////////////////////////////////////////////////////////////////
float32* CSculptRamp::Extra(SSolidSample* psSample, size uiExtra)
{
	switch (uiExtra)
	{
	case 0:		return &psSample->fHeight;
	case 1:		return &psSample->fShine;
	case 2:		return &psSample->fGloss;
	default:	return &psSample->fGlow;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The bar, a stop for each key, and the chosen key's other numbers, as
// rows of the ramp's box.
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::AddRows(Ptr<CNodeBox> pBox, Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme)
{
	SSolidRampKey*	psKey;
	size			i;

	mpBar = OMalloc<CColourRampBar>(pWindow, pTheme);
	for (i = 0; i < mpRamp->masKeys.NumElements(); i++)
	{
		psKey = mpRamp->masKeys.Get(i);
		mpBar->AddStop(psKey->fValue, psKey->sSample.sColour.x, psKey->sSample.sColour.y, psKey->sSample.sColour.z, (int32)i);
	}
	mpBar->Choose(0);
	mpBar->GetChangeEvent()->Add(this, &CSculptRamp::BarChanged);
	mpBar->GetChoiceEvent()->Add(this, &CSculptRamp::BarChosen);
	pBox->AddRow(mpBar);

	for (i = 0; i < SCULPT_RAMP_EXTRAS; i++)
	{
		mapExtras[i] = OMalloc<CNumberField>(pWindow, pTheme, gaszSculptRampExtras[i], 0.0);
		if (i > 0)
		{
			mapExtras[i]->SetRange(0.0, 1.0);
		}
		mapExtras[i]->GetChangeEvent()->Add(this, &CSculptRamp::ExtraChanged);
		pBox->AddRow(mapExtras[i]);
	}
	ShowExtras();
}


//////////////////////////////////////////////////////////////////////////
//
// The number fields are the chosen key's.
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::ShowExtras(void)
{
	SSolidRampKey*	psKey;
	size			i;

	psKey = mpRamp->masKeys.SafeGet((size)mpBar->GetChosen());
	if ((mpBar->GetChosen() >= 0) && (psKey != NULL))
	{
		for (i = 0; i < SCULPT_RAMP_EXTRAS; i++)
		{
			mapExtras[i]->SetValue(*Extra(&psKey->sSample, i));
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The ramp's keys are the bar's stops, in their order: each the key its
// stop's data says, or for a new stop the ramp's sample there as it was,
// at the stop's place and in its colour.  Each stop's data is then where
// its key now is.
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::BarChanged(CColourRampBar* pcBar)
{
	CArraySolidRampKey	asKeys;
	SColourRampStop*	psStop;
	SSolidRampKey*		psKey;
	SSolidRampKey*		psOld;
	size				i;

	asKeys.Init();
	for (i = 0; i < pcBar->NumStops(); i++)
	{
		psStop = pcBar->GetStop(i);
		psKey = asKeys.Add();
		psOld = (psStop->iData >= 0) ? mpRamp->masKeys.SafeGet((size)psStop->iData) : NULL;
		if (psOld != NULL)
		{
			psKey->sSample = psOld->sSample;
		}
		else
		{
			mpRamp->At(psStop->fPlace, &psKey->sSample);
		}
		psKey->fValue = psStop->fPlace;
		psKey->sSample.sColour.x = psStop->afColour[0];
		psKey->sSample.sColour.y = psStop->afColour[1];
		psKey->sSample.sColour.z = psStop->afColour[2];
	}
	mpRamp->masKeys.ReInit();
	mpRamp->masKeys.Copy(&asKeys);
	asKeys.Kill();
	for (i = 0; i < pcBar->NumStops(); i++)
	{
		pcBar->GetStop(i)->iData = (int32)i;
	}

	mpRamp->Changed();
	ShowExtras();
	mpcNodes->FieldChanged();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::BarChosen(CColourRampBar* pcBar)
{
	ShowExtras();
}


//////////////////////////////////////////////////////////////////////////
//
// One of the chosen key's other numbers was changed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptRamp::ExtraChanged(CNumberField* pcNumber)
{
	SSolidRampKey*	psKey;
	size			i;

	psKey = mpRamp->masKeys.SafeGet((size)mpBar->GetChosen());
	if ((mpBar->GetChosen() < 0) || (psKey == NULL))
	{
		return;
	}
	for (i = 0; i < SCULPT_RAMP_EXTRAS; i++)
	{
		if (&mapExtras[i] == pcNumber)
		{
			*Extra(&psKey->sSample, i) = (float32)pcNumber->GetValue();
		}
	}
	mpcNodes->FieldChanged();
}

