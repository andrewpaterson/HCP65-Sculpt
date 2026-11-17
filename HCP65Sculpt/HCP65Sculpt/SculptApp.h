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
#ifndef __SCULPT_APP_H__
#define __SCULPT_APP_H__
#include "StandardLib/Object.h"
#include "StandardLib/Pointer.h"
#include "SupportLib/FontColours.h"
#include "InputLib/Input.h"
#include "InputLib/NativeInput.h"
#include "WindowLib/ComponentInput.h"
#include "WindowLib/Dialog.h"
#include "WindowLib/GridContainer.h"
#include "WindowLib/MenuBar.h"
#include "WindowLib/NativeWindowFactory.h"
#include "WindowLib/Window.h"
#include "WindowLib/WindowInputSource.h"
#include "WindowLib/WindowTheme.h"


#define SCULPT_SNAPSHOT_TICKS	10	//Ticks before HCP65_SCULPT_SNAPSHOT's image is saved, so everything has been laid out and drawn.


//HCP65 Sculpt: its window (a menu bar over its page), the menu's items and the input that drives it.  main makes it, adds
//it to the root, runs it and removes it from the root.
//
//What a menu item asks for is done from the window's tick (Tick), not from the click, as a dialog reads the input itself.
//
//From the environment: HCP65_SCULPT_SNAPSHOT=file saves the window as a PNG there once it's been drawn, and
//HCP65_SCULPT_EXIT then closes it, which is how it's looked at on the reference screen (HCP65SculptRef).
class CSculptApp : public CObject
{
CONSTRUCTABLE(CSculptApp);
DESTRUCTABLE(CSculptApp);
protected:
	Ptr<CWindow>				mpWindow;
	Ptr<CFontColours>			mpFontColours;  //msDialogLook's dim font is one of its fonts.
	Ptr<CWindowTheme>			mpTheme;  //msDialogLook's theme.
	Ptr<CGridContainer>			mpMain;  //The menu bar over the page.
	Ptr<CMenuBar>				mpMenuBar;
	Ptr<CMenuItem>				mpExit;
	SDialogLook					msDialogLook;
	Ptr<CInput>					mpInput;
	Ptr<CWindowInputSource>		mpInputSource;
	CComponentInput				mcComponentInput;
	bool						mbValid;
	bool						mbInputs;  //The menu's listener and the component input are set up.
	bool						mbExit;  //File's Exit was clicked: the window stops at the next tick.
	int32						miSnapshotTicks;  //Ticks until the snapshot is saved, -1 once it has been or if none is asked for.

public:
	void	Init(void);
	bool	Setup(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory);
	void	Class(void) override;
	void	Free(void) override;

	bool	Save(CObjectWriter* pcFile) override;
	bool	Load(CObjectReader* pcFile) override;

	bool	Run(void);
	void	Tick(Ptr<CWindow> pWindow);
	void	MenuItemClicked(CMenuItem* pcItem);

protected:
	void	MakeMenus(void);
	void	Snapshot(Ptr<CWindow> pWindow);
};


void	InitSculptGlobals(void);
void	KillSculptGlobals(void);
bool	RunSculptApp(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory);


#endif // __SCULPT_APP_H__

