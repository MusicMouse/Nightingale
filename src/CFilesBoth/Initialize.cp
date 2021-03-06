/*
 *	Initialize.c for Nightingale
 *	All one-time only initialization code is in this file or InitNightingale.c,
 *	so that it can be unloaded by the caller when we're through to reclaim heap
 *	space.
 */

/*
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS PROPERTY OF AVIAN MUSIC
 * NOTATION FOUNDATION. Nightingale is an open-source project, hosted at
 * github.com/AMNS/Nightingale .
 *
 * Copyright © 2016 by Avian Music Notation Foundation. All Rights Reserved.
 */


#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#include "SampleMDEF.h"
#include "GraphicMDEF.h"
#include "EndianUtils.h"

/* Private routines */

static void			InitToolbox(void);
static Boolean		AddSetupResource(Handle);
static OSStatus		FindPrefsFile(unsigned char *name, OSType fType, OSType fCreator, FSSpec *prefsSpec);
static void			DebugDisplayCnfg(void);
static Boolean		GetConfig(void);
static Boolean		InitMemory(short numMasters);
static Boolean		NInitFloatingWindows(void);
static void			SetupToolPalette(PaletteGlobals *whichPalette, Rect *windowRect);
static short		GetToolGrid(PaletteGlobals *whichPalette);
static void			SetupPaletteRects(Rect *whichRects, short across, short down, short width,
								short height);
static Boolean		PrepareClipDoc(void);
static void			AddSampleItems(MenuRef menu);
static void			InstallCoreEventHandlers(void);

void				InitNightingale(void);	// FIXME: non-static, so should be in a header


/* ----------------------------------------------------------- Initialize and ally -- */
/* Do everything that must be done prior to entering main event loop. A great deal of
this is platform-dependent. */

static void InitToolbox()
{
#if TARGET_API_MAC_CARBON
	FlushEvents(everyEvent, 0);		// FIXME: DO WE NEED THIS?
#endif
}
		

static GrowZoneUPP growZoneUPP;			/* permanent GZ UPP */


void Initialize()
{
	OSErr err;
	Str63 dummyVolName;
	
	InitToolbox();

	/* Store away resource file and volume refNum and dir ID of Nightingale appl. */

	appRFRefNum = CurResFile();
	dummyVolName[0] = 0;
	err = HGetVol(dummyVolName, &appVRefNum, &appDirID);
	if (err)
		{ BadInit(); ExitToShell(); }
			
	/* We must allocate <strBuf> immediately: it's used to build error messages. */
	
	strBuf = (char *)NewPtr(256);
	if (!GoodNewPtr((Ptr)strBuf))
		{ OutOfMemory(256L); ExitToShell(); }
	
	/*
	 *	FIXME: The following comment describes a trick we used with THINK C that
	 *  I suspect not only no longer works, but isn't even necessary as of
	 *  v.6.0.1--I think starting with TC 6.0.1, we appear to have the correct
	 *  creatorType even under TC. On the other hand, it shouldn't have any
	 *	effect except on Ngale developers running in a development system.
	 *
	 *	For the benefit of AppleEvent handler installation, we attempt to
	 *	figure out whether we are running on our own or under THINK C. 
	 *	AppleEvents addressed to creatorType ('BYRD') never get to us when we
	 *	are running under TC, since it looks to the outside world like we're
	 *	THINK C (signature: 'KAHL').  We use the 'CODE' count at runtime to
	 *	distinguish between the two environments: under THINK C the number of
	 *	CODE resources should be either very small or 0.  In the following,
	 *	16 is an arbitrary threshold. (Doug had 32, which I don't think qualifies
	 *	as very small; in fact I think MapInfo counts on 0 for this purpose!)
	 *	This may not work under future versions of THINK C, but it did with
	 *	version 5.0.4 (the last version we tried it with).
	 */
	
	creatorType = CREATOR_TYPE_NORMAL;
	documentType = DOCUMENT_TYPE_NORMAL;

	/*
	 *	Figure out our machine environment before doing anything more. This
	 *	can't be done until after the various ToolBox calls above.
	 */	
	thisMac.machineType = -1;
	err = SysEnvirons(1,&thisMac);
	if (err) thisMac.machineType = -1;
			
	/* FIXME: We formerly checked here to see if we're running on Mac OS 7.0 or later.
		Rather pointless now (2016)!; this global should go away completely. */
	appleEventsOK = TRUE;

	if (appleEventsOK) InstallCoreEventHandlers();
	
	InitLogPrintf();

	if (!OpenSetupFile())							/* needs creatorType */
		{ BadInit(); ExitToShell(); }
	GetConfig();									/* needs the Prefs (Setup) file open */
	if (!OpenTextSetupFile())						/* needs creatorType */
		{ BadInit(); ExitToShell(); }
	GetTextConfig();								/* needs the Prefs (Setup) file open */
	
//	char *foo = GetPrefsValue("foo");
//	char *bazz = GetPrefsValue("bazz");

	if (!InitMemory(config.numMasters))				/* needs the CNFG resource */
		{ BadInit(); ExitToShell(); }

#ifdef TARGET_API_MAC_CARBON	
	// WaitNextEvent is always available in OS X
	
	hasWaitNextEvent = TRUE;
#else
	hasWaitNextEvent = TrapAvailable(_WaitNextEvent);
#endif
	/*
	 *	Allocate a large grow zone handle, which gets freed when we run out of memory,
	 *	so that whoever is asking for memory will probably get what they need without
	 *	our having to crash or die prematurely except in the rarest occasions. (The
	 *	"rainy day" memory fund.)
	 */
	memoryBuffer = NewHandle(config.rainyDayMemory*1024L);
	if (!GoodNewHandle(memoryBuffer)) {
		OutOfMemory(config.rainyDayMemory*1024L);
		BadInit();
		ExitToShell();
	}
	outOfMemory = FALSE;
	growZoneUPP = NewGrowZoneUPP(GrowMemory);
	if (growZoneUPP)
		SetGrowZone(growZoneUPP);			/* Install simple grow zone function */
	
	if (!NInitFloatingWindows())
		{ BadInit(); ExitToShell(); }
	
	InitCursor();
	WaitCursor();

	/* Do Application-specific initialization */
	
	if (!InitGlobals())
		{ BadInit(); ExitToShell(); }
	Init_Help();
	
	/*
	 * See if we have enough memory that the user should be able to do
	 * SOMETHING useful--and enough to get back to the main event loop, where
	 * we do our regular low-memory checking. As of v.999, 250K was enough, but
	 * make the minimum a lot higher.
	 */
	if (!PreflightMem(400))
		{ BadInit(); ExitToShell(); }
}


static OSStatus FindPrefsFile(unsigned char *fileName, OSType fType, OSType fCreator, FSSpec *prefsSpec) 
{
	short pvol;
	long pdir;
	Str255 name;
	CInfoPBRec cat;
	OSStatus err;
	
	/* Find the preferences folder, normally ~/Library/Preferences */
	err = FindFolder(kOnSystemDisk, kPreferencesFolderType, true, &pvol, &pdir);
	LogPrintf(LOG_NOTICE, "Prefs File: FindFolder: err=%d\n", err);
	if (err!=noErr) return err;
	
	/* Search the folder for the file */
	BlockZero(&cat, sizeof(cat));
	cat.hFileInfo.ioNamePtr = name;
	cat.hFileInfo.ioVRefNum = pvol;
	cat.hFileInfo.ioFDirIndex = 1;
	cat.hFileInfo.ioDirID = pdir;
	
	while (PBGetCatInfoSync(&cat) == noErr) {
		if (( (cat.hFileInfo.ioFlAttrib & 16) == 0) &&
				(cat.hFileInfo.ioFlFndrInfo.fdType == fType) &&
				(cat.hFileInfo.ioFlFndrInfo.fdCreator == fCreator) &&
				(PStrCmp(name, fileName) == TRUE)) {
			// make a fsspec referring to the file
			return FSMakeFSSpec(pvol, pdir, name, prefsSpec);
		}
		
		cat.hFileInfo.ioFDirIndex++;
		cat.hFileInfo.ioDirID = pdir;
	}
	
	err = FSMakeFSSpec(pvol, pdir, PREFS_PATH, prefsSpec);
	LogPrintf(LOG_NOTICE, "Prefs File: FSMakeFSSpec: err=%d (fnfErr=%d)\n", err, fnfErr);
	return fnfErr;		// FIXME: HUH? ALWAYS RETURN AN ERROR? ISN'T THIS "FILE NOT FOUND"?
} 


/* -------------------------------------------------------------- CreateSetupFile -- */
/* Create a new Nightingale Prefs file. (We used to call the "Prefs" file the "Setup"
file: hence all the xxSetupxx names.) If we succeed, return TRUE; if we fail, return
FALSE without giving an error message. */

static short rfVRefNum;
static long rfVolDirID;
static OSType prefsFileType = 'NSET';

Boolean CreateSetupFile(FSSpec *rfSpec)
{
	Handle			resH;
	OSType			theErr;
	short			nSPTB, i;
	ScriptCode		scriptCode = smRoman;
//	FSSpec			rfSpec;
	
	/* Create a new file in the ~/Library/Preferences and give it a resource fork.
		If there is no Preferences folder in ~/Library, create it. */
	
//	theErr = Create(, thisMac.sysVRefNum, creatorType, 'NSET'); /* Create new file */
//	theErr = FSMakeFSSpec(rfVRefNum, rfVolDirID, "\pNightingale 2001 Prefs", &rfSpec);

	Pstrcpy(rfSpec->name, SETUP_FILE_PATH);	
	FSpCreateResFile(rfSpec, creatorType, 'NSET', scriptCode);
	theErr = ResError();
	if (theErr) return FALSE;

//	CreateResFile(SETUP_FILE_NAME);
//	theErr = ResError();
	if (theErr!=noErr) return FALSE;
	
	/* Open it, setting global setupFileRefNum.  NB: HOpenResFile makes the file it
		opens the current resource file. */
		
	//setupFileRefNum = OpenResFile(SETUP_FILE_NAME);
	setupFileRefNum = FSpOpenResFile(rfSpec, fsRdWrPerm);
	
	theErr = ResError();
	if (theErr!=noErr) return FALSE;				/* Otherwise we'll write in the app's resource fork! */
	
	/*
	 * Get various resources and copy them to the new Prefs file.  Since the Prefs
	 * file has nothing in it yet, GetResource will look thru the other open resource
	 * files for each resource we Get, and presumably find them in the application.
	 * First copy the config and its template.
	 */	
	resH = GetResource('CNFG', THE_CNFG);
	if (!GoodResource(resH)) return FALSE;	
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetNamedResource('TMPL', "\pCNFG");
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Now copy the instrument list. */
	resH = GetResource('STR#', INSTR_STRS);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Next, the MIDI velocity table and its template. */
	resH = GetResource('MIDI', PREFS_MIDI);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetNamedResource('TMPL', "\pMIDI");
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* The MIDI modifier prefs table and its template. */
	resH = GetResource('MIDM', PREFS_MIDI_MODNR);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetNamedResource('TMPL', "\pMIDM");
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Now copy the palette, palette char. map, and palette translation map and template. */
	resH = GetResource('PICT', ToolPaletteID);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetResource('PLCH', ToolPaletteID);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetResource('PLMP', THE_PLMP);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;
	
	resH = GetNamedResource('TMPL', "\pPLMP");
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Now copy the MIDI Manager port resources. */
	resH = GetResource('port', time_port);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;
	
	resH = GetResource('port', input_port);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	resH = GetResource('port', output_port);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* And finally copy the built-in MIDI parameter resource. */
	resH = GetResource('BIMI', THE_BIMI);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Nightingale Prefs uses the same version number resource as Nightingale. */
	resH = GetResource('vers', 1);
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Copy all the spacing tables we can find into the Prefs file. We use
	 * Get1IndResource instead of GetIndResource to avoid getting a resource
	 * from the Prefs file we've just written out! Of course, this means we
	 * have to be sure the current resource file is the application; we're
	 * relying on AddSetupResource to take care of that. Also copy the template.
	 */
	nSPTB = CountResources('SPTB');
	for (i = 1; i<=nSPTB; i++) {
		resH = Get1IndResource('SPTB', i);
		if (!GoodResource(resH)) return FALSE;		
		if (!AddSetupResource(resH)) return FALSE;
	}
	
	resH = GetNamedResource('TMPL', "\pSPTB");
	if (!GoodResource(resH)) return FALSE;		
	if (!AddSetupResource(resH)) return FALSE;

	/* Finally, close the setup file. */

	CloseResFile(setupFileRefNum);
	
	return TRUE;
}

/* ---------------------------------------------------------------- OpenSetupFile -- */
/* Open the resource fork of the Prefs file.  If we can't find a Prefs file in the
System Folder, generate a new one using resources from the application and open it.
Also make the Prefs file the current resource file.

If we find a problem, including the Prefs file resource fork already open with
write permission, we give an error message and return FALSE. (If the Prefs file
resource fork is open but with only read permission, I suspect we won't detect that
and it won't be made the current resource file--bad but unlikely. Note that
NightCustomizer doesn't keep the Prefs file open while it's running.) If all goes
well, return TRUE. */

Boolean OpenSetupFile()
{
	OSErr result; Boolean okay=TRUE; Str63 volName;
	StringHandle strHdl; char fmtStr[256];
	Str255 setupFileName;

	short oldVRefNum; long oldDirID;
	short vRefNum; long dirID;

	FSSpec rfSpec; OSErr theErr;
	
//	short oldVol; 
//	GetVol(volName,&oldVol);
//	SetVol(volName,thisMac.sysVRefNum);		/* fromSysEnvirons */

	theErr = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &vRefNum, &dirID);
	
	HGetVol(volName,&oldVRefNum,&oldDirID);
	HSetVol(volName,vRefNum,dirID);					// Need to specify a location for setup file

	rfVRefNum = vRefNum;
	rfVolDirID = dirID;
	
	/* Try to open the Prefs file. Under "classic" Mac OS (9 and before), it was normally
	in  the System Folder; under OS X, it's in ~/Library/Preferences . */
	
	Pstrcpy(setupFileName, SETUP_FILE_NAME);
	theErr = FindPrefsFile(setupFileName, prefsFileType, creatorType, &rfSpec);
	LogPrintf(LOG_NOTICE, "OpenSetupFile: FindPrefsFile (filename '%s') returned theErr=%d\n",
				PToCString(setupFileName), theErr);
	if (theErr==noErr)
		setupFileRefNum = FSpOpenResFile(&rfSpec, fsRdWrPerm);
	
	/* If we can't open it, create a new one using app's own CNFG and other resources. */
	if (theErr==noErr)
		result = ResError();
	else
		result = theErr;
		
//	if (result==fnfErr || result==dirNFErr) {
	if (result==fnfErr) {
		/* Note that the progress message has the setup filename embedded in it: this is
		 * lousy--it should use SETUP_FILE_NAME, of course--but ProgressMsg() can't
		 * handle that!
		 */
		LogPrintf(LOG_NOTICE, "Can\'t find a '%s' (Preferences) file: creating a new one.\n", PToCString(setupFileName));
		ProgressMsg(CREATESETUP_PMSTR, "");
		SleepTicks((unsigned)(5*60L));						/* Give user time to read the msg */
		if (!CreateSetupFile(&rfSpec)) {
			GetIndCString(strBuf, INITERRS_STRS, 2);	/* "Can't create Prefs file" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			okay = FALSE;
			goto done;
		}
		/* Try opening the brand-new Prefs file. */
		setupFileRefNum = FSpOpenResFile(&rfSpec, fsRdWrPerm);

		ProgressMsg(0, "");
	}

	result = ResError();								/* Careful: this is not redundant! */
	if (result!=noErr) {
		strHdl = GetString(result);
		if (strHdl) {
			Pstrcpy((unsigned char *)strBuf, (unsigned char *)*strHdl);
			PToCString((unsigned char *)strBuf);
		}
		else {
			GetIndCString(fmtStr, INITERRS_STRS, 3);	/* "Resource Manager error %d" */
			sprintf(strBuf, fmtStr, result);
		}
		CParamText(strBuf, "", "", "");
		StopInform(OPENPREFS_ALRT);
		okay = FALSE;
	}
	
done:
//	SetVol(volName,oldVol);
	HSetVol(volName,oldVRefNum,oldDirID);
	return okay;
}

/* Adds the resource pointed to by resH to the current resource file. Returns
TRUE if OK, FALSE if error. */

static Boolean AddSetupResource(Handle resH)
{
	Handle	tempH;
	OSType	type;
	short	id;
	Str255	name;
	
	UseResFile(setupFileRefNum);				/* safeguard against writing in app's res fork */

	GetResInfo(resH, &id, &type, name);
	if (ReportResError()) return FALSE;
	
	tempH = resH;
	HandToHand(&tempH);	
	ReleaseResource(resH);
	if (ReportResError()) return FALSE;
	AddResource(tempH, type, id, name);
	if (ReportResError()) return FALSE;
	WriteResource(tempH);
	if (ReportResError()) return FALSE;
	ReleaseResource(tempH);
	if (ReportResError()) return FALSE;
	
	UseResFile(appRFRefNum);
	return TRUE;
}


static void DebugDisplayCnfg()
{
	LogPrintf(LOG_NOTICE, "Displaying CNFG:\n");
	LogPrintf(LOG_NOTICE, "  (1)maxDocuments=%d", config.maxDocuments);
	LogPrintf(LOG_NOTICE, "  (2)stemLenNormal=%d", config.stemLenNormal);
	LogPrintf(LOG_NOTICE, "  (3)stemLen2v=%d", config.stemLen2v);
	LogPrintf(LOG_NOTICE, "  (4)stemLenOutside=%d\n", config.stemLenOutside);
	LogPrintf(LOG_NOTICE, "  (5)stemLenGrace=%d", config.stemLenGrace);

	LogPrintf(LOG_NOTICE, "  (6)spAfterBar=%d", config.spAfterBar);
	LogPrintf(LOG_NOTICE, "  (7)minSpBeforeBar=%d", config.minSpBeforeBar);
	LogPrintf(LOG_NOTICE, "  (8)minRSpace=%d\n", config.minRSpace);
	LogPrintf(LOG_NOTICE, "  (9)minLyricRSpace=%d", config.minLyricRSpace);
	LogPrintf(LOG_NOTICE, "  (10)minRSpace_KSTS_N=%d", config.minRSpace_KSTS_N);

	LogPrintf(LOG_NOTICE, "  (11)hAccStep=%d", config.hAccStep);

	LogPrintf(LOG_NOTICE, "  (12)stemLW=%d\n", config.stemLW);
	LogPrintf(LOG_NOTICE, "  (13)barlineLW=%d", config.barlineLW);
	LogPrintf(LOG_NOTICE, "  (14)ledgerLW=%d", config.ledgerLW);
	LogPrintf(LOG_NOTICE, "  (15)staffLW=%d", config.staffLW);
	LogPrintf(LOG_NOTICE, "  (16)enclLW=%d\n", config.enclLW);

	LogPrintf(LOG_NOTICE, "  (17)beamLW=%d", config.beamLW);
	LogPrintf(LOG_NOTICE, "  (18)hairpinLW=%d", config.hairpinLW);
	LogPrintf(LOG_NOTICE, "  (19)dottedBarlineLW=%d", config.dottedBarlineLW);
	LogPrintf(LOG_NOTICE, "  (20)mbRestEndLW=%d\n", config.mbRestEndLW);
	LogPrintf(LOG_NOTICE, "  (21)nonarpLW=%d", config.nonarpLW);

	LogPrintf(LOG_NOTICE, "  (22)graceSlashLW=%d", config.graceSlashLW);
	LogPrintf(LOG_NOTICE, "  (23)slurMidLW=%d", config.slurMidLW);
	LogPrintf(LOG_NOTICE, "  (24)tremSlashLW=%d\n", config.tremSlashLW);
	LogPrintf(LOG_NOTICE, "  (25)slurCurvature=%d", config.slurCurvature);
	LogPrintf(LOG_NOTICE, "  (26)tieCurvature=%d", config.tieCurvature);
	LogPrintf(LOG_NOTICE, "  (27)relBeamSlope=%d", config.relBeamSlope);

	LogPrintf(LOG_NOTICE, "  (28)hairpinMouthWidth=%d\n", config.hairpinMouthWidth);
	LogPrintf(LOG_NOTICE, "  (29)mbRestBaseLen=%d", config.mbRestBaseLen);
	LogPrintf(LOG_NOTICE, "  (30)mbRestAddLen=%d", config.mbRestAddLen);
	LogPrintf(LOG_NOTICE, "  (31)barlineDashLen=%d", config.barlineDashLen);
	LogPrintf(LOG_NOTICE, "  (32)crossStaffBeamExt=%d\n", config.crossStaffBeamExt);
	LogPrintf(LOG_NOTICE, "  (33)titleMargin=%d", config.titleMargin);

	LogPrintf(LOG_NOTICE, "  (35)pageMarg.top=%d", config.pageMarg.top);
	LogPrintf(LOG_NOTICE, "  .left=%d", config.pageMarg.left);
	LogPrintf(LOG_NOTICE, "  .bottom=%d", config.pageMarg.bottom);
	LogPrintf(LOG_NOTICE, "  .right=%d\n", config.pageMarg.right);

	LogPrintf(LOG_NOTICE, "  (37)pageNumMarg.top=%d", config.pageNumMarg.top);
	LogPrintf(LOG_NOTICE, "  .left=%d", config.pageNumMarg.left);
	LogPrintf(LOG_NOTICE, "  .bottom=%d", config.pageNumMarg.bottom);
	LogPrintf(LOG_NOTICE, "  .right=%d\n", config.pageNumMarg.right);

	LogPrintf(LOG_NOTICE, "  (38)defaultLedgers=%d", config.defaultLedgers);
	LogPrintf(LOG_NOTICE, "  (39)defaultTSNum=%d", config.defaultTSNum);
	LogPrintf(LOG_NOTICE, "  (40)defaultTSDenom=%d", config.defaultTSDenom);
	LogPrintf(LOG_NOTICE, "  (41)defaultRastral=%d\n", config.defaultRastral);
	LogPrintf(LOG_NOTICE, "  (42)rastral0size=%d", config.rastral0size);

	LogPrintf(LOG_NOTICE, "  (43)minRecVelocity=%d", config.minRecVelocity);
	LogPrintf(LOG_NOTICE, "  (44)minRecDuration=%d\n", config.minRecDuration);
	LogPrintf(LOG_NOTICE, "  (45)midiThru=%d", config.midiThru);
	LogPrintf(LOG_NOTICE, "  (46)defaultTempoMM=%d", config.defaultTempoMM);

	LogPrintf(LOG_NOTICE, "  (47)lowMemory=%d", config.lowMemory);
	LogPrintf(LOG_NOTICE, "  (48)minMemory=%d\n", config.minMemory);

	LogPrintf(LOG_NOTICE, "  (49)numRows=%d", config.numRows);
	LogPrintf(LOG_NOTICE, "  (50)numCols=%d", config.numCols);
	LogPrintf(LOG_NOTICE, "  (51)maxRows=%d", config.maxRows);
	LogPrintf(LOG_NOTICE, "  (52)maxCols=%d\n", config.maxCols);
	LogPrintf(LOG_NOTICE, "  (53)hPageSep=%d", config.hPageSep);
	LogPrintf(LOG_NOTICE, "  (54)vPageSep=%d", config.vPageSep);
	LogPrintf(LOG_NOTICE, "  (55)hScrollSlop=%d", config.hScrollSlop);
	LogPrintf(LOG_NOTICE, "  (56)vScrollSlop=%d\n", config.vScrollSlop);

	LogPrintf(LOG_NOTICE, "  (57)maxSyncTolerance=%d", config.maxSyncTolerance);
	LogPrintf(LOG_NOTICE, "  (58)enlargeHilite=%d", config.enlargeHilite);
	LogPrintf(LOG_NOTICE, "  (59)infoDistUnits=%d", config.infoDistUnits);
	LogPrintf(LOG_NOTICE, "  (60)mShakeThresh=%d\n", config.mShakeThresh);
	
	LogPrintf(LOG_NOTICE, "  (61)musicFontID=%d", config.musicFontID);
	LogPrintf(LOG_NOTICE, "  (62)numMasters=%d", config.numMasters);
	LogPrintf(LOG_NOTICE, "  (63)indent1st=%d", config.indent1st);
	LogPrintf(LOG_NOTICE, "  (64)mbRestHeight=%d\n", config.mbRestHeight);
	LogPrintf(LOG_NOTICE, "  (65)chordSymMusSize=%d", config.chordSymMusSize);

	LogPrintf(LOG_NOTICE, "  (66)enclMargin=%d", config.enclMargin);
	LogPrintf(LOG_NOTICE, "  (67)legatoPct=%d", config.legatoPct);

	LogPrintf(LOG_NOTICE, "  (68)defaultPatch=%d\n", config.defaultPatch);
	LogPrintf(LOG_NOTICE, "  (69)whichMIDI=%d", config.whichMIDI);

	LogPrintf(LOG_NOTICE, "  (70)musFontSizeOffset=%d", config.musFontSizeOffset);
	LogPrintf(LOG_NOTICE, "  (71)fastScreenSlurs=%d", config.fastScreenSlurs);
	LogPrintf(LOG_NOTICE, "  (72)restMVOffset=%d\n", config.restMVOffset);
	LogPrintf(LOG_NOTICE, "  (73)autoBeamOptions=%d", config.autoBeamOptions);
	
	LogPrintf(LOG_NOTICE, "  (74)noteOffVel=%d", config.noteOffVel);
	LogPrintf(LOG_NOTICE, "  (75)feedbackNoteOnVel=%d", config.feedbackNoteOnVel);
	LogPrintf(LOG_NOTICE, "  (76)defaultChannel=%d\n", config.defaultChannel);
	LogPrintf(LOG_NOTICE, "  (77)enclWidthOffset=%d", config.enclWidthOffset);	
	LogPrintf(LOG_NOTICE, "  (78)rainyDayMemory=%d", config.rainyDayMemory);
	LogPrintf(LOG_NOTICE, "  (79)tryTupLevels=%d", config.tryTupLevels);
	LogPrintf(LOG_NOTICE, "  (80)justifyWarnThresh=%d\n", config.justifyWarnThresh);

	LogPrintf(LOG_NOTICE, "  (81)metroChannel=%d", config.metroChannel);
	LogPrintf(LOG_NOTICE, "  (82)metroNote=%d", config.metroNote);
	LogPrintf(LOG_NOTICE, "  (83)metroVelo=%d", config.metroVelo);
	LogPrintf(LOG_NOTICE, "  (84)metroDur=%d\n", config.metroDur);

	LogPrintf(LOG_NOTICE, "  (85)chordSymSmallSize=%d", config.chordSymSmallSize);
	LogPrintf(LOG_NOTICE, "  (86)chordSymSuperscr=%d", config.chordSymSuperscr);
	LogPrintf(LOG_NOTICE, "  (87)chordSymStkLead=%d\n", config.chordSymStkLead);

	LogPrintf(LOG_NOTICE, "  (88)tempoMarkHGap=%d", config.tempoMarkHGap);
	LogPrintf(LOG_NOTICE, "  (89)trebleVOffset=%d", config.trebleVOffset);
	LogPrintf(LOG_NOTICE, "  (90)cClefVOffset=%d", config.cClefVOffset);
	LogPrintf(LOG_NOTICE, "  (91)bassVOffset=%d\n", config.bassVOffset);

	LogPrintf(LOG_NOTICE, "  (92)tupletNumSize=%d", config.tupletNumSize);
	LogPrintf(LOG_NOTICE, "  (93)tupletColonSize=%d", config.tupletColonSize);
	LogPrintf(LOG_NOTICE, "  (94)octaveNumSize=%d", config.octaveNumSize);
	LogPrintf(LOG_NOTICE, "  (95)lineLW=%d\n", config.lineLW);
	LogPrintf(LOG_NOTICE, "  (96)ledgerLLen=%d", config.ledgerLLen);
	LogPrintf(LOG_NOTICE, "  (97)ledgerLOtherLen=%d", config.ledgerLOtherLen);
	LogPrintf(LOG_NOTICE, "  (98)slurDashLen=%d", config.slurDashLen);
	LogPrintf(LOG_NOTICE, "  (99)slurSpaceLen=%d\n", config.slurSpaceLen);

	LogPrintf(LOG_NOTICE, "  (100)courtesyAccLXD=%d", config.courtesyAccLXD);
	LogPrintf(LOG_NOTICE, "  (101)courtesyAccRXD=%d", config.courtesyAccRXD);
	LogPrintf(LOG_NOTICE, "  (102)courtesyAccYD=%d", config.courtesyAccYD);
	LogPrintf(LOG_NOTICE, "  (103)courtesyAccSize=%d\n", config.courtesyAccSize);

	LogPrintf(LOG_NOTICE, "  (104)quantizeBeamYPos=%d", config.quantizeBeamYPos);
	LogPrintf(LOG_NOTICE, "  (105)enlargeNRHiliteH=%d", config.enlargeNRHiliteH);
	LogPrintf(LOG_NOTICE, "  (106)enlargeNRHiliteV=%d", config.enlargeNRHiliteV);
	LogPrintf(LOG_NOTICE, "\n");
}


/* Install our configuration data from the Prefs file; also check for, report, and
correct any illegal values. Assumes the Prefs file is the current resource file. */

#define ERR(fn) { nerr++; LogPrintf(LOG_NOTICE, " err #%d,", fn); if (firstErr==0) firstErr = fn; }

static Boolean GetConfig()
{
	Handle cnfgH;
	Boolean gotCnfg;
	short nerr, firstErr;
	long cnfgSize;
	char fmtStr[256];
	
	cnfgH = Get1Resource('CNFG',THE_CNFG);
	if (!GoodResource(cnfgH)) {
		GetIndCString(strBuf, INITERRS_STRS, 4);		/* "Can't find the CNFG resource" */
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(CNFG_ALRT)==OK) ExitToShell();
		else							  gotCnfg = FALSE;
		
		/*
		 * Since we couldn't find the configuration data, set all checkable fields to
		 * illegal values so they'll be caught and corrected below; set the few 
		 * uncheckable ones to reasonable values.
		 */
		config.maxDocuments = -1;

		config.stemLenNormal = config.stemLen2v = -1;
		config.stemLenOutside = config.stemLenGrace = -1;

		config.spAfterBar = config.minSpBeforeBar = -1;
		config.minRSpace = config.minLyricRSpace = -1;
		config.minRSpace_KSTS_N = -1;

		config.hAccStep = -1;

		config.stemLW = config.barlineLW = config.ledgerLW = config.staffLW = -1;
		config.enclLW = -1;

		config.beamLW = config.hairpinLW = config.dottedBarlineLW = -1;
		config.mbRestEndLW = config.nonarpLW = -1;

		config.graceSlashLW = config.slurMidLW = config.tremSlashLW = -1;
		config.slurCurvature = config.tieCurvature = -1;
		config.relBeamSlope = -1;
		
		config.hairpinMouthWidth = -1;
		config.mbRestBaseLen = config.mbRestAddLen = -1;
		config.barlineDashLen = 0;
		config.crossStaffBeamExt = -1;
		
		config.slashGraceStems = 0;

		config.titleMargin = -1;

		SetRect(&config.paperRect,0, 0, 0, 0);
		SetRect(&config.pageMarg, 0, 0, 0, 0);
		SetRect(&config.pageNumMarg, 0, 0, 0, 0);
		
		config.defaultLedgers = 0;
		config.defaultTSNum = config.defaultTSDenom = -1;
		config.defaultRastral = -1;
		config.rastral0size = 0;
		
		config.defaultTempoMM = 0;
		config.minRecVelocity = 0;
		config.minRecDuration = 0;
		config.midiThru = -1;
		
		config.lowMemory = config.minMemory = -1;
		
		config.toolsPosition.h = -83;
		config.toolsPosition.v = -8;

		config.numRows = config.numCols = -1;
		config.maxRows = config.maxCols = -1;
		config.hPageSep = config.vPageSep = -1;
		config.hScrollSlop = config.vScrollSlop = -1;
		config.origin.h = config.origin.v = -20000;
		
		config.maxSyncTolerance = -1;
		config.dblBarsBarlines = 0;
		config.enlargeHilite = -1;
		config.delRedundantAccs = 1;
		config.infoDistUnits = -1;
		config.mShakeThresh = -1;

		config.numMasters = -1;
		
		config.indent1st = -1;
		config.chordSymMusSize = 0;
		config.fastScreenSlurs = 0;
		config.legatoPct = -1;
		config.defaultPatch = -1;
		config.whichMIDI = -1;
		config.enclMargin = -1;
		config.mbRestHeight = -1;
		config.musFontSizeOffset = -127;
		config.restMVOffset = -1;
		config.autoBeamOptions = -1;
		config.noteOffVel = -1;
		config.feedbackNoteOnVel = -1;
		config.defaultChannel = -1;
		config.defaultInputDevice = 0;
		config.defaultOutputChannel = -1;
		config.defaultOutputDevice = 0;
		
		config.rainyDayMemory = -1;
		config.tryTupLevels = -1;
		config.justifyWarnThresh = -1;
		
		config.metroChannel = -1;
		config.metroNote = -1;
		config.metroVelo = -1;
		config.metroDur = -1;
		config.metroDevice = 0;
		
		config.chordSymSmallSize = -1;	
		config.chordSymSuperscr = -1;
		config.chordSymStkLead = -127;

		config.tupletNumSize = 110;
		config.tupletColonSize = 60;
		config.octaveNumSize = 120;
		config.lineLW = -1;
		config.ledgerLLen = config.ledgerLOtherLen = -1;
		config.slurDashLen = config.slurSpaceLen = -1;

		config.courtesyAccLXD = config.courtesyAccRXD = -127;
		config.courtesyAccYD = -127;
		config.courtesyAccSize = -127;

		/* FIXME: What about OMS fields (metroDevice thru defaultOutputChannel)? */
		
		config.quantizeBeamYPos = -1;

		config.enlargeNRHiliteH = -1;
		config.enlargeNRHiliteV = -1;

		config.thruChannel = 0;
		config.thruDevice = noUniqueID;
	}
	 else {
		cnfgSize = GetResourceSizeOnDisk(cnfgH);
		if (cnfgSize!=(long)sizeof(Configuration))
			if (CautionAdvise(CNFGSIZE_ALRT)==OK) ExitToShell();
		config = *(Configuration *)(*cnfgH);
		gotCnfg = TRUE;

		if (ControlKeyDown()) {
			GetIndCString(strBuf, INITERRS_STRS, 11);		/* "Skipping checking the CNFG" */
			CParamText(strBuf, "", "", "");
			NoteInform(GENERIC_ALRT);
			goto Finish;
		}

	}

	DebugDisplayCnfg();

	/*
	 * Now do a reality check for values that might be bad. We can't easily check
	 * origin, toolsPosition, musicFontID, or the fields that represent Boolean values.
	 * Almost all other fields are checked, though not all as strictly as possible (of
	 * course there's often no well-defined limit). For each problem case we find,
	 * give an error message and set the field to a reasonable default value.
	 */
	LogPrintf(LOG_NOTICE, "Checking CNFG: ");
	nerr = 0; firstErr = 0;

	if (config.maxDocuments <= 0 || config.maxDocuments > 100)
		{ config.maxDocuments = 10; ERR(1); }

	if (config.stemLenNormal < 1) { config.stemLenNormal = 14; ERR(2); }
	if (config.stemLen2v < 1) { config.stemLen2v = 12; ERR(3); }
	if (config.stemLenOutside < 1) { config.stemLenOutside = 10; ERR(4); }
	if (config.stemLenGrace < 1) { config.stemLenGrace = 10; ERR(5); }

	if (config.spAfterBar < 0) { config.spAfterBar = 6; ERR(6); }
	if (config.minSpBeforeBar < 0) { config.minSpBeforeBar = 5; ERR(7); }
	if (config.minRSpace < 0) { config.minRSpace = 2;  ERR(8); }
	if (config.minLyricRSpace < 0) { config.minLyricRSpace = 4;  ERR(9); }
	if (config.minRSpace_KSTS_N < 0) { config.minRSpace_KSTS_N = 8; ERR(10); }

	if (config.hAccStep < 0 || config.hAccStep > 31) { config.hAccStep = 4; ERR(11); }

	if (config.stemLW < 0) { config.stemLW = 8;  ERR(12); }
	if (config.barlineLW < 0) { config.barlineLW = 10;  ERR(13); }
	if (config.ledgerLW < 0) { config.ledgerLW = 13;  ERR(14); }
	if (config.staffLW < 0) { config.staffLW = 8;  ERR(15); }
	if (config.enclLW < 0) { config.enclLW = 4; ERR(16); }

	if (config.beamLW < 1) { config.beamLW = 50; ERR(17); }
	if (config.hairpinLW < 1) { config.hairpinLW = 12; ERR(18); }
	if (config.dottedBarlineLW < 1) { config.dottedBarlineLW = 12; ERR(19); }
	if (config.mbRestEndLW < 1) { config.mbRestEndLW = 12; ERR(20); }
	if (config.nonarpLW < 1) { config.nonarpLW = 12; ERR(21); }

	if (config.graceSlashLW < 0) { config.graceSlashLW = 12; ERR(22); }
	if (config.slurMidLW < 0) { config.slurMidLW = 30; ERR(23); }
	if (config.tremSlashLW < 0) { config.tremSlashLW = 38; ERR(24); }
	if (config.slurCurvature < 1) { config.slurCurvature = 100; ERR(25); }
	if (config.tieCurvature < 1) { config.tieCurvature = 100; ERR(26); }
	if (config.relBeamSlope < 0) { config.relBeamSlope = 25; ERR(27); }

	if (config.hairpinMouthWidth < 0 || config.hairpinMouthWidth > 31)
			{ config.hairpinMouthWidth = 5; ERR(28); }
	if (config.mbRestBaseLen < 1) { config.mbRestBaseLen = 6;  ERR(29); }
	if (config.mbRestAddLen < 0) { config.mbRestAddLen = 1;  ERR(30); }
	if (config.barlineDashLen < 1) { config.barlineDashLen = 4; ERR(31); }
	if (config.crossStaffBeamExt < 0) { config.crossStaffBeamExt = 5;  ERR(32); }

	if (config.titleMargin < 0) { config.titleMargin = in2pt(1);  ERR(33); };

	if (EmptyRect(&config.paperRect))
		{ SetRect(&config.paperRect, 0, 0, in2pt(17)/2, in2pt(11)); ERR(34); }

	if (config.pageMarg.top<1 || config.pageMarg.left<1 || config.pageMarg.bottom<1
		|| config.pageMarg.right<1) {
		if (config.pageMarg.top<1) { config.pageMarg.top = in2pt(1)/2; }
		if (config.pageMarg.left<1) { config.pageMarg.left = in2pt(1)/2; }
		if (config.pageMarg.bottom<1) { config.pageMarg.bottom = in2pt(1)/2; }
		if (config.pageMarg.right<1) { config.pageMarg.right = in2pt(1)/2; }
		ERR(35);
	}
	/* Crudely try to insure that even for smallest paper size, margins don't cross */
	if (config.pageMarg.top+config.pageMarg.bottom>in2pt(6)) {
		config.pageMarg.top = in2pt(1)/2;
		config.pageMarg.bottom = in2pt(1)/2;
		ERR(36);
	}
	if (config.pageMarg.left+config.pageMarg.right>in2pt(5)) {
		config.pageMarg.left = in2pt(1)/2;
		config.pageMarg.right = in2pt(1)/2;
		ERR(36);
	}

	if (config.pageNumMarg.top<1 || config.pageNumMarg.left<1 || config.pageNumMarg.bottom<1
		|| config.pageNumMarg.right<1) {
		if (config.pageNumMarg.top<1) { config.pageNumMarg.top = in2pt(1)/2; }
		if (config.pageNumMarg.left<1) { config.pageNumMarg.left = in2pt(1)/2; }
		if (config.pageNumMarg.bottom<1) { config.pageNumMarg.bottom = in2pt(1)/2; }
		if (config.pageNumMarg.right<1) { config.pageNumMarg.right = in2pt(1)/2; }
		ERR(37);
	}
	
	if (config.defaultLedgers < 1 || config.defaultLedgers > MAX_LEDGERS)
			{ config.defaultLedgers = 6; ERR(38); }
	if (TSNUM_BAD(config.defaultTSNum)) 	{ config.defaultTSNum = 4; ERR(39); }
	if (TSDENOM_BAD(config.defaultTSDenom)) { config.defaultTSDenom = 4; ERR(40); }
	if (config.defaultRastral < 0 || config.defaultRastral > MAXRASTRAL)
			{ config.defaultRastral = 5; ERR(41); }
	if (config.rastral0size < 4 || config.rastral0size > 72)
			{ config.rastral0size = pdrSize[0]; ERR(42); }

	if (config.minRecVelocity < 1 || config.minRecVelocity > MAX_VELOCITY)
			{ config.minRecVelocity = 10; ERR(43); }
	if (config.minRecDuration < 1) { config.minRecDuration = 50; ERR(44); }
#define MIDI_THRU
#ifdef MIDI_THRU
	/* FIXME: MIDI THRU SHOULD WORK FOR OMS, BUT MAY FAIL OR EVEN BE DANGEROUS W/OTHER
		DRIVERS! Needs thought/testing. */
	if (config.midiThru < 0) { config.midiThru = 0; ERR(45); }
#else
	config.midiThru = 0;
#endif		

	if (config.defaultTempoMM < MIN_BPM || config.defaultTempoMM > MAX_BPM)
			{ config.defaultTempoMM = 96; ERR(46); }
	if (config.lowMemory < config.minMemory) { config.lowMemory = config.minMemory; ERR(47); }
	if (config.minMemory < 1) { config.minMemory = 1; ERR(48); }

	if (config.numRows < 1) { config.numRows = 4; ERR(49); }
	if (config.numCols < 1) { config.numCols = 4; ERR(50); }
	if (config.maxRows < 1) { config.maxRows = 16; ERR(51); }
	if (config.maxCols < 1) { config.maxCols = 8; ERR(52); }
	if (config.hPageSep < 0) { config.hPageSep = 8; ERR(53); }
	if (config.vPageSep < 0) { config.vPageSep = 8; ERR(54); }
	if (config.hScrollSlop < 0) { config.hScrollSlop = 16; ERR(55); }
	if (config.vScrollSlop < 0) { config.vScrollSlop = 16; ERR(56); }

	if (config.maxSyncTolerance < 0) { config.maxSyncTolerance = 60;  ERR(57); }
	if (config.enlargeHilite < 0 || config.enlargeHilite > 10)
			{ config.enlargeHilite = 1; ERR(58); }
	if (config.infoDistUnits < 0 || config.infoDistUnits > 2)		/* Max. from enum in InfoDialog.c */
			{ config.infoDistUnits = 0; ERR(59); }
	if (config.mShakeThresh < 0) { config.mShakeThresh = 0; ERR(60); }

	if (config.numMasters < 64) { config.numMasters = 64; ERR(62); }
	
	if (config.indent1st < 0) { config.indent1st = 47; ERR(63); }
	if (config.mbRestHeight < 1) { config.mbRestHeight = 2; ERR(64); }
	if (config.chordSymMusSize < 10) { config.chordSymMusSize = 150; ERR(65); }
	if (config.enclMargin < 0) { config.enclMargin = 2; ERR(66); }

	if (config.legatoPct < 1) { config.legatoPct = 95; ERR(67); }
	if (config.defaultPatch < 1 || config.defaultPatch > MAXPATCHNUM)
			{ config.defaultPatch = 1; ERR(68); }
			
	if (config.whichMIDI < 0 || config.whichMIDI > MIDISYS_NONE) { config.whichMIDI = MIDISYS_CM; ERR(69); }
	
	/* Set min. for musFontSizeOffset so even if rastral 0 is 4 pts, will still give >0 PS size */
	if (config.musFontSizeOffset < -3) { config.musFontSizeOffset = 0; ERR(70); }

	if (config.restMVOffset < 0 || config.restMVOffset > 20)
			{ config.restMVOffset = 2; ERR(72); }
	if (config.autoBeamOptions < 0 || config.autoBeamOptions > 3)
			{ config.autoBeamOptions = 0; ERR(73); }
	if (config.noteOffVel < 1 || config.noteOffVel > MAX_VELOCITY)
			{ config.noteOffVel = 64; ERR(74); }
	if (config.feedbackNoteOnVel < 1 || config.feedbackNoteOnVel > MAX_VELOCITY)
			{ config.feedbackNoteOnVel = 64; ERR(75); }
	if (config.defaultChannel < 1 || config.defaultChannel > MAXCHANNEL)
			{ config.defaultChannel = 1; ERR(76); }
	if (config.rainyDayMemory < 32 || config.rainyDayMemory > 1024)
			{ config.rainyDayMemory = 32; ERR(78); }
	if (config.tryTupLevels < 1 || config.tryTupLevels > 321)
			{ config.tryTupLevels = 21; ERR(79); }
	if (config.justifyWarnThresh < 10) { config.justifyWarnThresh = 15; ERR(80); }

	if (config.metroChannel < 1 || config.metroChannel > MAXCHANNEL)
			{ config.metroChannel = 1; ERR(81); }
	if (config.metroNote < 1 || config.metroNote > MAX_NOTENUM)
			{ config.metroNote = 77; ERR(82); }
	if (config.metroVelo < 1 || config.metroVelo > MAX_VELOCITY)
			{ config.metroVelo = 90; ERR(83); }
	if (config.metroDur < 1 || config.metroDur > 999) { config.metroDur = 50; ERR(84); }

	if (config.chordSymSmallSize < 1 || config.chordSymSmallSize > 127)
			{ config.chordSymSmallSize = 1; ERR(85); }
	if (config.chordSymSuperscr < 0 || config.chordSymSuperscr > 127)
			{ config.chordSymSuperscr = 1; ERR(86); }
	if (config.chordSymStkLead < -20 || config.chordSymStkLead > 127)
			{ config.chordSymStkLead = 10; ERR(87); }

	if (config.tupletNumSize < 0 || config.tupletNumSize > 127)
			{ config.tupletNumSize = 110; ERR(92); }
	if (config.tupletColonSize < 0 || config.tupletColonSize > 127)
			{ config.tupletColonSize = 60; ERR(93); }
	if (config.octaveNumSize < 0 || config.octaveNumSize > 127)
			{ config.octaveNumSize = 110; ERR(94); }
	if (config.lineLW < 5 || config.lineLW > 127) { config.lineLW = 25; ERR(95); }
	if (config.ledgerLLen < 32) { config.ledgerLLen = 48; ERR(96); }
	if (config.ledgerLOtherLen < 0) { config.ledgerLOtherLen = 12; ERR(97); }
	if (config.slurDashLen < 1) { config.slurDashLen = 3; ERR(98); }
	if (config.slurSpaceLen < 1) { config.slurSpaceLen = 3; ERR(99); }

	if (config.courtesyAccLXD < 0) { config.courtesyAccLXD = 6; ERR(100); }
	if (config.courtesyAccRXD < 0) { config.courtesyAccRXD = 8; ERR(101); }
	if (config.courtesyAccYD < 0) { config.courtesyAccYD = 8; ERR(102); }
	if (config.courtesyAccSize < 20) { config.courtesyAccSize = 100; ERR(103); }

	if (config.quantizeBeamYPos<0) { config.quantizeBeamYPos = 3; ERR(104); };

	if (config.enlargeNRHiliteH < 0 || config.enlargeNRHiliteH > 10)
			{ config.enlargeNRHiliteH = 1; ERR(105); }
	if (config.enlargeNRHiliteV < 0 || config.enlargeNRHiliteV > 10)
			{ config.enlargeNRHiliteV = 1; ERR(106); }

	/* No validity check at this time for default or metro Devices, do it in InitOMS */

	if (gotCnfg && nerr>0) {
        LogPrintf(LOG_NOTICE, " TOTAL OF %d ERROR(S) FOUND.\n", nerr);
		GetIndCString(fmtStr, INITERRS_STRS, 5);		/* "CNFG resource in Prefs has [n] illegal value(s)" */
		sprintf(strBuf, fmtStr, nerr, firstErr);
		CParamText(strBuf, "", "", "");
		if (CautionAdvise(CNFG_ALRT)==OK) ExitToShell();
	}
    else
        LogPrintf(LOG_NOTICE, "(no errors)\n");
	
Finish:
	/* Make any final adjustments. N.B. Must undo these in UpdateSetupFile()! */
	
	config.maxDocuments++;								/* to allow for clipboard doc */

	return TRUE;
}


/* Make the heap _almost_ as large as possible (so we have a little extra stack space),
and allocate as many Master Pointers as possible up to numMasters. Return TRUE if
all went well, FALSE if not. */

static Boolean InitMemory(short numMasters)
{
#ifdef CARBON_NOMORE
	THz thisZone; long heapSize; short orig;
	
	/* Increase stack size by decreasing heap size (thanks to DBW for the method). */
	heapSize = (long)GetApplLimit()-(long)GetZone();
	heapSize -= 32000;
	SetApplLimit((Ptr)ApplicationZone()+heapSize);

	MaxApplZone();
	thisZone = ApplicationZone();
	orig = thisZone->moreMast;
	thisZone->moreMast = numMasters;
	do {
		MoreMasters();
	} while (MemError()!=noErr && (thisZone->moreMast=(numMasters >>= 1))!=0);
	thisZone->moreMast = orig;
#endif
	
	return(numMasters >= 64);
}


/* Initialize everything about the floating windows. NB: as of v.2.01, for a long time,
we've had a tool palette. We've kept vestigal code for the help palette (unlikely
ever to be used) and for the clavier palette (fairly likely to be used someday). */

static Boolean NInitFloatingWindows()
	{
		short index,wdefID;
		PaletteGlobals *whichPalette;
		Rect windowRects[TOTAL_PALETTES];
		int totalPalettes = TOTAL_PALETTES;
		
		for (index=0; index<totalPalettes; index++) {
		
			/* Get a handle to a PaletteGlobals structure for this palette */
			
			paletteGlobals[index] = (PaletteGlobals **)GetResource('PGLB',ToolPaletteWDEF_ID+index);
			if (!GoodResource((Handle)paletteGlobals[index])) return FALSE;

			MoveHHi((Handle)paletteGlobals[index]);
			HLock((Handle)paletteGlobals[index]);
			whichPalette = *paletteGlobals[index];
			
			switch(index) {
				case TOOL_PALETTE:
					SetupToolPalette(whichPalette, &windowRects[index]);
					wdefID = ToolPaletteWDEF_ID;
					break;
				case HELP_PALETTE:
				case CLAVIER_PALETTE:
					break;
				}

			wdefID = floatGrowProc;
			
			if (thisMac.hasColorQD)
				palettes[index] = (WindowPtr)NewCWindow(NULL,
										&windowRects[index],
										"\p",FALSE,wdefID,
										BRING_TO_FRONT,TRUE,
										(long)index);
			 else
				palettes[index] = (WindowPtr)NewWindow(NULL,
										&windowRects[index],
										"\p",FALSE,wdefID,
										BRING_TO_FRONT,TRUE,
										(long)index);
			if (!GoodNewPtr((Ptr)palettes[index])) return FALSE;
		
			//	((WindowPeek)palettes[index])->spareFlag = (index==TOOL_PALETTE || index==HELP_PALETTE);
			
			/* Add a zoom box to tools and help palette */
			if (index==TOOL_PALETTE || index==HELP_PALETTE)		
			{
				ChangeWindowAttributes(palettes[index], kWindowFullZoomAttribute, kWindowNoAttributes);
			}

			/* Finish Initializing the TearOffMenuGlobals structure. */
			
			whichPalette->environment = &thisMac;
			whichPalette->paletteWindow = palettes[index];
			SetWindowKind(palettes[index], PALETTEKIND);
			
			HUnlock((Handle)paletteGlobals[index]);
			}
	
	return TRUE;
	}


/* Initialise the Symbol Palette: We use the size of the PICT in resources as well as
the grid dimensions in the PLCH resource to determine the size of the palette, with a
margin around the cells so that there is space in the lower right corner for a grow box.
The PICT should have a frame size that is (TOOLS_ACROSS * TOOLS_CELL_WIDTH)-1 pixels
wide and (TOOLS_DOWN * TOOLS_CELL_HEIGHT) - 1 pixels high, where TOOLS_ACROSS and
TOOLS_DOWN are the values taken from the first two bytes of the PLCH resource. This
picture will be displayed centered in the palette window with a TOOLS_MARGIN pixel
margin on all sides. */

static void SetupToolPalette(PaletteGlobals *whichPalette, Rect *windowRect)
	{
		PicHandle toolPicture; Rect picRect;
		short curResFile; short defaultToolItem;
		
		/* Allocate a grid of characters from the 'PLCH' resource. */
		defaultToolItem = GetToolGrid(whichPalette);
		if (!defaultToolItem) { BadInit(); ExitToShell(); }
		
		curResFile = CurResFile();
		UseResFile(setupFileRefNum);
		toolPicture = (PicHandle)GetPicture(ToolPaletteID);
		fix_end((*toolPicture)->picFrame.bottom);
		fix_end((*toolPicture)->picFrame.right);
		UseResFile(curResFile);

		if(!GoodResource((Handle)toolPicture)) { BadInit(); ExitToShell(); }
		HNoPurge((Handle)toolPicture);
		
		/* Calculate the palette's portRect based on the toolPicture. */
		
		picRect = (*toolPicture)->picFrame;
		OffsetRect(&picRect, -picRect.left, -picRect.top);
		*windowRect = picRect;
		
		SetupPaletteRects(toolRects,
				whichPalette->maxAcross, whichPalette->maxDown,
				toolCellWidth = (windowRect->right + 1) / whichPalette->maxAcross,
				toolCellHeight = (windowRect->bottom + 1) / whichPalette->maxDown);
		
		windowRect->right -= (whichPalette->maxAcross-whichPalette->firstAcross) * toolCellWidth;
		windowRect->bottom -= (whichPalette->maxDown-whichPalette->firstDown) * toolCellHeight;

		windowRect->right += 2*TOOLS_MARGIN;
		windowRect->bottom += 2*TOOLS_MARGIN;
		toolsFrame = *windowRect;
		InsetRect(&toolsFrame,TOOLS_MARGIN,TOOLS_MARGIN);
		
		/* Initialize the PaletteGlobals structure */
		
	 	whichPalette->currentItem = defaultToolItem;
		whichPalette->drawMenuProc = (void (*)())DrawToolMenu;
		whichPalette->findItemProc = (short (*)())FindToolItem;
		whichPalette->hiliteItemProc = (void (*)())HiliteToolItem;
		
		/* Put picture into offscreen port so that any rearrangements can be saved */

#if 0	
		GrafPtr oldPort;
		palPort = NewGrafPort(picRect.right, picRect.bottom);
		if (palPort == NULL) { BadInit(); ExitToShell(); }
		GetPort(&oldPort); SetPort(palPort);
		
		HLock((Handle)toolPicture);
		DrawPicture(toolPicture, &picRect);
		HUnlock((Handle)toolPicture);
		ReleaseResource((Handle)toolPicture);
		SetPort(oldPort);
#else
		SaveGWorld();
		
		GWorldPtr gwPtr = MakeGWorld(picRect.right, picRect.bottom, TRUE);
		SetGWorld(gwPtr, NULL);
		
		HLock((Handle)toolPicture);
		DrawPicture(toolPicture, &picRect);
		HUnlock((Handle)toolPicture);
		ReleaseResource((Handle)toolPicture);
		
		palPort = gwPtr;
//		UnlockGWorld(gwPtr);
		RestoreGWorld();
#endif
	}


/* Allocate a grid of characters from the 'PLCH' resource; uses GridRec dataType to
store the information for a maximal maxRow by maxCol grid, where the dimensions
are stored in the PLCH resource itself and should match the PICT that is being
used to draw the palette.  Deliver the item number of the default tool (arrow),
or 0 if problem. */

static short GetToolGrid(PaletteGlobals *whichPalette)
	{
		short maxRow, maxCol, row, col, item, defItem = 0; short curResFile;
		GridRec *pGrid; Handle hdl;
		unsigned char *p;			/* Careful: contains both binary and char data! */
		
		curResFile = CurResFile();
		UseResFile(setupFileRefNum);

		hdl = GetResource('PLCH', ToolPaletteID);			/* get char map of palette */
		if (!GoodResource(hdl)) {
			GetIndCString(strBuf, INITERRS_STRS, 6);		/* "can't get Tool Palette resource" */
			CParamText(strBuf, "", "", "");
			StopInform(GENERIC_ALRT);
			UseResFile(curResFile);
			return(0);
			}
		
		UseResFile(curResFile);

		/* Pull in the maximum and suggested sizes for palette */
		
		p = (unsigned char *) *hdl;
		maxCol = whichPalette->maxAcross = *p++;
		maxRow = whichPalette->maxDown = *p++;
		whichPalette->firstAcross = whichPalette->oldAcross = whichPalette->across = *p++;
		whichPalette->firstDown   = whichPalette->oldDown   = whichPalette->down   = *p++;
		
		/* Create storage for 2-D table of char/positions for palette grid */
		
		grid = (GridRec *) NewPtr((Size)maxRow*maxCol*sizeof(GridRec));
		if (!GoodNewPtr((Ptr)grid)) {
			OutOfMemory((long)maxRow*maxCol*sizeof(GridRec));
			return(0);
			}

		/* And scan through resource to install grid cells */
		
		p = (unsigned char *) (*hdl + 4*sizeof(long));		/* Skip header stuff */
		pGrid = grid;
		item = 1;
		for (row=0; row<maxRow; row++)						/* initialize grid[] */
			for (col=0; col<maxCol; col++,pGrid++,item++) {
				SetPt(&pGrid->cell, col, row);				/* Doesn't move memory */
				if ((pGrid->ch = *p++) == CH_ENTER)
					if (defItem == 0) defItem = item; 
				}
		
		GetToolZoomBounds();
		return(defItem);
	}


/* Install the rectangles for the given pallete rects array and its dimensions */

static void SetupPaletteRects(Rect *whichRects, short itemsAcross, short itemsDown,
								short itemWidth, short itemHeight)
	{
		short across,down;
		
		whichRects->left = 0;
		whichRects->top = 0;
		whichRects->right = 0;
		whichRects->bottom = 0;

		whichRects++;
		for (down=0; down<itemsDown; down++)
			for (across=0; across<itemsAcross; across++,whichRects++) {
				whichRects->left = across * itemWidth;
				whichRects->top = down * itemHeight;
				whichRects->right = (across + 1) * itemWidth;
				whichRects->bottom = (down + 1) * itemHeight;
				OffsetRect(whichRects,TOOLS_MARGIN,TOOLS_MARGIN);
				}
	}


/* Provide the pre-allocated clipboard document with default values. */
	
static Boolean PrepareClipDoc()
	{
		WindowPtr w; char title[256]; LINK pL; long junkVersion;

		clipboard = documentTable;
		clipboard->inUse = TRUE;
		w = GetNewWindow(docWindowID,NULL,(WindowPtr)-1);
		if (w == NULL) return(FALSE);
		
		clipboard->theWindow = w;
		SetWindowKind(w, DOCUMENTKIND);

		//		((WindowPeek)w)->spareFlag = TRUE;
		ChangeWindowAttributes(w, kWindowFullZoomAttribute, kWindowNoAttributes);

		if (!BuildDocument(clipboard,NULL,0,NULL,&junkVersion,TRUE))
			return FALSE;
		for (pL=clipboard->headL; pL!=clipboard->tailL; pL=DRightLINK(clipboard, pL))
			if (DObjLType(clipboard, pL)==MEASUREtype)
				{ clipFirstMeas = pL; break; }
		clipboard->canCutCopy = FALSE;
		GetIndCString(title,MiscStringsID,2);
		SetWCTitle((WindowPtr)clipboard,title);
		return TRUE;
	}
	
Boolean BuildEmptyDoc(Document *doc) 
	{
		if (!InitAllHeaps(doc)) { NoMoreMemory(); return FALSE; }
		InstallDoc(doc);
		BuildEmptyList(doc,&doc->headL,&doc->tailL);
		
		doc->selStartL = doc->selEndL = doc->tailL;			/* Empty selection  */	
		return TRUE;
	}

	
//#define TEST_MDEF_CODE
#ifdef TEST_MDEF_CODE
// add some interesting sample items
static void AddSampleItems(MenuRef menu)
{
	MenuItemIndex	item;
	
	AppendMenuItemTextWithCFString(menu, CFSTR("Checkmark"), 0, 0, &item);
	CheckMenuItem(menu, item, true);
	AppendMenuItemTextWithCFString(menu, CFSTR("Dash"), 0, 0, &item);
	SetItemMark(menu, item, '-');
	AppendMenuItemTextWithCFString(menu, CFSTR("Diamond"), 0, 0, &item);
	SetItemMark(menu, item, kDiamondCharCode);
	AppendMenuItemTextWithCFString(menu, CFSTR("Bullet"), 0, 0, &item);
	SetItemMark(menu, item, kBulletCharCode);
	AppendMenuItemTextWithCFString(menu, NULL, kMenuItemAttrSeparator, 0, NULL);
	AppendMenuItemTextWithCFString(menu, CFSTR("Section Header"), kMenuItemAttrSectionHeader, 0, NULL);
	AppendMenuItemTextWithCFString(menu, CFSTR("Indented item 1"), 0, 0, &item);
	SetMenuItemIndent(menu, item, 1);
	AppendMenuItemTextWithCFString(menu, CFSTR("Indented item 2"), 0, 0, &item);
	SetMenuItemIndent(menu, item, 1);
}
#endif

/* Initialize application-specific globals, etc. */

Boolean InitGlobals()
	{
		long size; Document *doc;
		
		InitNightingale();
				
		Pstrcpy((unsigned char *)strBuf, VersionString());
		PToCString((unsigned char *)strBuf);
		GoodStrncpy(applVerStr, strBuf, 15); 	/* Allow one char. for terminator */

		/* Create an empty Document table for max number of docs in configuration */
		
		size = (long)sizeof(Document) * config.maxDocuments;
		documentTable = (Document *)NewPtr(size);
		if (!GoodNewPtr((Ptr)documentTable))
			{ OutOfMemory(size); return FALSE; }
		
		topTable = documentTable + config.maxDocuments;
		for (doc=documentTable; doc<topTable; doc++) doc->inUse = FALSE;
		
		/* Preallocate the clipboard and search documents */
		if (!PrepareClipDoc()) return FALSE;
			
		/* Set up the global scrap reference */
		GetCurrentScrap(&gNightScrap);
	
		/* Allocate various non-relocatable things first */
		
		tmpStr = (unsigned char *)NewPtr(256);
		if (!GoodNewPtr((Ptr)tmpStr))
			{ OutOfMemory(256L); return FALSE; }
		
		updateRectTable = (Rect *)NewPtr(MAX_UPDATE_RECTS * sizeof(Rect));
		if (!GoodNewPtr((Ptr)updateRectTable))
			{ OutOfMemory((long)MAX_UPDATE_RECTS * sizeof(Rect)); return FALSE; }
		
		/* Pull in the permanent menus from resources and install in menu bar. */
		
		appleMenu = GetMenu(appleID);		if (!appleMenu) return FALSE;
		AppendResMenu(appleMenu,'DRVR');
		InsertMenu(appleMenu,0);
		
		fileMenu = GetMenu(fileID);			if (!fileMenu) return FALSE;
		InsertMenu(fileMenu,0);
		
		editMenu = GetMenu(editID);			if (!editMenu) return FALSE;
		InsertMenu(editMenu,0);
		
		if (FALSE || CmdKeyDown() && OptionKeyDown())
		{
			AppendMenu(editMenu, "\p(-");
			AppendMenu(editMenu, "\pBrowser");
			AppendMenu(editMenu, "\pDebug...");
			AppendMenu(editMenu, "\pDelete Objects");
		}

#ifndef PUBLIC_VERSION
		testMenu = GetMenu(testID);		if (!testMenu) return FALSE;
		InsertMenu(testMenu,0);
#endif
		
		scoreMenu = GetMenu(scoreID);		if (!scoreMenu) return FALSE;
		InsertMenu(scoreMenu,0);
		
		notesMenu = GetMenu(notesID);		if (!notesMenu) return FALSE;
		InsertMenu(notesMenu,0);
		
		groupsMenu = GetMenu(groupsID);		if (!groupsMenu) return FALSE;
		InsertMenu(groupsMenu,0);
		
		viewMenu = GetMenu(viewID);			if (!viewMenu) return FALSE;
		InsertMenu(viewMenu,0);
		
		magnifyMenu = GetMenu(magnifyID);	if (!magnifyMenu) return FALSE;
		InsertMenu(magnifyMenu,hierMenu);
		
		/* We install only one of the following menus at a time. */
		
		playRecMenu = GetMenu(playRecID);	if (!playRecMenu) return FALSE;
		InsertMenu(playRecMenu,0);
		
#ifdef TEST_MDEF_CODE
		MenuRef			customMenu;
		MenuDefSpec		defSpec;
		
		defSpec.defType = kMenuDefProcPtr;
		defSpec.u.defProc = NewMenuDefUPP( MyMDefProc );

		// Create a custom menu
		//CreateCustomMenu( &defSpec, 1201, 0, &customMenu );
		//SetMenuTitleWithCFString( customMenu, CFSTR("Sample [Custom]") );
		//InsertMenu( customMenu, 0 );
		//AddSampleItems( customMenu );
		CreateCustomMenu( &defSpec, TWODOTDUR_MENU, 0, &customMenu );
		SetMenuTitleWithCFString( customMenu, CFSTR("Graphic MDEF") );
		InsertMenu( customMenu, 0 );
		AddSampleItems( customMenu );
#endif
		
		masterPgMenu = GetMenu(masterPgID);	if (!masterPgMenu) return FALSE;
		
		formatMenu = GetMenu(formatID);		if (!formatMenu) return FALSE;
		
		DrawMenuBar();
		
		/* Pull in other miscellaneous things from resources. */
		
		GetIndPattern(&paperBack,MiscPatternsID,1);
		GetIndPattern(&diagonalDkGray,MiscPatternsID,2);
		GetIndPattern(&diagonalLtGray,MiscPatternsID,3);
		
		/* Call stub routines to load all segments needed for selection, so user
		doesn't have to wait for them to load when he/she makes the the first
		selection. */

		XLoadEditScoreSeg();
		
		return TRUE;
	}


/* Install the addresses of the functions that the Event Manager will call when
any of the Apple events we handle (for now, only the core events) come in over the
transom. */

static AEEventHandlerUPP oappUPP, odocUPP, pdocUPP, quitUPP;
#ifdef FUTUREEVENTS
static AEEventHandlerUPP editUPP, closUPP;
#endif

static void InstallCoreEventHandlers()
{
	OSErr err;
	
	/* Leave these UPP's permanently allocated. */
	oappUPP = NewAEEventHandlerUPP(HandleOAPP);
	odocUPP = NewAEEventHandlerUPP(HandleODOC);
	pdocUPP = NewAEEventHandlerUPP(HandlePDOC);
	quitUPP = NewAEEventHandlerUPP(HandleQUIT);

	if (quitUPP) {
		err = AEInstallEventHandler(kCoreEventClass,kAEOpenApplication,oappUPP,0,FALSE);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEOpenDocuments,odocUPP,0,FALSE);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEPrintDocuments,pdocUPP,0,FALSE);
		if (err) goto broken;
		err = AEInstallEventHandler(kCoreEventClass,kAEQuitApplication,quitUPP,0,FALSE);
		if (err) goto broken;
	}
	
#ifdef FUTUREEVENTS
	/*	See the note where <myAppType> is set on distinguishing run-time env. */

	editUPP = NewAEEventHandlerUPP(HandleEdit);
	closUPP = NewAEEventHandlerUPP(HandleClose);

	if (closUPP) {
		err = AEInstallEventHandler(myAppType,'Edit',editUPP,0,FALSE);
		if (err) goto broken;
		err = AEInstallEventHandler(myAppType,'Clos',closeUPP,0,FALSE);
	}
#endif

broken:
	if (err) {
	}
}
