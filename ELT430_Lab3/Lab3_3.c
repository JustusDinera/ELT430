/*
 * Lab3_3.c
 *
 *  Created on:     09.04.2020
 *  Author:         Dinera
 *  Description:    K.I.T.T Lauflicht (mit Stop bei Tastendruck)
 */

#include "DSP28x_Project.h"
void main(void) {
    /** Variablen Deklaration                */
    unsigned int zaehler = 1;               // Zaehler-Variable
    unsigned int richtung;                  // Richtung des Lauflichts
    unsigned long i;                        // Timer Zaehlvariable deklarieren
    unsigned int schalter;                  // Variable fuer Rueckgabewert von S1

    /*** Initialisierung                    */
    /* Initialisierung Clock (90MHz)        */
    InitSysCtrl();

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x2D;                // Watchdog einschalten und den Prescaler auf 16 setzen

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])
    EDIS;                                   // Sperre bei kritischen Registern setzen.

    /*** Main loop                          */
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

        // Shift Operation entsprechend der Richtung
        if (richtung) {
            zaehler = (zaehler >> 1);
        } else {
            zaehler = (zaehler << 1);
        }

        // ca. 200ms warten (10MHz)
        //for(i=0;i<142853;i++);

        // ca. 200ms warten (90MHz)
        for(i=0;i<1285710;i++);

        do {                                // Schleife fuer Schalter S1
        // Bediehnung des Watchdogs
            EALLOW;
            SysCtrlRegs.WDKEY = 0x55;
            SysCtrlRegs.WDKEY = 0xAA;
            EDIS;
            schalter = GpioDataRegs.GPADAT.bit.GPIO12; // Schalter S1 auslesen
        } while (!schalter);
    }
}
