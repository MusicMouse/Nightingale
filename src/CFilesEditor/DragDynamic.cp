/* DragDynamic for Nightingale, by John Gibson - drag non-hairpin dynamics */

/*											NOTICE
 *
 * THIS FILE IS PART OF THE NIGHTINGALE™ PROGRAM AND IS CONFIDENTIAL PROP-
 * ERTY OF ADVANCED MUSIC NOTATION SYSTEMS, INC.  IT IS CONSIDERED A TRADE
 * SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT RECEIVED
 * WRITTEN AUTHORIZATION FROM THE OWNER.
 * Copyright © 1992-97 by Advanced Music Notation Systems, Inc. All Rights Reserved.
 */

#include "Nightingale_Prefix.pch"
#include "Nightingale.appl.h"

#define USE_BITMAP	/* use offscreen bitmap when dragging; otherwise use srcXor drawing mode */

static void InitDynamicBounds(Document *, LINK, Point, Rect *);


void DragDynamic(Document *doc, LINK dynamL)
{
	Point			oldPt, newPt, origPt;
	long			aLong;
	INT16			staffn, dh, dv, dhTotal, dvTotal, oldTxMode;
	Rect			oldObjRect, newObjRect, boundsRect;		/* in paper coords */
	Boolean		dynamVis, firstTime = TRUE, suppressRedraw = FALSE,
					stillWithinSlop, horiz, vert;
	CONTEXT		context[MAXSTAVES+1];
	LINK			dynSubL, measL;
	DYNAMIC		origDynObj;
	PDYNAMIC		thisDynObj;
	ADYNAMIC		origDynSubObj;
	PADYNAMIC	thisDynSubObj;
#ifdef USE_BITMAP
	GrafPtr		dynamPort, scrnPort;
	Rect			destRect;
	Point			grPortOrigin;
	INT16			fontID;
	const BitMap *dynamPortBits = NULL;
	const BitMap *scrnPortBits = NULL;
#endif
	Rect			bounds;

PushLock(OBJheap);
PushLock(DYNAMheap);

	/* Deselect dynamic to prevent flashing when user clicks outside of
	 * dynamic, and to avoid hiliting original dynamic when autoscrolling.
	 */	
	DeselectNode(dynamL);
	GetPaperMouse(&oldPt, &doc->currentPaper);

	thisDynObj = GetPDYNAMIC(dynamL);
	origDynObj = *thisDynObj;
	oldObjRect = LinkOBJRECT(dynamL);

	dynSubL = FirstSubLINK(dynamL);
	thisDynSubObj = GetPADYNAMIC(dynSubL);
	origDynSubObj = *thisDynSubObj;
	staffn = DynamicSTAFF(dynSubL);
	dynamVis = thisDynSubObj->visible;
	
	measL = LSSearch(dynamL, MEASUREtype, staffn, GO_LEFT, FALSE);		/* meas containing dynamic */
	GetAllContexts(doc, context, measL);
	
	InitDynamicBounds(doc, dynamL, oldPt, &boundsRect);

	oldTxMode = GetPortTxMode();

#ifdef USE_BITMAP

#ifdef USE_GRAFPORT
	dynamPort = NewGrafPort(oldObjRect.right-oldObjRect.left, oldObjRect.bottom-oldObjRect.top);
	if (dynamPort==NULL)
		return;

	GetPort(&scrnPort);
	fontID = GetPortTxFont();
	SetPort(dynamPort);
	TextFont(fontID);

	/* Fiddle with bitmap coordinate system so that DrawGRAPHIC will draw into our tiny rect. */
	grPortOrigin = TOP_LEFT(oldObjRect);
	Pt2Window(doc, &grPortOrigin);
	SetOrigin(grPortOrigin.h, grPortOrigin.v);
	GetPortBounds(dynamPort,&bounds);
	ClipRect(&bounds);

	DrawDYNAMIC(doc, dynamL, context, TRUE);			/* draw dynamic into bitmap */

	SetPort(scrnPort);

	destRect = oldObjRect;
	Rect2Window(doc, &destRect);
#else
	GetPort(&scrnPort);
	fontID = GetPortTxFont();
	
	SaveGWorld();
	
	GWorldPtr gwPtr = MakeGWorld(oldObjRect.right-oldObjRect.left, oldObjRect.bottom-oldObjRect.top, TRUE);
	SetGWorld(gwPtr, NULL);
	dynamPort = gwPtr;

	SetPort(dynamPort);
	TextFont(fontID);

	/* Fiddle with bitmap coordinate system so that DrawGRAPHIC will draw into our tiny rect. */
	grPortOrigin = TOP_LEFT(oldObjRect);
	Pt2Window(doc, &grPortOrigin);
	SetOrigin(grPortOrigin.h, grPortOrigin.v);
	GetPortBounds(dynamPort,&bounds);
	ClipRect(&bounds);

	DrawDYNAMIC(doc, dynamL, context, TRUE);			/* draw dynamic into bitmap */
	
	RestoreGWorld();
	
	destRect = oldObjRect;
	Rect2Window(doc, &destRect);
#endif // USE_GRAFPORT

#else
	TextMode(srcXor);
#endif

	FlushEvents(everyEvent, 0);

	origPt = newPt = oldPt;
	stillWithinSlop = TRUE;
	horiz = vert = TRUE;
	if (StillDown()) while (WaitMouseUp()) {
		GetPaperMouse(&newPt, &doc->currentPaper);

		/* Force the code below to see the mouse as always within boundsRect. */
		aLong = PinRect(&boundsRect, newPt);
		newPt.v = HiWord(aLong);	newPt.h = LoWord(aLong);

		if (EqualPt(newPt, oldPt)) continue;
		if (stillWithinSlop) {
			/* If mouse is still within slop bounds, don't do anything. Otherwise,
				decide whether horizontally/vertically constrained. */
			dhTotal = newPt.h - origPt.h;
			dvTotal = newPt.v - origPt.v;
			if (ABS(dhTotal)<2 && ABS(dvTotal)<2) continue;
			if (ShiftKeyDown()) {
				horiz = ABS(dhTotal) > ABS(dvTotal);		/* 45 degree movement => vertical */
				vert = !horiz;
			}
			/* And don't ever come back, you hear! */
			stillWithinSlop = FALSE;
		}
		dh = (horiz? newPt.h - oldPt.h : 0);
		dv = (vert? newPt.v - oldPt.v : 0);
				
#if 0
/*
 *	Wait until a tick, which is synchronised with the start of a vertical
 * raster scan. This is quick and dirty but helps some. NB: slotted Macs
 * may require a VBL task to be more accurate.
 */
		now = TickCount(); while (now == TickCount());
#endif

#ifdef USE_BITMAP
		if (firstTime) {
			firstTime = FALSE;
		}
		else {
			dynamPortBits = GetPortBitMapForCopyBits(dynamPort);
			scrnPortBits = GetPortBitMapForCopyBits(scrnPort);
			GetPortBounds(dynamPort,&bounds);
			
			CopyBits(dynamPortBits, scrnPortBits, &bounds, &destRect, srcXor, NULL);

//			CopyBits(&dynamPort->portBits, &scrnPort->portBits,		/* erase old dynamic */
//		 				&(*dynamPort).portBits.bounds, &destRect, srcXor, NULL);
		}
#else
		if (firstTime)							/* ???if we get it to draw in gray above, remove this firstTime business */
			firstTime = FALSE;
		else
			DrawDYNAMIC(doc, dynamL, context, TRUE);				/* erase old dynamic */
#endif

		thisDynObj->xd += p2d(dh);
		thisDynSubObj->yd += p2d(dv);

		/* We need to make the dynamic temporarily invisible;
		 * otherwise AutoScroll will leave a trail of text images as we scroll.
		 * ???NB: also need to turn off ShowInvisibles if it's on!!
		 */
		TextMode(srcOr);													/* so staff lines won't cut through notes */
		thisDynSubObj->visible = FALSE;
		AutoScroll();
		thisDynSubObj->visible = dynamVis;
#ifndef USE_BITMAP
		TextMode(srcXor);
#endif

#ifdef USE_BITMAP
		/* shift on-screen destination rect */
		destRect.top += dv;		destRect.bottom += dv;
		destRect.left += dh;		destRect.right += dh;
		
		dynamPortBits = GetPortBitMapForCopyBits(dynamPort);
		scrnPortBits = GetPortBitMapForCopyBits(scrnPort);
		GetPortBounds(dynamPort,&bounds);
		
		CopyBits(dynamPortBits, scrnPortBits, &bounds, &destRect, srcXor, NULL);
			
//		CopyBits(&dynamPort->portBits, &scrnPort->portBits,		/* draw new dynamic */
//		 			&(*dynamPort).portBits.bounds, &destRect, srcXor, NULL);
#else
		DrawDYNAMIC(doc, dynamL, context, TRUE);				/* draw new dynamic */
#endif
		
		oldPt = newPt;
	}
	TextMode(srcOr);
	LinkVALID(dynamL) = FALSE;										/* force objRect recomputation */
	DrawDYNAMIC(doc, dynamL, context, TRUE);					/* draw final black dynamic */

	TextMode(oldTxMode);

	/* Inval old and new location */
	newObjRect = LinkOBJRECT(dynamL);							/* DrawDYNAMIC has updated this */
	Rect2Window(doc, &newObjRect);
	if (BlockCompare(thisDynObj, &origDynObj, sizeof(DYNAMIC)) ||
		 BlockCompare(thisDynSubObj, &origDynSubObj, sizeof(ADYNAMIC))) {
		doc->changed = TRUE;
		LinkTWEAKED(dynamL) = TRUE;								/* Flag to show node was edited */
		Rect2Window(doc, &oldObjRect);
		InsetRect(&oldObjRect, -pt2p(5), -pt2p(3));			/* ??wouldn't have to be 4 if objrects far enuf to the right for wide dynamics (fff, ppp) */ 
		if (!suppressRedraw)
			EraseAndInval(&oldObjRect);
	}
	InsetRect(&newObjRect, -pt2p(5), -pt2p(3));
	EraseAndInval(&newObjRect);					/* must do this even if no change, to keep hiliting in sync */

#ifdef USE_BITMAP

#ifdef USE_GRAFPORT
	DisposGrafPort(dynamPort);
#else
	DestroyGWorld(gwPtr);
#endif // USE_GRAFPORT

#endif

	MEHideCaret(doc);
	LinkSEL(dynamL) = TRUE;								/* so that dynamic will hilite after we're done dragging */
	thisDynObj->selected = TRUE;						/* mark obj and subobj as selected */
	DynamicSEL(dynSubL) = TRUE;
	LinkVALID(dynamL) = FALSE;							/* force objRect recomputation */
	doc->selStartL = dynamL;	doc->selEndL = RightLINK(dynamL);

PopLock(OBJheap);
PopLock(DYNAMheap);
}


/* ------------------------------------------------------ InitDynamicBounds -- */
/* Set up the rectangle beyond which the user won't be allowed to drag the dynamic. */

#define SYSHT_SLOP 128		/* DDIST */

static void InitDynamicBounds(Document *doc, LINK dynamL,
										Point mousePt, Rect *bounds)	/* paper coords */
{
	INT16			staffn, mouseFromLeft, mouseFromRight;
	Rect			objRect;
	CONTEXT		context;
	PAMEASURE	aMeasP;
	DDIST			sysLeft, sysTop, sysRight, sysBot, measWid;
	LINK			firstSyncL, measL, prevMeasL, nextMeasL, targetMeasL;

	objRect = LinkOBJRECT(dynamL);

	mouseFromLeft = objRect.left - mousePt.h;
	mouseFromRight = objRect.right - mousePt.h;	/* ???this will need correction due to bad objRect width */
	
	staffn = DynamicSTAFF(FirstSubLINK(dynamL));
	GetContext(doc, dynamL, staffn, &context);
	sysLeft = context.systemLeft;
	sysTop = context.systemTop;
	sysRight = context.staffRight;
	sysBot = context.systemBottom;
	
	firstSyncL = DynamFIRSTSYNC(dynamL);

	/* largest acceptible rect */
	bounds->left = d2p(sysLeft);
	bounds->top = d2p(sysTop - SYSHT_SLOP);
	bounds->right = d2p(sysRight);
	bounds->bottom = d2p(sysBot + SYSHT_SLOP);
	
	/* Constrain further, so that the dynamic will never be farther
	 * than one measure away from its firstSync.
	 */

	/* Find the measure that will be the left boundary */
	measL = LSSearch(firstSyncL, MEASUREtype, staffn, GO_LEFT, FALSE);		/* meas containing 1st sync */
	prevMeasL = LinkLMEAS(measL);															/* meas before that */
	if (SameSystem(prevMeasL, measL))
		targetMeasL = prevMeasL;
	else
		targetMeasL = measL;
		
	/* Allow dragging into reserved area */
	measL = LSSearch(MeasSYSL(targetMeasL), MEASUREtype, staffn, GO_RIGHT, FALSE);	/* 1st meas in system */	
	if (targetMeasL == measL)																/* targetMeas is 1st in system */
		bounds->left = d2p(sysLeft);
	else
		bounds->left = d2p(LinkXD(targetMeasL) + sysLeft);

	/* Find the measure that will be the right boundary */
	measL = LSSearch(firstSyncL, MEASUREtype, staffn, GO_LEFT, FALSE);		/* meas containing 1st sync */
	nextMeasL = LinkRMEAS(measL);															/* meas after the one containing 1st sync */
	if (SameSystem(nextMeasL, measL))
		targetMeasL = nextMeasL;
	else
		targetMeasL = measL;
	aMeasP = GetPAMEASURE(FirstSubLINK(targetMeasL));								/* get xd of END of meas */
	measWid = aMeasP->measureRect.right;
	bounds->right = d2p(LinkXD(targetMeasL) + measWid + sysLeft);

	/* Offset boundaries depending on mouse position relative to
	 *	dynamic. (PinRect in DragDynamic decides whether
	 *	mousePt, not dynamic, is within bounds.)
	 */
	bounds->left -= mouseFromLeft;
	bounds->right -= mouseFromRight-1;
}