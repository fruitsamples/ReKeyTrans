/*******************************************************************************	OpenWindow by Cameron	(OpenWindow? Cameron? What's going on here?)	OK, intrepid developer-like objects, we admit it! We don't	know where this sample came from and we don't know anything about	its original intent. However, since we're super-geniuses, we were	able to make up a plausible story.	Say you're a terminal emulator. And you have to run on a Mac Plus.	With us so far? Now, the Plus doesn't have a control key, but you'd	really like to pretend it does. So you use the option key. Here's where	the problem comes in.	The default behavior of KeyTrans is to support umlauts and other	fun letter modifiers by making option-U, for example, into a	"dead" key which results in no keystrokes but modifies subsequent	keystrokes. So when you type option-U followed by a u, you get: '�'.	And there was much rejoicing, except in Terminal Emulator Land.	There, the people were downtrodden, as they had no way to tell	their UNIX command line to clear.	So, you ask, why don't I just install my own keymap? Well, that'd	be swell for all us qwerty people, but the people who type on those	keyboards with all the keys rearranged would really hate it (Hi Quinn!)	and they'd have to relearn the icky qwerty (try typing that 5 times	fast) layout again after they'd gone to all the effort of rearranging	their keycaps on the keyboard so they'd feel funny (they're not all	the same shape, you know). So...		This sample shows how to patch KeyTrans in order to completely	ignore the dead-key processing that goes on and lets you type those	fancy accented characters. You're back in 7-bit ASCII days now,	bucko. Enjoy.*******************************************************************************/#include <String.h>/* Type 1 includes */#include <Types.h>#include <QuickDraw.h>/* Type 2 includes */#include <Controls.h>#include <Events.h>#include <Fonts.h>#include <Memory.h>#include <Menus.h>#include <OSUtils.h>#include <Resources.h>#include <SegLoad.h>#include <TextEdit.h>#include <ToolUtils.h>#include <Traps.h>#include <Script.h>/* Type 3 includes */#include <Desk.h>#include <Files.h>#include <OSEvents.h>#include <Windows.h>/* Type 4 includes */#include <Dialogs.h>/* Type BITE ME includes */#include <Traps.h>#include <Assembler.h>/*  Global Variables  */Str255			WindTitle;WindowPtr		MyWindow,aWindPtr;short			err,keycode,StartV,StartH;EventRecord		MyEvent;Boolean			quit,DrawOn;Rect			WindRect,Rect1,Rect2;Point			aPoint,lastPoint;long			KCHRID,newKey,state;Handle			KCHRHdl;/*************************************************************************************/static asm pascal UniversalProcPtr KeyTransPatcher (UniversalProcPtr){	// compute patch code size	LEA		endPatch,A1	LEA		patch,A0	MOVE.L	A1,D0	SUB.L	A0,D0	MOVE.L	D0,-(A7)				// save size for later	_NewPtrSys						// alloc block in sys heap for patch	MOVE.L	(A7)+,D0				// restore size for BlockMove	MOVE.L	A0,D1					// set up CCR (I thought 68K was CISC!)	BEQ.S	fail					// if'n (_NewPtrSys) didn't work, give up	MOVE.L	A0,A1					// new block is BlockMove dest	// hijack A0 temporarily, thank you	LEA		oldTrapAddress,A0		// point to PC-relative storage	MOVE.L	4(A7),(A0)				// set PC-relative storage	LEA		patch,A0				// source for BlockMove	_BlockMove						// copy patch to system heap	BRA.S	out						// skip exception handlingfail:	SUB.L	A1,A1					// tell caller we're hosedout:	MOVE.L	(A7)+,A0				// get return address	ADDQ.L	#4,A7					// drop arg	MOVE.L	A1,(A7)					// set return value	JMP		(A0)					// phone homepatch:	// set the up/down bit - KeyTrans won't process	// "dead" up strokes. Currently, this does not	// affect the event record up/down state.	BSET	#7,9(SP)	MOVE.L	oldTrapAddress,-(A7)	RTSoldTrapAddress:	DC.L	'hack'endPatch:}/*************************************************************************************/static void	InitMac(void){	InitGraf(&qd.thePort);	InitFonts();	FlushEvents(everyEvent,0);	InitWindows();	InitCursor();	quit = false;	DrawOn = false;	StartV = 25;	StartH = 5;}/*************************************************************************************/static void	DoCR(void){	if (lastPoint.v > (*MyWindow).portRect.bottom-15)	{		EraseRect(&(*MyWindow).portRect);		lastPoint.v = StartV;		lastPoint.h = StartH;	}	else	{		lastPoint.v = lastPoint.v+14;		lastPoint.h = StartH;	}}/*************************************************************************************/static void	DoKeyDown(void){	if (BitAnd(MyEvent.message,charCodeMask) == 'q'		// if the user typed the magic "cmd-q" sequence,	&& BitAnd(MyEvent.modifiers,0x0100))		{ quit = true; }								// it's Miller time!	else	{		MoveTo(lastPoint.h,lastPoint.v);		newKey = BitAnd(MyEvent.message,keyCodeMask);	// Strip out all but the keycode		newKey = BitShift(newKey,-8);					// move the keycode down to the low byte		keycode = LoWord(newKey);						// extract the low word of the event message (ignore modifier bits)		keycode = keycode & 0xFF7F;						// make sure it looks like a key down stroke		newKey = KeyTranslate(*KCHRHdl, keycode, (UInt32*)&state);	// have KeyTrans translate the key code to ASCII		keycode = LoWord(newKey);						// now get the ASCII 2 value (IM5, p195 has it backward!)		DrawChar(keycode);								// just to make sure, let's have a look		GetPen(&lastPoint);								// now make sure we stay inside the content region		if (lastPoint.h > ((*MyWindow).portRect.right-10))	// if we're getting to close to the edge,			{ 	DoCR();	}								// go fix it up	}}/*************************************************************************************/void main(){	InitMac();		MyWindow = GetNewWindow(2009,nil,(WindowPtr)-1);	if (MyWindow)	{		SetPort(MyWindow);		MoveTo(StartH,StartV);		lastPoint.h = StartH;		lastPoint.v = StartV;		KCHRID = GetScriptVariable(smRoman, smScriptKeys);	/* returns resource ID of KCHR being used by system */		KCHRHdl = GetResource('KCHR',KCHRID);		if (KCHRHdl)		{			UniversalProcPtr	oldKeyTrans = GetToolTrapAddress (_KeyTrans),								newKeyTrans = KeyTransPatcher (oldKeyTrans);			if (newKeyTrans)			{				SetToolTrapAddress (newKeyTrans, _KeyTrans);				MoveHHi(KCHRHdl);				HLock(KCHRHdl);				while (quit != true)				{					if (DrawOn)					{						if (StillDown())						{							GetMouse(&aPoint);							StdLine(aPoint);						}						else { DrawOn = false; }					}					else					{						if (WaitNextEvent(everyEvent,&MyEvent,0,nil))						{							switch (MyEvent.what)							{								case mouseDown:								{									Rect boundsRect = qd.screenBits.bounds;									switch (FindWindow(MyEvent.where,&aWindPtr))									{										case inGoAway :											quit = TrackGoAway (aWindPtr,MyEvent.where);											break;										case inDrag :											InsetRect (&boundsRect,4,4);											DragWindow (aWindPtr,MyEvent.where,&boundsRect);											break;																				}									break;								}								case keyDown:								{									DoKeyDown();									break;								}								case autoKey:								{									DoKeyDown();									break;								}							}						}					}				}				SetToolTrapAddress (oldKeyTrans, _KeyTrans);				DisposePtr ((Ptr) newKeyTrans);			}		}	}}