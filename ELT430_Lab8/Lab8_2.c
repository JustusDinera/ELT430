/*
 * Lab8_1.c
 *
 *  Created on:     05.05.2020
 *  Author:         Dinera
 *  Description:    K.I.T.T Lauflicht (mit Watchdog Interrupt)
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);
__interrupt void mein_Watchdog_ISR(void);

void main(void)
{
    /*** Initialisierung                    */
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (Uint32)&RamfuncsLoadSize); // kopieren des Programms in den RAM zur Laufzeit um schnellere Zufriffszeiten zu nutzen.

    /* Initialisierung Clock (90MHz)        */
    InitSysCtrl();

    /* Performence optimierung fuer FLASH */
    InitFlash();

    /* Ininitalisierung Interrupts          */
    InitPieCtrl();
    InitPieVectTable();

    /* Initialisierung CPU-Timer0 (90MHz) auf 200ms */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 200000);
    CpuTimer0Regs.TCR.bit.TSS = 0;

    /** Initialisierung kritische Register  	*/
    /* Initialisierung Watchdog             	*/
    EALLOW;                                 	// Sperre von kritischen Registern entriegeln
    /* Watchdog*/
    SysCtrlRegs.WDCR = 0x2D;                	// Watchdog einschalten und den Prescaler auf 16 setzen
    SysCtrlRegs.SCSR |= 2;		    	// Watchdog Interrupt einschalten

    /* Initialisierung der GPIO Pins        	*/
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;     	// Ausgang setzen (GPIO 43 [Build-in LED])
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     	// Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     	// Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    	// Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Freigabe Interrupts		  	*/
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen (Timer0)
    PieVectTable.WAKEINT = &mein_Watchdog_ISR;  // Eigene ISR aufrufen (WD)

    EDIS;                                  	// Sperre bei kritischen Registern setzen.
    asm("   CLRC INTM");                    	// Freigabe der Interrupts
    IER |= 1;                               	// INT1 freigeben

    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;          // Interrupt-Leitung 7 freigeben (Timer0)
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen (Timer0)

    PieCtrlRegs.PIEIER1.bit.INTx8 = 1; 		// Interrupt-Leitung 8 freigeben (WD)
    PieVectTable.WAKEINT = &mein_Watchdog_ISR;  // Eigene ISR aufrufen (WD)

    /*   Low Power Mode einstellen              */
    SysCtrlRegs.LPMCR0.bit.LPM = 0; // IDLE Mode aktivieren

    /*** Main loop                          */
    while(1)
    {
        // IDLE Mode aktivieren
        asm("   IDLE");

        // Bediehnung des Watchdogs (Good Key part 1)
        EALLOW;
        SysCtrlRegs.WDKEY = 0xAA;
        EDIS;
    }
}

/* Interrupt Routine Timer0		*/
__interrupt void mein_CPU_Timer_0_ISR(void)
{
    /* Variablen Deklaration                	*/
    static unsigned int position = 1;           // LED Position
    static unsigned int richtung;               // Richtung des Lauflichts
    static unsigned int schalter;               // Variable fuer Taster

    // Bediehnung des Watchdogs (Good Key part 2)
    EALLOW;
    SysCtrlRegs.WDKEY = 0x55;
    EDIS;

    // Schalter S1 auslesen
    schalter = GpioDataRegs.GPADAT.bit.GPIO12;

    if (schalter)
    {
        // LEDs auschalten
        GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;
        GpioDataRegs.GPBCLEAR.all |= 0x8C0000;

        // LEDs entsprechend der Variable "position" setze
        switch (position) {
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
            position = (position >> 1);
        } else {
            position = (position << 1);
        }
    }

    // Freigabe Interrupt (Acknowledge Flag)
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
}

/* Interrupt Routine Watchdog		*/
__interrupt void mein_Watchdog_ISR(void)
{
  GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;	// Build in LED schalten
  // Freigabe Interrupt (Acknowledge Flag)
  PieCtrlRegs.PIEACK.bit.ACK1 = 1;
}


