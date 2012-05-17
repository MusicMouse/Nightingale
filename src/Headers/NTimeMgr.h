/* NTimeMgr.h - public prototypes for Nightingale Time Manager */

void NTMInit(void);
void NTMClose(void);
void NTMStartTimer(unsigned short delayTime);
void NTMStopTimer(void);
long NTMGetTime(void);

