/*
 * Lab3_1.c
 *
 *  Created on:     09.04.2020
 *  Author:         Dinera
 *  Description:    K.I.T.T Lauflicht
 */

#include "DSP28x_Project.h"
void main(void) {
    /** Variablen Deklaration                */
    unsigned int zaehler = 1;               // Zaehler-Variable
    unsigned int richtung;                  // Richtung des Lauflichts
    unsigned long i;                        // Timer Zaehlvariable deklarieren

    /** Initialisierung                     */
    /* Initialisierung Clock                */
    InitSysCtrl();

    /* Initialisierung der GPIO Pins        */
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17)
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50, 51, 55)
    SysCtrlRegs.WDCR = 0xE8;                // WD abschalten
    EDIS;

    while(1)
    {
        // LEDs auschalten
        GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;
        GpioDataRegs.GPBCLEAR.all |= 0x8C0000;



        // LEDs entsprechend der Variable "zaehler" setzen
        switch (zaehler) {
        case 1:
            GpioDataRegs.GPASET.bit.GPIO17 = 1; // P1
            richtung = 0;
            break;
        case 2:
            GpioDataRegs.GPBSET.bit.GPIO50 = 1; // P2
            break;
        case 4:
            GpioDataRegs.GPBSET.bit.GPIO51 = 1; // P3
            break;
        case 8:
            GpioDataRegs.GPBSET.bit.GPIO55 = 1; // P4
            richtung = 1;
            break;
        default:
                break;
        }

        if (richtung) {
            zaehler = (zaehler >> 1);
        } else {
            zaehler = (zaehler << 1);
        }

        // ca. 200ms warten
        for(i=0;i<142900;i++);

    }
}
