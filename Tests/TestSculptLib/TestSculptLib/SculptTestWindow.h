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
#ifndef __SCULPT_TEST_WINDOW_H__
#define __SCULPT_TEST_WINDOW_H__
#include "SupportLib/FontColours.h"
#include "WindowLib/Dialog.h"
#include "WindowLib/Window.h"
#include "WindowLib/WindowTheme.h"
#include "WindowLib/WindowTick.h"
#include "WinRefLib/WinRefWindowFactory.h"


//Called at each of the window's ticks, the first numbered 1.  False stops the window.
typedef bool (*SculptTestTickFunc)(Ptr<CWindow> pWindow, int32 iTick, void* pvData);


//A test window's tick: the test's function, with its data.
class CSculptTestTick : public CWindowTick
{
CONSTRUCTABLE(CSculptTestTick);
DESTRUCTABLE(CSculptTestTick);
protected:
	SculptTestTickFunc	mfTick;
	void*				mpvData;
	int32				miTick;

public:
	void	Init(SculptTestTickFunc fTick, void* pvData);
	void	Class(void) override;
	void	Tick(Ptr<CWindow> pWindow, int64 iUpdateTimeInMillieconds, int64 iTotalTimeInMillieconds) override;
};


//A window with no screen (WinRefLib's), as Sculpt's is, and Sculpt's look: its font, colours and theme.  The window is
//added to the root; SculptTestWindowDone takes it out.  Between the two the objects aren't validated at each change.  The look's theme is held by pTheme, so that goes first.
Ptr<CWindow>	SculptTestWindow(Ptr<CWinRefWindowFactory>& pFactory, int32 iWidth, int32 iHeight, SculptTestTickFunc fTick, void* pvData);
bool			SculptTestLook(Ptr<CWindow> pWindow, SDialogLook* psLook, Ptr<CFontColours>& pFontColours, Ptr<CWindowTheme>& pTheme);
void			SculptTestWindowDone(Ptr<CWindow>& pWindow, Ptr<CWinRefWindowFactory>& pFactory);

//The B, G and R of pImage's pixel, for an image a view drew.
uint32			SculptTestViewPixel(Ptr<CImage> pImage, int32 x, int32 y);


#endif // __SCULPT_TEST_WINDOW_H__

