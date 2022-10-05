;// -----------------------------------------
;// Program entry point.
;//
;// COPYRIGHT (C) Texas Instruments, Alexey Poleschuk 2007-2008
;// _________________________________________
;//
;// DEPENDENCIES: 
;//		ZwCANBase.h
;// _________________________________________
;//
;// REVISION 	- 0
;// REV. DATE 	- 1.01.2008
;//
;// -----------------------------------------  

    .ref _c_int00
    .global code_start

***********************************************************************
* Function: codestart section
*
* Description: Branch to code starting point
***********************************************************************

    .sect "codestart"

code_start:
         LB wdd_and_run       ;Branch to watchdog disable code

;end codestart section


***********************************************************************
* Function: wdd_and_run
*
* Description: Disables the watchdog timer
***********************************************************************

    .text
wdd_and_run:
    SETC OBJMODE        ;Set OBJMODE for 28x object code
    EALLOW              ;Enable EALLOW protected register access
    MOVZ DP, #7029h>>6  ;Set data page for WDCR register
    MOV @7029h, #0068h  ;Set WDDIS bit in WDCR to disable WD
    EDIS                ;Disable EALLOW protected register access
    LB _c_int00         ;Branch to start of boot.asm in RTS library

;end wdd_and_run

    .end
	
;//===========================================================================
;// End of file.
;//===========================================================================
