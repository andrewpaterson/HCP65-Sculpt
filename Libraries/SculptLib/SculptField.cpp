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
#include <string.h>
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SolidShow.h"
#include "SculptNodes.h"
#include "SculptField.h"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::Init(Ptr<CSolidTexture> pTexture, CUnmanagedField* pcField, size uiElement, CSculptNodes* pcNodes)
{
	PreInit();

	mpTexture = pTexture;
	mpcField = pcField;
	muiElement = uiElement;
	mpcNodes = pcNodes;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::Class(void)
{
	M_Pointer(mpTexture);
	U_Pointer(mpcField);
	U_Size(muiElement);
	U_Pointer(mpcNodes);
}


void CSculptField::Free(void) {}
char* CSculptField::GetFieldName(void) const { return mpcField->GetName(); }


//////////////////////////////////////////////////////////////////////////
//
// A field that's an array (but for a vector, which is one of three
// floats) is nothing a node shows.
//
//////////////////////////////////////////////////////////////////////////
ESculptFieldKind CSculptField::Kind(CUnmanagedField* pcField)
{
	if (pcField->GetLength() != 1)
	{
		return SFK_None;
	}
	switch (pcField->GetType())
	{
	case PT_float32:
	case PT_float64:
		return SFK_Number;
	case PT_int8:
	case PT_int16:
	case PT_int32:
	case PT_int64:
	case PT_uint8:
	case PT_uint16:
	case PT_uint32:
	case PT_uint64:
		return SFK_Whole;
	case PT_bool:
		return SFK_Tick;
	case PT_Enum:
		return SFK_Enum;
	case PT_M3float32:
		return SFK_Vector;
	default:
		return SFK_None;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The field's number: a float, a vector's float, or an integer of
// whatever size it is (a bool is 0 or 1, an enum its number).
//
//////////////////////////////////////////////////////////////////////////
float64 CSculptField::Get(void)
{
	void*			pvData;
	EPrimitiveType	eType;
	float32			fFloat;
	float64			fDouble;
	int64			iValue;
	uint64			uiValue;
	size			uiSize;

	pvData = mpcField->GetData(&mpTexture);
	eType = mpcField->GetType();
	if ((eType == PT_float32) || (eType == PT_M3float32))
	{
		memcpy(&fFloat, (float32*)pvData + muiElement, sizeof(float32));
		return fFloat;
	}
	if (eType == PT_float64)
	{
		memcpy(&fDouble, pvData, sizeof(float64));
		return fDouble;
	}
	if (eType == PT_bool)
	{
		return *(bool*)pvData ? 1.0 : 0.0;
	}

	//The low bytes of an integer are its first: both hosts are little-endian.
	uiSize = mpcField->GetSizeOf();
	uiValue = 0;
	memcpy(&uiValue, pvData, (uiSize < sizeof(uint64)) ? uiSize : sizeof(uint64));
	if ((eType == PT_int8) || (eType == PT_int16) || (eType == PT_int32) || (eType == PT_int64) || (eType == PT_Enum))
	{
		iValue = (int64)(uiValue << (64 - 8 * uiSize)) >> (64 - 8 * uiSize);  //Its sign carried up.
		return (float64)iValue;
	}
	return (float64)uiValue;
}


//////////////////////////////////////////////////////////////////////////
//
// The number put in the field, as its type holds it, and the texture
// told its fields have changed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::Set(float64 fValue)
{
	void*			pvData;
	EPrimitiveType	eType;
	float32			fFloat;
	int64			iValue;
	size			uiSize;

	pvData = mpcField->GetData(&mpTexture);
	eType = mpcField->GetType();
	if ((eType == PT_float32) || (eType == PT_M3float32))
	{
		fFloat = (float32)fValue;
		memcpy((float32*)pvData + muiElement, &fFloat, sizeof(float32));
	}
	else if (eType == PT_float64)
	{
		memcpy(pvData, &fValue, sizeof(float64));
	}
	else if (eType == PT_bool)
	{
		*(bool*)pvData = (fValue != 0.0);
	}
	else
	{
		uiSize = mpcField->GetSizeOf();
		iValue = (int64)fValue;
		memcpy(pvData, &iValue, (uiSize < sizeof(int64)) ? uiSize : sizeof(int64));
	}
	mpTexture->Changed();
}


//////////////////////////////////////////////////////////////////////////
//
// A number field of the field's number: whole for an integer, and kept
// to the range said of it (SolidShow.h), if one is.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CNumberField> CSculptField::MakeNumber(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel)
{
	Ptr<CNumberField>	pNumber;
	SSolidFieldShow*	psShow;

	pNumber = OMalloc<CNumberField>(pWindow, pTheme, szLabel, Get());
	pNumber->SetWhole(Kind(mpcField) == SFK_Whole);
	psShow = FindSolidFieldShow(&mpTexture, mpcField->GetName());
	if ((psShow != NULL) && (psShow->eShow == SFS_Range))
	{
		pNumber->SetRange(psShow->fMinimum, psShow->fMaximum);
	}
	pNumber->GetChangeEvent()->Add(this, &CSculptField::NumberChanged);
	return pNumber;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
Ptr<CCheckBox> CSculptField::MakeTick(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme, const char* szLabel)
{
	Ptr<CCheckBox>	pTick;

	pTick = OMalloc<CCheckBox>(pWindow, pTheme, szLabel, Get() != 0.0);
	pTick->GetChangeEvent()->Add(this, &CSculptField::Ticked);
	return pTick;
}


//////////////////////////////////////////////////////////////////////////
//
// A combo box of the enum's names (SolidShow.h), in the enum's order,
// the field's chosen: NULL if nothing says what they are.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CComboBox> CSculptField::MakeChoice(Ptr<CWindow> pWindow, Ptr<CWindowTheme> pTheme)
{
	Ptr<CComboBox>		pChoice;
	SSolidFieldShow*	psShow;
	CChars				szName;
	const char*			szNames;

	psShow = FindSolidFieldShow(&mpTexture, mpcField->GetName());
	if ((psShow == NULL) || (psShow->eShow != SFS_Enum))
	{
		return NULL;
	}

	pChoice = OMalloc<CComboBox>(pWindow, pTheme);
	szName.Init();
	for (szNames = psShow->szNames; ; szNames++)
	{
		if ((*szNames == '|') || (*szNames == '\0'))
		{
			pChoice->AddItem(szName.Text());
			szName.Clear();
			if (*szNames == '\0')
			{
				break;
			}
		}
		else
		{
			szName.Append(*szNames);
		}
	}
	szName.Kill();
	pChoice->Select((int32)Get());
	pChoice->GetSelectionEvent()->Add(this, &CSculptField::Chosen);
	return pChoice;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::NumberChanged(CNumberField* pcNumber)
{
	Set(pcNumber->GetValue());
	mpcNodes->FieldChanged();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::Ticked(CCheckBox* pcTick)
{
	Set(pcTick->IsTicked() ? 1.0 : 0.0);
	mpcNodes->FieldChanged();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptField::Chosen(CComboBox* pcChoice)
{
	Set((float64)pcChoice->GetSelected());
	mpcNodes->FieldChanged();
}

