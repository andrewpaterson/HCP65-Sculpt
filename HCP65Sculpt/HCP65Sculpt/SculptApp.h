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
#include "WindowLib/FileChooser.h"
#include "WindowLib/GridContainer.h"
#include "WindowLib/MenuBar.h"
#include "WindowLib/NativeWindowFactory.h"
#include "WindowLib/Window.h"
#include "WindowLib/WindowInputSource.h"
#include "WindowLib/WindowTheme.h"
#include "SculptLib/SculptPage.h"


#define SCULPT_SNAPSHOT_TICKS	10	//Ticks before HCP65_SCULPT_SNAPSHOT's image is saved, so everything has been laid out and drawn.


//What a menu item asked for that the window's tick does, as each shows a dialog.
enum ESculptAsked
{
	SA_Nothing,
	SA_Open,		//Asks for a Sculpt file, or a material's, and opens it.
	SA_Save,		//Saves to the file last opened or saved to; asks for one if there's none.
	SA_SaveAs,		//Asks for a file and saves to it.
	SA_Export,		//Asks for a file and writes the material alone to it, as another program reads it.
};


//HCP65 Sculpt: its window (a menu bar over its page, SculptLib's CSculptPage), the menu's items and the input that drives it.  main makes it, adds
//it to the root, runs it and removes it from the root.
//
//What a menu item asks for is done from the window's tick (Tick), not from the click, as a dialog reads the input itself:
//File's Open, Save, Save As and Export Material, which ask for a file.
//
//The page's bakes are made on a thread of their own (SculptLib's CSculptBaker), so the window is drawn, and answers,
//without waiting for one: the objects' locking is on while the app runs.  The baker is stopped when the window closes.
//
//From the environment: HCP65_SCULPT_SNAPSHOT=file saves the window as a PNG there once it's been drawn, and
//HCP65_SCULPT_EXIT then closes it, which is how it's looked at on the reference screen (HCP65SculptRef).
//HCP65_SCULPT_NO_BAKER has the page bake on the window's thread, as it did before there was a baker.
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
	Ptr<CMenuItem>				mpOpen;
	Ptr<CMenuItem>				mpSave;
	Ptr<CMenuItem>				mpSaveAs;
	Ptr<CMenuItem>				mpExport;
	Ptr<CImageCel>				mpFolderIcon;  //The file chooser's.
	Ptr<CImageCel>				mpFileIcon;
	Ptr<CFileChooser>			mpChooser;  //While Open's, Save As's or Export's is shown.
	CChars						mszFile;  //The Sculpt file last opened or saved to, empty if none has been.
	ESculptAsked				meAsked;  //What to do at the next tick.
	Ptr<CMenuContainer>			mpNewMenu;  //A material to start from: its items are in SculptMaterials.h's order.
	Ptr<CMenuItem>				mpWeather;
	Ptr<CMenuItem>				mpUndo;
	Ptr<CMenuItem>				mpRedo;
	Ptr<CSculptPage>			mpPage;
	Ptr<CSculptBaker>			mpBaker;  //Whose thread bakes for the page; NULL if the page bakes itself.
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
	bool	OpenFile(const char* szFilename);
	void	Class(void) override;
	void	Free(void) override;

	bool	Save(CObjectWriter* pcFile) override;
	bool	Load(CObjectReader* pcFile) override;

	bool	Run(void);
	void	Tick(Ptr<CWindow> pWindow);
	void	MenuItemClicked(CMenuItem* pcItem);
	void	NewMenuItemClicked(CMenuItem* pcItem);

protected:
	void	MakeMenus(void);
	void	Snapshot(Ptr<CWindow> pWindow);
	void	DoAsked(void);
	bool	ChooseFile(const char* szTitle, bool bSave, const char* szStart, CChars* pszChosen);
	void	Report(const char* szMessage, const char* szFilename);
};


void	InitSculptGlobals(void);
void	KillSculptGlobals(void);
bool	RunSculptApp(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory, const char* szFilename);


#endif // __SCULPT_APP_H__

