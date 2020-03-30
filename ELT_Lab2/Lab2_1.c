/*
 * Lab2_1.c
 *
 *  Created on:     30.03.2020
 *  Author:         Dinera
 *  Description:    Binaerzaehler
 */

#include "DSP28x_Project.h"
0void main(void) {
    unsigned int zaehler = 0;
    unsigned int ausgabe = 0;
    unsigned long i;                        // Timer Zaehlvariable deklarieren
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     //Ausgang setzen
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen
    SysCtrlRegs.WDCR = 0xE8;                // WD abschalten
    EDIS;

    while(1)
    {
        // LEDs auschalten
        GpioDataRegs.GPASET.bit.GPIO17 = 1;
        GpioDataRegs.GPBSET.all |= 0x8C0000;

        // Zuweisung ausgabe
        ausgabe = zaehler;
        // LEDs entsprechend der Variable "zaehler" setzen
        GpioDataRegs.GPACLEAR.bit.GPIO17 = (ausgabe & 0x1); // P1
        ausgabe = (ausgabe >> 1);
        GpioDataRegs.GPBCLEAR.bit.GPIO50 = (ausgabe & 0x1); // P2
        ausgabe = (ausgabe >> 1);
        GpioDataRegs.GPBCLEAR.bit.GPIO51 = (ausgabe & 0x1); // P3
        ausgabe = (ausgabe >> 1);
        GpioDataRegs.GPBCLEAR.bit.GPIO55 = (ausgabe & 0x1); // P4

        // ca. 200ms warten
        for(i=0;i<8923;i++);

        // zaehler erhoehen
        zaehler++;

        //  Bedingung fuer Maximalwert und Ruecksetzung
        if (zaehler > 15) {
            zaehler = 0;
        }
    }
}
