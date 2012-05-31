/* MasterPage.h: prototypes for MasterPage.c for Nightingale */

void FixLedgerYSp(Document *);

void ExportMasterPage(Document *);

void DoMasterStfLen(Document *);
void DoMasterStfSize(Document *);
void DoMasterStfLines(Document *);
void MPEditMargins(Document *);

void SetupMasterMenu(Document *, Boolean);
Boolean SetupMasterView(Document *);
Boolean ExitMasterView(Document *);
void EnterMasterView(Document *);
void ImportMasterPage(Document *);

void MPUpdateStaffTops(Document *, LINK, LINK);
void UpdateMPSysRectHeight(Document *, DDIST);
void UpdateDraggedSystem(Document *, long);
void UpdateDraggedStaff(Document *, LINK, LINK, long);
void UpdateMasterMargins(Document *);

void Score2MasterPage(Document *doc);
void ReplaceMasterPage(Document *doc);
void VisifyMasterStaves(Document *doc);
Boolean NewMasterPage(Document *, DDIST sysTop, Boolean fromDoc);
Boolean CopyMasterRange(Document *, LINK, LINK, LINK);

void MPDeletePart(Document *doc);
INT16 CountConnParts(Document *,LINK,LINK,LINK,LINK);
void UpdateConnFields(LINK connectL,INT16 stfDiff,INT16 lastStf);

void StoreAllConnectParts(LINK headL);
void StoreConnectPart(LINK headL,LINK aConnectL);
void MPAddPart(Document *doc);

INT16 GetPartSelRange(Document *doc, LINK *firstPart, LINK *lastPart);
INT16 PartRangeSel(Document *doc);
INT16 GroupSel(Document *doc);
INT16 PartSel(Document *doc);

void MPSetInstr(Document *doc, PPARTINFO pPart);
void GetSelStaves(LINK staffL,INT16 *minStf,INT16 *maxStf);
void DoGroupParts(Document *doc);
void DoUngroupParts(Document *doc);
Boolean DoMake1StaffParts(Document *doc);

void MPDistributeStaves(Document *);