/*
 * Lab11_1.c
 *
 *  Created on:     06.06.2020
 *  Author:         Dinera
 *  Description:    PWM - Modulation Kanal A und B
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0 (LEDs)
__interrupt void mein_CPU_Timer_1_ISR(void);    //Interrupt Routune Timer1 (Pulsweite)
void fn_ePWM1_Init(void);                      // Initial Funktion fuer PWM A1 (GPIO0)

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

    /* Initialisierung CPU-Timer */
    InitCpuTimers();
    /* CPU-Timer0 (90MHz) auf 200ms */
    ConfigCpuTimer(&CpuTimer0, 90, 200000);
    CpuTimer0Regs.TCR.bit.TSS = 0;
    /* CPU-Timer1 (90MHz) auf 111us */
    ConfigCpuTimer(&CpuTimer1, 90, 111);
    CpuTimer1Regs.TCR.bit.TSS = 0;

    /* Initialisierung ePWM1A               */
    fn_ePWM1_Init();

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x2D;                // Watchdog einschalten und den Prescaler auf 16 setzen

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen (Timer0 / Lauflicht)
    PieVectTable.TINT1 = &mein_CPU_Timer_1_ISR; // ISR Timer1 (Pulsweitenanpassung)

    EDIS;                                   // Sperre bei kritischen Registern setzen.

    /* konfigurration Interrupts */
    asm("   CLRC INTM");                    // Freigabe der Interrupts
    IER |= 1 | (1<<12);                               // INT1 und INT13 freigeben

    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben (Timer0)


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

/* LED Timer (Timer0) */
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

    if (schalter)                       // Bedingung zum stoppen der Pulsbreitenaenderung
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

/* PWM Pulsbreiten Timer (Timer1) */
__interrupt void mein_CPU_Timer_1_ISR(void)
{
    unsigned int schalter = GpioDataRegs.GPADAT.bit.GPIO12; // Schalter S1 auslesen

    /* Variablen Deklaration                */
    static unsigned int pwm_richtung_a = 1;           // Zunahme (1) oder Abnahme (0) Richtung des CMPA Registers
    static unsigned int pwm_richtung_b = 1;           // Zunahme (1) oder Abnahme (0) Richtung des CMPB Registers

    // PWM1A
    // PWM1A Pulsebreite Richtung
    if ( EPwm1Regs.CMPA.half.CMPA >= EPwm1Regs.TBPRD){
        pwm_richtung_a = 0;
    }
    if (EPwm1Regs.CMPA.half.CMPA == 0){
        pwm_richtung_a = 1;
    }

    // PWM1A Pulsbreite aendern
    if (pwm_richtung_a) {
        EPwm1Regs.CMPA.half.CMPA++;
    } else {
        EPwm1Regs.CMPA.half.CMPA--;
    }

    // PWM1B
    if (schalter) {
        // PWM1B Pulsebreite Richtung
        if ( EPwm1Regs.CMPB >= EPwm1Regs.TBPRD){
            pwm_richtung_b = 0;
        }
        if (EPwm1Regs.CMPB == 0){
            pwm_richtung_b = 1;
        }

        // PWM1B Pulsbreite aendern
        if (pwm_richtung_b) {
            EPwm1Regs.CMPB++;
        } else {
            EPwm1Regs.CMPB--;
        }
    }
}

/* Initial Funktion fuer PWM A1 (GPIO0) */
void fn_ePWM1_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;           //  GPAMUX1 auf ePWM1A setzen
    GpioCtrlRegs.GPAMUX1.bit.GPIO1= 1;           //  GPAMUX1 auf ePWM1A setzen
    EDIS;

    // // 1kHz
    EPwm1Regs.TBPRD = 45026;           // Periode auf 45026 Takte setzen
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen

    EPwm1Regs.AQCTLA.bit.CAU = 2;       // PWM A set bei steigender Flanke
    EPwm1Regs.AQCTLA.bit.CAD = 1;       // PWM A clear bei fallender Flanke
    EPwm1Regs.AQCTLB.bit.CBU = 2;       // PWM B clear bei steigender Flanke
    EPwm1Regs.AQCTLB.bit.CBD = 1;       // PWM B set bei fallender Flanke

    EPwm1Regs.CMPA.half.CMPA = 0;       // PWM-Vergleichswert A (Pulsbreitenverhältnis) auf 100% setzen
    EPwm1Regs.CMPB = EPwm1Regs.TBPRD;   // PWM-Vergleichswert B (Pulsbreitenverhältnis) auf 0% setzen


}
