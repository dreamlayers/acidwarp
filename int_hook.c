/* INT_HOOK.C For Microsoft C -- Provides functions for the
 *	automatic setting of an INT8 hook (Timer Interrupt)
 * (C)Copyright 1993 by Noah Spurrier
 */

/*
		  Interrupt Sequence

	8259							CPU
	----							---
	IRQ
	INT 					--->	INTR
									<pushf>
									<cli>
	INTA					<---	INTA
	VECT# 				--->	<call far (vect# * 4)>
	set inService bit
									sti
									other isr instrs.
	clr inService bit <---	out 20, eoi
									iret

	Notes:
	Caps show electrical signals
	Arrows show cause-->effect
	<...> show automatic actions performed by CPU
*/

#include <dos.h>
#include <conio.h>
#include "handy.h"
#include "acidwarp.h"

                      /* Number of times per tick that the background process is called.  */
#define NUM_BACKGROUND_REPS	1	/* Normal value = 1; >= 4 causes overruns during title screen.			*/

											/* Enables beep diagnostic for timer ISR overrun, i.e., a new interrupt	*/
#define BEEP_FOR_OVERRUN		0	/*	occurring before the ISR invocation from the last one has finished.	*/

											/* Enables STI during timer ISR.  Without it, overruns cannot occur.					*/
#define STI_DURING_ISR			0	/* Permits kbd svc, and thus Ctrl-NumLock pause, during ISR execution; this will	*/
											/* stop rotation, and cause a continuous overrun, until the pause is released.	*/

#define STD_DOS_TIMER_DIVISOR	0

UINT newTickFreqHz = NORMAL_DOS_TICK_FREQ;
int  TimerDelayCountdown = 0, RatioOfNewToOldTickFreq = 0, backgroundPaletteProcessNum = NONE;
void (__interrupt __far *origTimerHdweBiosIntVect08)(VOID);

void __interrupt _far newTimerHdweInt08Svc(void)
{
	static int countdownToCallOfTimerIsrInBios = 0, backgndRepNum, timerIsrAlreadyEntered = 0;

	if (TimerDelayCountdown)
	--TimerDelayCountdown;

	if (--countdownToCallOfTimerIsrInBios <= 0)
	{
		countdownToCallOfTimerIsrInBios = RatioOfNewToOldTickFreq;
		(*origTimerHdweBiosIntVect08)();		/* Sends EOI but does NOT do STI!	*/
	}
	else
		_outp (0x20, 0x20);		/* Send EOI	to 8259 */

	#if STI_DURING_ISR
		_asm sti
	#endif

	if (timerIsrAlreadyEntered)
		#if BEEP_FOR_OVERRUN
			beepOn(1000);
		#else
			;
		#endif
	else
	{
		beepOff();
		timerIsrAlreadyEntered = 1;
			for (backgndRepNum = 0; backgndRepNum < NUM_BACKGROUND_REPS; backgndRepNum++)
			{
				switch (backgroundPaletteProcessNum)	/* A bad value will just cause the ISR to return	*/
				{													/* without executing any background process.			*/
					case (NONE):
					break;
				
					case (ROLL):
            rollMainPalArrayAndLoadDACRegs();
					break;
				
					case (ROLL_AND_FADE_WHITE):
            rolNFadeWhtMainPalArrayNLoadDAC();
					break;
				
					case (ROLL_AND_FADE_BLACK):
            rolNFadeBlkMainPalArrayNLoadDAC();
					break;
				
					case (ROLL_AND_FADE_TO_TARGET):
            rolNFadeMainPalAryToTargNLodDAC();
					break;
				
					case (ROLL_AND_FADE_TO_RANDOM_TARGET):
            rolNFadMainPalAry2RndTargNLdDAC();
					break;
				}
			}
		timerIsrAlreadyEntered = 0;
	}
}

void beg_timer_hook (void)
{
	UINT newTimerDivisor;

	if (newTickFreqHz == NORMAL_DOS_TICK_FREQ)
	{
		newTimerDivisor = 0;
		RatioOfNewToOldTickFreq = 1;
	}
	else
	{
		newTimerDivisor = (UINT)(1193180L / newTickFreqHz);

		if (newTimerDivisor == 0)
			RatioOfNewToOldTickFreq = 1;
		else
			RatioOfNewToOldTickFreq = (UINT)65535L / newTimerDivisor;
	}

	origTimerHdweBiosIntVect08 = _dos_getvect (0x08);

	_disable();		
		_dos_setvect (0x08, newTimerHdweInt08Svc);
											
		_outp ((UINT)0x43, 0x36);	/* Set new timer interrupt frequency.  Tell timer to receive new divisor.	*/
		_outp ((UINT)0x40,   newTimerDivisor & 0x00ff);
		_outp ((UINT)0x40, ((newTimerDivisor & 0xff00) >> 8));
	_enable();
}

void end_timer_hook (void)
{
	_disable();							/* Restore old timer vector */
		_dos_setvect ((UINT)0x08, origTimerHdweBiosIntVect08);

		_outp ((UINT)0x43, 0x36);	/* Restore old timer frequency.  Tell timer to receive new divisor.	*/
		_outp ((UINT)0x40,  STD_DOS_TIMER_DIVISOR & 0x00ff);
		_outp ((UINT)0x40, (STD_DOS_TIMER_DIVISOR & 0xff00) >> 8);
	_enable();
}

void beepOn(UINT divisor)
{
	_outp(0x42, divisor & 0x00FF);		/* load timer 2 count reg, lsb, msb	*/
	_outp(0x42, divisor >> 8);
	_outp(0x61, _inp(0x61) | 3);	 /* timer gate, spkr data on, PIA port B	*/
}

void beepOff(void)
{
	_outp(0x61, _inp(0x61) & ~3); /* timer gate, spkr data off, PIA port B	*/
}

