/*
 * Lab13_1A.c
 *
 *  Created on:     16.06.2020
 *  Author:         Dinera
 *  Description:    PWM - Signal Messung
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0
void fn_ePWM1A_Init(void);                      // Initial Funktion fuer PWM A1 (GPIO0)
void fn_eCAP1_Init(void);                       // Initial Funktion fuer eCAP1  (GPIO24)

void main(void)
{
    /*** Initialisierung                    */
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (Uint32)&RamfuncsLoadSize); // kopieren des Programms in den RAM zur Laufzeit und schnellere Zufriffszeiten zu nutzen.

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

    /* Initialisierung ePWM1A               */
    fn_ePWM1A_Init();

    /* Initialisierung eCAP1 Modul */
    fn_eCAP1_Init();

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x28;                // Watchdog einschalten

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen
    EDIS;                                   // Sperre bei kritischen Registern setzen.
    asm("   CLRC INTM");                    // Freigabe der Interrupts
    IER |= 1;                               // INT1 freigeben
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen

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

__interrupt void mein_CPU_Timer_0_ISR(void)
{
    /* Variablen Deklaration                */
    static unsigned int position = 1;               // LED Position
    static unsigned int richtung;                  // Richtung des Lauflichts
    static unsigned int schalter;                   // Variable fuer Taster

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
    PieCtrlRegs.PIEACK.all = 1;
}

/* Initial Funktion fuer PWM A1 (GPIO0) */
void fn_ePWM1A_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;           //  GPAMUX1 auf ePWM1A setzen
    EDIS;

    // // 500Hz
    //EPwm1Regs.TBPRD = 45026;           // Periode auf 45000 Takte setzen
    //EPwm1Regs.TBCTL.bit.CLKDIV = 1;     // Clockdivider auf den Wert 2 setzen
     // 1kHz
    EPwm1Regs.TBPRD = 45026;           // Periode auf 45000 Takte setzen
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen
    // // 2kHz
    //EPwm1Regs.TBPRD = 22500;           // Periode auf 22500 Takte setzen
    //EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen
    // // 5kHz
    //EPwm1Regs.TBPRD = 9000;           // Periode auf 9000 Takte setzen
    //EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen
    // // 20kHz
    //EPwm1Regs.TBPRD = 2250;           // Periode auf 2250 Takte setzen
    //EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen
    // // 50kHz
    //EPwm1Regs.TBPRD = 900;           // Periode auf 900 Takte setzen
    //EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm1Regs.AQCTLA.bit.ZRO = 2;      // ZRO auf "Set: force EPWMxA output high" setzen
    EPwm1Regs.AQCTLA.bit.PRD = 1;       // PRD auf "Clear: force EPWMxA output low" setzen
}

// Initial Funktion fuer eCAP1  (GPIO24)
void fn_eCAP1_Init(void){
    ECap1Regs.ECCTL1.bit.FREE_SOFT =0;  // stoppt bei Debugging Brakepoint
    ECap1Regs.ECCTL1.bit.PRESCALE = 0;  // Prescaler umgehen (Divider = 1)
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;   // CAP1 bis CAP4 bei Capture Events laden
    ECap1Regs.ECCTL1.bit.CTRRST4 = 1;   // nach dem 4. Timestamp den Counter resetten
    ECap1Regs.ECCTL1.bit.CAP1POL = 0;   // Capture Trigger 1 bei steigender Flanke
    ECap1Regs.ECCTL1.bit.CAP2POL = 1;   // Capture Trigger 2 bei fallender Flanke
    ECap1Regs.ECCTL1.bit.CAP3POL = 0;   // Capture Trigger 3 bei steigender Flanke
    ECap1Regs.ECCTL1.bit.CAP4POL = 1;   // Capture Trigger 4 bei fallender Flanke
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 0;   // continues Mode aktivieren
    ECap1Regs.ECCTL2.bit.STOP_WRAP = 3; // Wrap nach dem Event 4
    ECap1Regs.ECCTL2.bit.REARM = 0;     // Re-Arm ausschalten
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1; // Time Stamp Counter in free-running Mode
    ECap1Regs.ECCTL2.bit.SYNCI_EN = 0;  // Synchronisation Features ausgeschalten
    ECap1Regs.ECCTL2.bit.CAP_APWM = 0;  // Modul im Capture-Mode laufen lassen
    ECap1Regs.ECEINT.bit.CEVT4 = 1;     // Interrupt bei Event 4 ausloesen

    EALLOW;
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 1;           //  GPIO24 (GPAMUX2) auf eCAP1 setzen
    EDIS;

}
