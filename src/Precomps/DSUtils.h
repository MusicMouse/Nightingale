/* DSUtils.h for Nightingale */

void InvalMeasure(LINK, INT16);
void InvalMeasures(LINK, LINK, INT16);
void InvalSystem(LINK);
void InvalSystems(LINK, LINK);
void InvalSysRange(LINK, LINK);
void InvalSelRange(Document *);
void InvalRange(LINK, LINK);
void EraseAndInvalRange(Document *, LINK, LINK);
void InvalContent(LINK, LINK);
void InvalObject(Document *doc, LINK pL, INT16 doErase);

void FixMeasNums(LINK, INT16);
Boolean FakeMeasure(Document *, LINK);
void UpdatePageNums(Document *);
void UpdateSysNums(Document *, LINK);
Boolean UpdateMeasNums(Document *, LINK);
INT16 GetMeasNum(Document *, LINK);

Boolean PtInMeasure(Document *doc,Point pt,LINK sL);

DDIST PageRelxd(LINK pL, PCONTEXT pContext);
DDIST PageRelyd(LINK pL, PCONTEXT pContext);
DDIST GraphicPageRelxd(Document *, LINK, LINK, PCONTEXT);
Point LinkToPt(Document *doc, LINK pL, Boolean toWindow);
DDIST ObjSpaceUsed(Document *doc, LINK pL);
DDIST SysRelxd(LINK);
DDIST Sysxd(Point, DDIST);
DDIST PMDist(LINK, LINK);
Boolean HasValidxd(LINK);
LINK FirstValidxd(LINK, Boolean);
LINK DFirstValidxd(Document *, Document *, LINK, Boolean);
LINK ObjWithValidxd(LINK, Boolean);
DDIST GetSubXD(LINK, LINK);

void ZeroXD(LINK, LINK);
Boolean RealignObj(LINK, Boolean);

DDIST GetSysWidth(Document *);
DDIST GetSysLeft(Document *);
DDIST StaffHeight(Document *doc, LINK, INT16);
DDIST StaffLength(LINK);
LINK GetLastMeasInSys(LINK sysL);
void MeasRange(Document *doc,LINK pL,LINK *startMeas,LINK *endMeas);
DDIST MeasWidth(LINK);
DDIST MeasOccupiedWidth(Document *, LINK, long);
DDIST MeasJustWidth(Document *, LINK, CONTEXT);
Boolean SetMeasWidth(LINK, DDIST);
Boolean MeasFillSystem(LINK);

Boolean IsAfter(LINK, LINK);
Boolean IsAfterIncl(LINK, LINK);
Boolean BetweenIncl(LINK, LINK, LINK);
Boolean WithinRange(LINK, LINK, LINK);
Boolean IsOutside(LINK, LINK, LINK);
Boolean SameSystem(LINK, LINK);
Boolean SameMeasure(LINK, LINK);
Boolean ConsecSync(LINK, LINK, INT16, INT16);
INT16 LinkBefFirstMeas(LINK);
INT16 BeforeFirstPageMeas(LINK);
Boolean BeforeFirstMeas(LINK);

LINK GetCurrentPage(Document *doc);
LINK GetMasterPage(Document *doc);
void GetSysRange(Document *, LINK, LINK, LINK *, LINK *);
LINK FirstSysMeas(LINK pL);
LINK FirstDocMeas(Document *doc);
Boolean FirstMeasInSys(LINK measL);
Boolean LastMeasInSys(LINK);
Boolean LastUsedMeasInSys(Document *doc, LINK measL);
LINK LastOnPrevSys(LINK);
LINK LastObjInSys(Document *, LINK);
LINK GetNextSystem(Document *, LINK);
Boolean	SamePage(LINK, LINK);
Boolean LastSysInPage(LINK);
LINK GetLastSysInPage(LINK);
Boolean FirstSysInPage(LINK);
Boolean FirstSysInScore(LINK sysL);
INT16 NSysOnPage(LINK);
LINK LastObjOnPage(Document *, LINK);
Boolean RoomForSystem(Document *, LINK);
Boolean MasterRoomForSystem(Document *, LINK);
LINK NextSystem(Document *, LINK);
LINK GetNext(LINK, Boolean);
LINK RSysOnPage(Document *, LINK);

LINK MoveInMeasure(LINK, LINK, DDIST);
void MoveMeasures(LINK, LINK, DDIST);
void MoveAllMeasures(LINK, LINK, DDIST);
LINK DMoveInMeasure(Document *, Document *, LINK, LINK, DDIST);
void DMoveMeasures(Document *, Document *, LINK, LINK, DDIST);
Boolean CheckMoveMeasures(Document *doc, LINK, DDIST);
Boolean MoveRestOfSystem(Document *, LINK, DDIST);

long SyncAbsTime(LINK);
LINK MoveTimeInMeasure(LINK, LINK, long);
void MoveTimeMeasures(LINK, LINK, long);

Boolean ContentObj(LINK);

unsigned INT16 CountNotesInRange(INT16 staff, LINK startL, LINK endL, Boolean selectedOnly);
unsigned INT16 CountGRNotesInRange(INT16 staff, LINK startL, LINK endL, Boolean selectedOnly);
unsigned INT16 CountNotes(INT16, LINK, LINK, Boolean);
unsigned INT16 VCountNotes(INT16, LINK, LINK, Boolean);
unsigned INT16 CountGRNotes(INT16, LINK, LINK, Boolean);
unsigned INT16 SVCountNotes(INT16, INT16, LINK, LINK, Boolean);
unsigned INT16 SVCountGRNotes(INT16, INT16, LINK, LINK, Boolean);
unsigned INT16 CountObjects(LINK, LINK, INT16);

void CountInHeaps(Document *, unsigned INT16 [], Boolean);

Boolean HasOtherStemSide(LINK, INT16);
Boolean NoteLeftOfStem(LINK, LINK, Boolean);

INT16 GetStemUpDown(LINK, INT16);
INT16 GetGRStemUpDown(LINK, INT16);
void GetExtremeNotes(LINK syncL, INT16 voice, LINK *pLowNoteL, LINK *pHiNoteL);
void GetExtremeGRNotes(LINK syncL, INT16 voice, LINK *pLowNoteL, LINK *pHiNoteL);
LINK FindMainNote(LINK, INT16);
LINK FindGRMainNote(LINK, INT16);

void GetObjectLimits(INT16, INT16 *, INT16 *, Boolean *);
Boolean InDataStruct(Document *doc, LINK, INT16);
INT16 GetSubObjStaff(LINK, INT16);
INT16 GetSubObjVoice(LINK, INT16);
Boolean ObjOnStaff(LINK, INT16, Boolean);
INT16 CommonStaff(Document *, LINK, LINK);
Boolean ObjHasVoice(LINK pL);
LINK ObjSelInVoice(LINK pL, INT16 v);
LINK StaffOnStaff(LINK staffL, INT16 s);
LINK ClefOnStaff(LINK pL, INT16 s);
LINK KeySigOnStaff(LINK pL, INT16 s);
LINK TimeSigOnStaff(LINK pL, INT16 s);
LINK TempoOnStaff(LINK pL, INT16 s);
LINK MeasOnStaff(LINK pL, INT16 s);
LINK NoteOnStaff(LINK pL, INT16 s);
LINK GRNoteOnStaff(LINK pL, INT16 s);
LINK NoteInVoice(LINK pL, INT16 v, Boolean needSel);
LINK GRNoteInVoice(LINK pL, INT16 v, Boolean needSel);
Boolean SyncInVoice(LINK pL, INT16 voice);
Boolean GRSyncInVoice(LINK pL, INT16 voice);
INT16 SyncVoiceOnStaff(LINK, INT16);
Boolean SyncInBEAMSET(LINK, LINK);
Boolean SyncInOCTAVA(LINK, LINK);

Boolean PrevTiedNote(LINK, LINK, LINK *, LINK *);
Boolean FirstTiedNote(LINK, LINK, LINK *, LINK *);

LINK ChordNextNR(LINK syncL, LINK theNoteL);

Boolean GetCrossStaff(INT16, LINK[], STFRANGE *);

Boolean InitialClefExists(LINK clefL);
Boolean BFKeySigExists(LINK keySigL); 
Boolean BFTimeSigExists(LINK timeSigL);

char *StaffPartName(Document *doc, INT16 staff);
void SetTempFlags(Document *, Document *, LINK, LINK, Boolean);
void SetSpareFlags(LINK, LINK, Boolean);
Boolean GetMultiVoice(LINK pL, INT16 staff);
INT16 GetSelectionStaff(Document *doc);
void TweakSubRects(Rect *r, LINK aNoteL, CONTEXT *pContext);
INT16 CompareScoreFormat(Document *doc1,Document *doc2,INT16 pasteType);
LINK GetaMeasL(LINK measL,INT16 stf);
void DisposeMODNRs(LINK, LINK);

LINK Staff2PartL(Document *doc,LINK headL,INT16 stf);
INT16 PartL2Partn(Document *doc, LINK partL);
LINK Partn2PartL(Document *doc, INT16 partn);

LINK VHasTieAcross(LINK node,INT16 voice);
Boolean HasSmthgAcross(Document *, LINK, char *);

INT16 LineSpace2Rastral(DDIST);
DDIST Rastral2LineSpace(INT16);
INT16 StaffRastral(LINK);