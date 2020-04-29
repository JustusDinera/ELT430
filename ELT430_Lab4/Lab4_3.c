/*
 * Lab4_3.c
 *
 *  Created on:     09.04.2020
 *  Author:         Dinera
 *  Description:    K.I.T.T Lauflicht (mit Geschwindigkeitswechsel)
 */

#include "DSP28x_Project.h"
void main(void) {
    /** Variablen Deklaration                */
    unsigned int zaehler = 1;               // Zaehler-Variable
    unsigned int richtung;                  // Richtung des Lauflichts
    unsigned int schalter;                  // Variable fuer Rueckgabewert von S1
    unsigned int schalter_alt = 1;          // vorheriger (alter) Wert von Schalter
    unsigned int wiederholung_aktuell;      // Anzahl der Wiederholungen im aktuellen Durchlauf
    unsigned int wiederholung[6]= {1,2,4,10,20,40}; // Anzahl der Wiederholungen in einem Durchgang
    unsigned int geschwindigkeit = 0;       // wird durch druecken von S1 erhoeht
    unsigned int gedrueckt  = 0;            //

    /** Funktionsdeklaration                */
    void watchdog_bediehnen();

    /*** Initialisierung                    */

    /* Initialisierung Clock (90MHz)        */
    InitSysCtrl();

    /* Initialisierung CPU-Timer0 (90MHz) auf 50ms */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 50000);
    CpuTimer0Regs.TCR.bit.TSS = 0;

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
        // Bedingung ist erfuellt, wenn die eingestelle Zeit abgelaufen ist.
        if (CpuTimer0Regs.TCR.bit.TIF == 1)
        {
            // CPU-Timer0 zurueck setzen
            CpuTimer0Regs.TCR.bit.TIF = 1;

            // Watchdog bediehnen
            watchdog_bediehnen();

            // Schalter S1 auslesen
            schalter = GpioDataRegs.GPADAT.bit.GPIO12;

            // entprellen von S1
            if ((schalter == 1) && (schalter_alt == 1)) {    // ruecksetzten des "gedrueckt-Status"
                gedrueckt = 0;
            }

            if ((schalter == 0) && (schalter_alt == 0) && (gedrueckt == 0))  // Wenn S1 gedrueckt ist und vorher nicht gedrueckt war
            {
                // bei gedruecktem Taster S1 wird die Geschwindigkeit erhoeht
                if (geschwindigkeit < 5)
                {
                    geschwindigkeit++;
                }
                else
                {
                    geschwindigkeit = 0;
                }
                //
                gedrueckt = 1;
            }
            schalter_alt = schalter;

            // pruefen, ob die Anzahl an 50ms Durchlaeufen erreicht ist
            if (wiederholung_aktuell < wiederholung[geschwindigkeit])
            {
                // Erhoehen der aktuellen Druchlaeufe
                wiederholung_aktuell++;
            }
            else
            {
                // bei erreichen der
                wiederholung_aktuell = 0;

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
            }
        }
    }
}

// Bediehnung des Watchdogs
void watchdog_bediehnen()
{
    EALLOW;
    SysCtrlRegs.WDKEY = 0x55;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;
}

