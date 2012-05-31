/* InsNew.h for Nightingale */

void UpdateBFClefStaff(LINK firstClefL,INT16 staffn,INT16 subtype);
void UpdateBFKSStaff(LINK firstKSL,INT16 staffn,KSINFO newKSInfo);
void UpdateBFTSStaff(LINK firstTSL,INT16 staffn,INT16 subType,INT16 numerator,INT16 denominator);

LINK ReplaceClef(Document *, LINK, INT16, char);
Boolean ClefBeforeBar(Document *, LINK, char, INT16);
LINK ReplaceKeySig(Document *, LINK, INT16, INT16);
Boolean KeySigBeforeBar(Document *, LINK, INT16, INT16);
void ReplaceTimeSig(Document *, LINK, INT16, INT16, INT16, INT16);
Boolean TimeSigBeforeBar(Document *, LINK, INT16, INT16, INT16, INT16);

void AddDot(Document *, LINK, INT16);
Boolean NewArpSign(Document *, Point, char, INT16, INT16);
Boolean NewLine(Document *, INT16, INT16, char, INT16, INT16, LINK);

void NewGraphic(Document *, Point, char, INT16, INT16, INT16, Boolean,
						INT16, INT16, INT16, INT16, Boolean, unsigned char *, unsigned char *, INT16);
void NewMeasure(Document *, Point, char);
LINK CreateMeasure(Document *, LINK, INT16, INT16, CONTEXT);
void AddToClef(Document *, char, INT16);
void NewClef(Document *, INT16, char, INT16);
void NewPseudoMeas(Document *, Point, char);
INT16 ModNRPitchLev(Document *, Byte, LINK, LINK);
void NewMODNR(Document *, char, INT16, INT16, INT16, LINK, LINK);
void AddXSysDynamic(Document *, INT16, INT16, char, INT16, INT16, LINK);
LINK AddNewDynamic(Document *, INT16, INT16, DDIST *, char, INT16 *, PCONTEXT, Boolean); 
void NewRptEnd(Document *, INT16, char, INT16);

void NewKeySig(Document *, INT16, INT16, INT16);
void NewTimeSig(Document *, INT16, char, INT16, INT16, INT16, INT16);
void NewDynamic(Document *, INT16, INT16, char, INT16, INT16, LINK, Boolean);
void NewEnding(Document *, INT16, INT16, char, INT16, STDIST, LINK, INT16, INT16);

void NewTempo(Document *, Point, char, INT16, STDIST, Boolean, INT16, Boolean,
					unsigned char *, unsigned char *);
void NewSpace(Document *, Point, char, INT16, INT16, STDIST);

