/** ---------------- COPYRIGHT NOTICE, DISCLAIMER, and LICENSE ------------- **

Copyright (c) 2026 Andrew Paterson

This file is part of The Codaphela Project: Codaphela HCP65Sculpt

Codaphela HCP65Sculpt is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Codaphela HCP65Sculpt is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Codaphela HCP65Sculpt.  If not, see <http://www.gnu.org/licenses/>.

** ------------------------------------------------------------------------ **/
#include "BaseLib/GlobalMemory.h"
#include "StandardLib/Objects.h"
#include "WinRefLib/WinRefWindowFactory.h"
#include "WinRefLib/WinRefInput.h"
#include "SculptApp.h"


//The reference screen: a common developer's.
#define SCULPT_REF_SCREEN_WIDTH		2560
#define SCULPT_REF_SCREEN_HEIGHT	1440
#define SCULPT_REF_WINDOW_WIDTH		1280
#define SCULPT_REF_WINDOW_HEIGHT	800


//////////////////////////////////////////////////////////////////////////
//
// HCP65 Sculpt on the reference screen, drawn into memory with nothing
// on any display: for looking at what it draws where there's no screen,
// with HCP65_SCULPT_SNAPSHOT=file and HCP65_SCULPT_EXIT (see CSculptApp).
// Run from the HCP65Sculpt directory, as on a screen, with a file to
// open as its first argument if it's to have one.  Nothing outside
// it can press a key or move the pointer.
//
//////////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char* argv[])
{
	bool	bResult;

	InitSculptGlobals();

	{
		Ptr<CWinRefWindowFactory>	pNativeFactory;
		CWinRefInput				cRefInput;
		CInt32Rect					cScreen;

		//Rooted while Sculpt runs.  Its windows hold it too, so it's freed after the last of them.
		pNativeFactory = OMalloc<CWinRefWindowFactory>(&gcMemoryAllocator, SCULPT_REF_WINDOW_WIDTH, SCULPT_REF_WINDOW_HEIGHT, (char*)NULL);
		ORoot()->Add(pNativeFactory);
		cScreen.Init(0, 0, SCULPT_REF_SCREEN_WIDTH, SCULPT_REF_SCREEN_HEIGHT);
		pNativeFactory->SetScreenArea(&cScreen);

		cRefInput.Init();
		bResult = RunSculptApp(&pNativeFactory, &cRefInput, "../../../CommonLibraries/Libraries/WinInputLib/xml", (argc > 1) ? argv[1] : NULL);
		cRefInput.Kill();

		ORoot()->Remove(pNativeFactory);
		pNativeFactory = NULL;
	}

	KillSculptGlobals();
	return bResult ? 0 : 1;
}

