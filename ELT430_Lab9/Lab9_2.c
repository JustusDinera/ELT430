/*
 * Lab9_2.c
 *
 *  Created on:     02.06.2020
 *  Author:         Dinera
 *  Description:    Phasenversetzte PWM Signale
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0
void fn_ePWM1A_Init(void);                      // Initial Funktion fuer PWM A1 (GPIO0)
void fn_ePWM2A_Init(void);                      // Initial Funktion fuer PWM A2 (GPIO2)
void fn_ePWM3A_Init(void);                      // Initial Funktion fuer PWM A3 (GPIO4)
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
    fn_ePWM2A_Init();
    fn_ePWM3A_Init();

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x2D;                // Watchdog einschalten und den Prescaler auf 16 setzen

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
    EPwm1Regs.TBCTL.bit.SYNCOSEL = 1;             //  Synchronisierungssignal bei TBPRD = 0
    EDIS;

    // // 1kHz
    EPwm1Regs.TBPRD = 45026;           // Periode auf 45026 Takte setzen
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen


    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm1Regs.AQCTLA.bit.ZRO = 2;      // ZRO auf "Set: force EPWMxA output high" setzen
    EPwm1Regs.AQCTLA.bit.PRD = 1;       // PRD auf "Clear: force EPWMxA output low" setzen
}

/* Initial Funktion fuer PWM A2 (GPIO2) */
void fn_ePWM2A_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;           //  GPAMUX1 auf ePWM2A setzen
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0;             //  Synchronisierungssignal bei EPWM1SYNC = 0
    EPwm2Regs.TBCTL.bit.PHSEN = 1;             //  Synchronisierungssignal bei TBPRD = 0
    EDIS;

    EPwm2Regs.TBPHS.half.TBPHS = 30016;          // Phasenversatz einstellen (120Grad)

    // // 1kHz
    EPwm2Regs.TBPRD = 45026;           // Periode auf 45026 Takte setzen
    EPwm2Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen


    EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm2Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm2Regs.AQCTLA.bit.ZRO = 2;      // ZRO auf "Set: force EPWMxA output high" setzen
    EPwm2Regs.AQCTLA.bit.PRD = 1;       // PRD auf "Clear: force EPWMxA output low" setzen
}

/* Initial Funktion fuer PWM A3 (GPIO4) */
void fn_ePWM3A_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;           //  GPAMUX1 auf ePWM3A setzen
    EPwm3Regs.TBCTL.bit.SYNCOSEL = 3;             //  Synchronisierungssignal bei EPWM1SYNC = 0
    EPwm3Regs.TBCTL.bit.PHSEN = 1;             //  Synchronisierungssignal bei TBPRD = 0
    EDIS;

    EPwm3Regs.TBPHS.half.TBPHS = 60032;          // Phasenversatz einstellen (240Grad)


    // // 1kHz
    EPwm3Regs.TBPRD = 45026;           // Periode auf 45026 Takte setzen
    EPwm3Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen


    EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm3Regs.AQCTLA.bit.ZRO = 2;      // ZRO auf "Set: force EPWMxA output high" setzen
    EPwm3Regs.AQCTLA.bit.PRD = 1;       // PRD auf "Clear: force EPWMxA output low" setzen
}
