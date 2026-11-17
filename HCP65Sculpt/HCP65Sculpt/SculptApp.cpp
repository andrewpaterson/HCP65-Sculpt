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
#include <stdlib.h>
#include "BaseLib/FileUtil.h"
#include "BaseLib/Logger.h"
#include "BaseLib/TypeNames.h"
#include "BaseLib/TypeConverter.h"
#include "BaseLib/GlobalMemory.h"
#include "BaseLib/GlobalDataTypesIO.h"
#include "StandardLib/ClassDefines.h"
#include "StandardLib/Objects.h"
#include "SupportLib/PNGWriter.h"
#include "SupportLib/TrueTypeFontFactory.h"
#include "WindowLib/Canvas.h"
#include "WindowLib/ClearCanvasDraw.h"
#include "WindowLib/JewelTheme.h"
#include "SculptTick.h"
#include "SculptApp.h"


#define SCULPT_SNAPSHOT_VARIABLE	"HCP65_SCULPT_SNAPSHOT"
#define SCULPT_EXIT_VARIABLE		"HCP65_SCULPT_EXIT"


//////////////////////////////////////////////////////////////////////////
//
// Before it is set up.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Init(void)
{
	PreInit();

	memset(&msDialogLook, 0, sizeof(SDialogLook));
	mbValid = false;
	mbInputs = false;
	mbExit = false;
	miSnapshotTicks = (getenv(SCULPT_SNAPSHOT_VARIABLE) != NULL) ? SCULPT_SNAPSHOT_TICKS : -1;

	PostInit();
}


//////////////////////////////////////////////////////////////////////////
//
// The window, its menu and its input.  The devices are described by the
// XML in szInputDirectory; pcNativeInput (the platform's keyboard and
// mouse) is added to them.  Run fails if anything here did.  Call it
// once the app is in the root, so that what is made here is built under
// the root rather than as an unrooted graph.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::Setup(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory)
{
	Ptr<CFont>				pWhite;
	Ptr<CClearCanvasDraw>	pBackground;

	pBackground = OMalloc<CClearCanvasDraw>(0);  //Coloured once there is a theme.
	mpWindow = OMalloc<CWindow>("HCP65 Sculpt", pcNativeFactory, OMalloc<CSculptTick>(this), pBackground, CFT_RGBX, CCO_BGR, CRGB_24bit, ARGB_8bit);
	mpWindow->SetPartialRedraw(true);

	//The font is white and coloured as it is drawn.
	pWhite = ReadTrueTypeFont("Resources/InterVariable.ttf", "Inter 13px", 13.0f, mpWindow->GetColourOrder());
	mbValid = pWhite.IsNotNull();
	if (mbValid)
	{
		mpFontColours = OMalloc<CFontColours>(pWhite);
		mpTheme = CJewelTheme::CreateDarkTheme(mpWindow, mpFontColours);
		pBackground->SetColour(mpTheme->GetColour(TC_Panel));
		msDialogLook.pcTheme = &mpTheme;
		msDialogLook.pcDimFont = &mpFontColours->GetFont(mpTheme->GetColour(TC_Icon));

		MakeMenus();
		mpMain = OMalloc<CGridContainer>(mpWindow);
		mpMain->AddComponent(mpMenuBar, 0, 0, 1, 1, 1, 0);
		mpWindow->SetContainer(mpMain);

		mpInput = OMalloc<CInput>((CNativeInput*)NULL);
		mbValid = mpInput->Read((char*)szInputDirectory);
		mpInput->AddNativeInput(pcNativeInput);
		mpInputSource = OMalloc<CWindowInputSource>(mpInput);
		mcComponentInput.Init(&mpInputSource, &mpWindow);
		mbInputs = true;
		mcComponentInput.AddKeyboards();
		mcComponentInput.AddMice();
		mpWindow->SetComponentInput(&mcComponentInput);

		mpWindow->SetWindowSize(1280, 800);
	}
	return mbValid;
}


//////////////////////////////////////////////////////////////////////////
//
// The menu bar along the top: File has Exit.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::MakeMenus(void)
{
	Ptr<CMenuContainer>	pFileMenu;

	mpMenuBar = OMalloc<CMenuBar>(mpWindow, mpTheme);
	mpMenuBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	pFileMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpMenuBar->AddMenu("File", pFileMenu);
	mpExit = pFileMenu->AddItem("Exit");
	mpMenuBar->GetItemClickEvent()->Add(this, &CSculptApp::MenuItemClicked);
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Class(void)
{
	M_Pointer(mpWindow);
	M_Pointer(mpFontColours);
	M_Pointer(mpTheme);
	M_Pointer(mpMain);
	M_Pointer(mpMenuBar);
	M_Pointer(mpExit);
	U_Data(SDialogLook, msDialogLook);
	M_Pointer(mpInput);
	M_Pointer(mpInputSource);
	U_Data(CComponentInput, mcComponentInput);
	U_Bool(mbValid);
	U_Bool(mbInputs);
	U_Bool(mbExit);
	U_Int32(miSnapshotTicks);
}


//////////////////////////////////////////////////////////////////////////
//
// Only this object's own members: the window and the input may already
// be freed.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Free(void)
{
	if (mbInputs)
	{
		mcComponentInput.Kill();
	}
	msDialogLook.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::Save(CObjectWriter* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::Load(CObjectReader* pcFile)
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// Runs the window until it is closed (if Setup worked), then takes the
// menu's listener and the component input out of the window.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::Run(void)
{
	if (mbValid)
	{
		mpWindow->Show();
	}

	if (mbInputs)
	{
		mpMenuBar->GetItemClickEvent()->Remove(this);
		mpWindow->SetComponentInput(NULL);
	}
	return mbValid;
}


//////////////////////////////////////////////////////////////////////////
//
// The window's tick: what the menu asked for is done here.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Tick(Ptr<CWindow> pWindow)
{
	if (miSnapshotTicks >= 0)
	{
		Snapshot(pWindow);
	}
	if (mbExit)
	{
		pWindow->Stop();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// Counts down to the snapshot, keeping the window ticking, then saves
// what the window shows, and exits if that's asked for too.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Snapshot(Ptr<CWindow> pWindow)
{
	CFileUtil		cFileUtil;
	const char*		szFilename;

	if (miSnapshotTicks > 0)
	{
		//The window would otherwise wait for input before the next tick.
		pWindow->Invalidate();
		pWindow->RequestTick(0);
	}
	else
	{
		szFilename = getenv(SCULPT_SNAPSHOT_VARIABLE);
		cFileUtil.TouchDir(szFilename, true);
		SavePNG(pWindow->GetCanvas()->GetImageOrNull(), (char*)szFilename);
		mbExit = mbExit || (getenv(SCULPT_EXIT_VARIABLE) != NULL);
	}
	miSnapshotTicks--;
}


//////////////////////////////////////////////////////////////////////////
//
// Only asks: the window's tick does it.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::MenuItemClicked(CMenuItem* pcItem)
{
	if (&mpExit == pcItem)
	{
		mbExit = true;
	}
}


//////////////////////////////////////////////////////////////////////////
//
// What every Codaphela program sets up before it makes objects, and
// takes down after.
//
//////////////////////////////////////////////////////////////////////////
void InitSculptGlobals(void)
{
	gcLogger.Init();
	TypesInit();
	TypeConverterInit();
	MemoryInit();
	DataIOInit();
	ObjectsInit();
}


//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////
void KillSculptGlobals(void)
{
	ObjectsFlush();
	ObjectsKill();
	DataIOKill();
	MemoryKill();
	TypeConverterKill();
	TypesKill();
	gcLogger.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// The app (its window and input) is held by the root while it runs on
// the host's window factory and input.  szInputDirectory has
// WinInputLib's XML device descriptions.
//
//////////////////////////////////////////////////////////////////////////
bool RunSculptApp(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory)
{
	Ptr<CSculptApp>	pApp;
	bool			bResult;

	//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
	gcObjects.DisableValidation();

	pApp = OMalloc<CSculptApp>();
	ORoot()->Add(pApp);
	bResult = pApp->Setup(pcNativeFactory, pcNativeInput, szInputDirectory) && pApp->Run();
	ORoot()->Remove(pApp);
	pApp = NULL;

	gcObjects.EnableValidation();
	gcObjects.ValidateObjectsConsistency();
	return bResult;
}

