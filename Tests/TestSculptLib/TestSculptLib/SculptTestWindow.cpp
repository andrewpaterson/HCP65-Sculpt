/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela TestSculptLib

Codaphela TestSculptLib is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela TestSculptLib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela TestSculptLib.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include <string.h>
#include "BaseLib/GlobalMemory.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SupportLib/TrueTypeFontFactory.h"
#include "WindowLib/JewelTheme.h"
#include "SculptTestWindow.h"


#define SCULPT_TEST_FONT	".." _FS_ ".." _FS_ ".." _FS_ "HCP65Sculpt" _FS_ "HCP65Sculpt" _FS_ "Resources" _FS_ "InterVariable.ttf"


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptTestTick::Init(SculptTestTickFunc fTick, void* pvData)
{
	PreInit();

	CWindowTick::Init();
	mfTick = fTick;
	mpvData = pvData;
	miTick = 0;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptTestTick::Class(void)
{
	CWindowTick::Class();

	U_Pointer(mpvData);
	U_Int32(miTick);
}


//////////////////////////////////////////////////////////////////////////
//
// The window is drawn again and ticked again at once, whatever the tick
// did, until the test's function stops it.
//
//////////////////////////////////////////////////////////////////////////
void CSculptTestTick::Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds)
{
	miTick++;
	if (!mfTick(pWindow, miTick, mpvData))
	{
		pWindow->Stop();
	}
	else
	{
		pWindow->Invalidate();
		pWindow->RequestTick(0);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// The window's pixels are B, G, R, X as Sculpt's are.  It writes no
// frames.
//
//////////////////////////////////////////////////////////////////////////
Ptr<CWindow> SculptTestWindow(Ptr<CWinRefWindowFactory>& pFactory, int32 iWidth, int32 iHeight, SculptTestTickFunc fTick, void* pvData)
{
	Ptr<CWindow>	pWindow;
	CPointer		pNull;

	pFactory = OMalloc<CWinRefWindowFactory>(&gcMemoryAllocator, iWidth, iHeight, (char*)NULL);
	pWindow = OMalloc<CWindow>("Sculpt Test", &pFactory, OMalloc<CSculptTestTick>(fTick, pvData), pNull, CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);
	ORoot()->Add(pWindow);
	return pWindow;
}


//////////////////////////////////////////////////////////////////////////
//
// The app's font (from its Resources), white, its colours, and the dark
// theme.  False if the font can't be read.
//
//////////////////////////////////////////////////////////////////////////
bool SculptTestLook(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours>& pFontColours, Ptr<CWindowTheme>& pTheme)
{
	Ptr<CFont>	pWhite;

	memset(psLook, 0, sizeof(SDialogLook));
	pWhite = ReadTrueTypeFont(SCULPT_TEST_FONT, "Inter 13px", 13.0f, pWindow->GetColourOrder());
	if (pWhite.IsNull())
	{
		return false;
	}
	pFontColours = OMalloc<CFontColours>(pWhite);
	pTheme = CJewelTheme::CreateDarkTheme(pWindow, pFontColours);
	psLook->pcTheme = &pTheme;
	psLook->pcDimFont = &pFontColours->GetFont(pTheme->GetColour(TC_Icon));
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void SculptTestWindowDone(Ptr<CWindow>& pWindow, Ptr<CWinRefWindowFactory>& pFactory)
{
	ORoot()->Remove(pWindow);
	pWindow = NULL;
	pFactory = NULL;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
uint32 SculptTestViewPixel(Ptr<CImage> pImage, int32 x, int32 y)
{
	uint32	uiPixel;

	memcpy(&uiPixel, (uint8*)pImage->GetData() + ((size)y * pImage->GetWidth() + (size)x) * 4, 4);
	return uiPixel & 0x00FFFFFF;
}

