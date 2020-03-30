/*
 * Lab1_1.c
 *
 *  Created on:     11.03.2020
 *  Author:         Dinera
 *  Description:    Wechselseitiges Blinken der 4 LEDs auf dem Zusatzboard im 0,5 Sekunden Takt
 */

#include "DSP28x_Project.h"
void main(void) {
    unsigned long i;                        // Timer Zaehlvariable deklarieren
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     //Ausgang setzen
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;     // Ausgang setzen
    SysCtrlRegs.WDCR = 0xE8;                // WD abschalten
    EDIS;
    GpioDataRegs.GPASET.bit.GPIO17 = 1;     // LED P1 ein
    GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1;   // LED P2 aus
    GpioDataRegs.GPBSET.bit.GPIO51 = 1;     // LED P3 ein
    GpioDataRegs.GPBCLEAR.bit.GPIO55 = 1;   // LED P4 aus

    while(1)
    {
        for(i=0;i<360000;i++);
        GpioDataRegs.GPBTOGGLE.all = 0x8C0000;  // LED P2, P3 und P4 toggeln
        GpioDataRegs.GPATOGGLE.bit.GPIO17 = 1;  // LED P1 toggeln
    }
}
