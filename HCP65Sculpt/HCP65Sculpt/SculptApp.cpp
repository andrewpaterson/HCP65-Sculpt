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
#include "WindowLib/JewelImageGenerator.h"
#include "WindowLib/JewelTheme.h"
#include "WindowLib/MessageDialog.h"
#include "SculptLib/SculptMaterials.h"
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
	mszFile.Init();
	meAsked = SA_Nothing;
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

		mpPage = OMalloc<CSculptPage>(mpWindow, &msDialogLook, mpFontColours);
		MakeMenus();
		mpMain = OMalloc<CGridContainer>(mpWindow);
		mpMain->AddComponent(mpMenuBar, 0, 0, 1, 1, 1, 0);
		mpMain->AddComponent(mpPage, 0, 1, 1, 1, 1, 1);
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
		mpPage->GetView()->FocusView();
	}
	return mbValid;
}


//////////////////////////////////////////////////////////////////////////
//
// A Sculpt file, or a material's, opened as File's Open would: it's the
// file Save then saves to.  False if it's neither.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::OpenFile(const char* szFilename)
{
	if (!mpPage->Open(szFilename))
	{
		return false;
	}
	mszFile.Set(szFilename);
	return true;
}


//////////////////////////////////////////////////////////////////////////
//
// The menu bar along the top: File has New (a material to start from),
// Open, Save and Save As (a Sculpt file), Export Material (the material
// alone, for another program), Weather (the material there is, weathered
// on the shape shown) and Exit; Edit has Undo and Redo of the material's graph; and Add the nodes
// that can be added to it (the page's nodes make it and take its clicks).
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::MakeMenus(void)
{
	Ptr<CMenuContainer>	pFileMenu;
	Ptr<CMenuContainer>	pEditMenu;
	CColourFormatHelper	cFormat;
	size				i;

	mpWindow->GetColourFormat(&cFormat);
	mpFolderIcon = CJewelImageGenerator::CreateFolder(mpTheme->GetColour(TC_Icon), &cFormat);
	mpFileIcon = CJewelImageGenerator::CreateFile(mpTheme->GetColour(TC_Icon), &cFormat);

	mpMenuBar = OMalloc<CMenuBar>(mpWindow, mpTheme);
	mpMenuBar->SetAlignment(CSH_Stretch, CSV_Stretch);
	pFileMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpMenuBar->AddMenu("File", pFileMenu);
	mpNewMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	for (i = 0; i < NumSculptMaterials(); i++)
	{
		mpNewMenu->AddItem(GetSculptMaterialName(i));
	}
	mpNewMenu->GetItemClickEvent()->Add(this, &CSculptApp::NewMenuItemClicked);
	pFileMenu->AddItem("New", NULL, mpNewMenu);
	mpOpen = pFileMenu->AddItem("Open...", mpFileIcon);
	mpSave = pFileMenu->AddItem("Save");
	mpSaveAs = pFileMenu->AddItem("Save As...");
	mpExport = pFileMenu->AddItem("Export Material...");
	pFileMenu->AddSeparator();
	mpWeather = pFileMenu->AddItem("Weather");
	pFileMenu->AddSeparator();
	mpExit = pFileMenu->AddItem("Exit");
	pEditMenu = OMalloc<CMenuContainer>(mpWindow, mpTheme);
	mpMenuBar->AddMenu("Edit", pEditMenu);
	mpUndo = pEditMenu->AddItem("Undo");
	mpRedo = pEditMenu->AddItem("Redo");
	mpMenuBar->AddMenu("Add", mpPage->GetNodes()->MakeAddMenu());
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
	M_Pointer(mpOpen);
	M_Pointer(mpSave);
	M_Pointer(mpSaveAs);
	M_Pointer(mpExport);
	M_Pointer(mpFolderIcon);
	M_Pointer(mpFileIcon);
	M_Pointer(mpChooser);
	U_String(mszFile);
	U_Enum(meAsked);
	M_Pointer(mpNewMenu);
	M_Pointer(mpWeather);
	M_Pointer(mpUndo);
	M_Pointer(mpRedo);
	M_Pointer(mpPage);
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
	mszFile.Kill();
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
		mpNewMenu->GetItemClickEvent()->Remove(this);
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
	DoAsked();
	if (mbExit)
	{
		pWindow->Stop();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// What a menu item asked for is done, once.  A file that can't be read
// or written is said.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::DoAsked(void)
{
	ESculptAsked	eAsked;
	CChars			szChosen;

	eAsked = meAsked;
	meAsked = SA_Nothing;
	if (eAsked == SA_Nothing)
	{
		return;
	}

	szChosen.Init();
	if (eAsked == SA_Open)
	{
		if (ChooseFile("Open", false, mszFile.Text(), &szChosen))
		{
			if (!OpenFile(szChosen.Text()))
			{
				Report("Couldn't open it: it isn't a Sculpt file or a material's.", szChosen.Text());
			}
		}
	}
	else if (eAsked == SA_Export)
	{
		if (ChooseFile("Export Material", true, mszFile.Text(), &szChosen) && !mpPage->ExportMaterial(szChosen.Text()))
		{
			Report("Couldn't write the material.", szChosen.Text());
		}
	}
	else if ((eAsked == SA_Save) && !mszFile.Empty())
	{
		if (!mpPage->Save(mszFile.Text()))
		{
			Report("Couldn't save.", mszFile.Text());
		}
	}
	else if (ChooseFile("Save As", true, mszFile.Text(), &szChosen))
	{
		if (mpPage->Save(szChosen.Text()))
		{
			mszFile.Set(szChosen.Text());
		}
		else
		{
			Report("Couldn't save.", szChosen.Text());
		}
	}
	szChosen.Kill();
}


//////////////////////////////////////////////////////////////////////////
//
// A file chooser, starting at szStart: the file chosen, or false if it
// was cancelled.
//
//////////////////////////////////////////////////////////////////////////
bool CSculptApp::ChooseFile(const char* szTitle, bool bSave, const char* szStart, CChars* pszChosen)
{
	static SFileChooserType	asTypes[] =
	{
		{ "Sculpt and Material Files (*.DRG)", "DRG" },
		{ "All Files", NULL },
	};
	bool	bChosen;

	mpChooser = OMalloc<CFileChooser>(mpWindow, &msDialogLook, szTitle, bSave, szStart, asTypes, sizeof(asTypes) / sizeof(SFileChooserType), mpFolderIcon, mpFileIcon);
	bChosen = mpChooser->Show();
	if (bChosen)
	{
		pszChosen->Set(mpChooser->GetChosen());
	}
	mpChooser = NULL;
	return bChosen;
}


//////////////////////////////////////////////////////////////////////////
//
// Says what went wrong with a file, in a dialog.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::Report(const char* szMessage, const char* szFilename)
{
	Ptr<CMessageDialog>	pMessage;
	CChars				szText;

	szText.Init(szMessage);
	szText.Append("\n");
	szText.Append(szFilename);
	pMessage = OMalloc<CMessageDialog>(mpWindow, &msDialogLook, "HCP65 Sculpt", szText.Text());
	pMessage->Show();
	szText.Kill();
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
// Exit and the file items only ask: the window's tick does them.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::MenuItemClicked(CMenuItem* pcItem)
{
	if (&mpExit == pcItem)
	{
		mbExit = true;
	}
	else if (&mpOpen == pcItem)
	{
		meAsked = SA_Open;
	}
	else if (&mpSave == pcItem)
	{
		meAsked = SA_Save;
	}
	else if (&mpSaveAs == pcItem)
	{
		meAsked = SA_SaveAs;
	}
	else if (&mpExport == pcItem)
	{
		meAsked = SA_Export;
	}
	else if (&mpWeather == pcItem)
	{
		mpPage->WeatherMaterial();
	}
	else if (&mpUndo == pcItem)
	{
		mpPage->GetNodes()->Undo();
	}
	else if (&mpRedo == pcItem)
	{
		mpPage->GetNodes()->Redo();
	}
}


//////////////////////////////////////////////////////////////////////////
//
// One of File's New's materials was clicked: it's the page's material,
// the item's place in the menu being the material's in the list.
//
//////////////////////////////////////////////////////////////////////////
void CSculptApp::NewMenuItemClicked(CMenuItem* pcItem)
{
	size	i;

	for (i = 0; i < NumSculptMaterials(); i++)
	{
		if (&mpNewMenu->GetItem(i) == pcItem)
		{
			mpPage->SetMaterial(MakeSculptMaterial(i, SCULPT_MATERIAL_SEED));
		}
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
// WinInputLib's XML device descriptions.  szFilename, if it isn't NULL,
// is a file to open to start with: one that can't be is logged, and
// Sculpt starts as it would with none.
//
//////////////////////////////////////////////////////////////////////////
bool RunSculptApp(CNativeWindowFactory* pcNativeFactory, CNativeInput* pcNativeInput, const char* szInputDirectory, const char* szFilename)
{
	Ptr<CSculptApp>	pApp;
	bool			bResult;

	//Validating every object each time one changes is too slow for the text's glyphs.  They are validated once at the end.
	gcObjects.DisableValidation();

	pApp = OMalloc<CSculptApp>();
	ORoot()->Add(pApp);
	bResult = pApp->Setup(pcNativeFactory, pcNativeInput, szInputDirectory);
	if (bResult && (szFilename != NULL) && !pApp->OpenFile(szFilename))
	{
		gcLogger.Error2(__METHOD__, " Could not open [", szFilename, "].", NULL);
	}
	bResult = bResult && pApp->Run();
	ORoot()->Remove(pApp);
	pApp = NULL;

	gcObjects.EnableValidation();
	gcObjects.ValidateObjectsConsistency();
	return bResult;
}

