/*
 * Lab12_2.c
 *
 *  Created on:     06.06.2020
 *  Author:         Dinera
 *  Description:    PWM - Modulierung Sinus
 */

#include "DSP28x_Project.h"

/* Variablendeklaration                 */
float * sinus = (float*)0x3FD590;               //FPU-ROM

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0
__interrupt void EPwm1A_compare_ISR(void);          //Interrupt Routune EPwm1 (Pulsweite)
void fn_ePWM1A_Init(void);                      // Initial Funktion fuer PWM A1 (GPIO0)

void main(void)
{
    /*** Variablen Deklaration               */

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

    /* Initialisierung ePWM1A               */
    fn_ePWM1A_Init();

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
    PieVectTable.EPWM1_INT = &EPwm1A_compare_ISR;


    EDIS;                                   // Sperre bei kritischen Registern setzen.

    /* konfigurration Interrupts */
    asm("   CLRC INTM");                    // Freigabe der Interrupts
    IER |= 1 | (1<<2) ;                               // INT1 und INT3 freigeben

    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben (Timer0)
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;      // Interrupt-Leitung 1 freigeben (EPwm1Int)


    /*   Low Power Mode einstellen              */
    SysCtrlRegs.LPMCR0.bit.LPM = 0;         // IDLE Mode aktivieren

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

// PWM Interrupt Routine (Sinus)
__interrupt void EPwm1A_compare_ISR(void)
{
    //Variablen
    static unsigned int step=0;


    EPwm1Regs.CMPA.half.CMPA = (int)((float)EPwm1Regs.TBPRD-((*sinus+1)*(float)EPwm1Regs.TBPRD*0.5)); //Umrechnung Kennfeldwert auf Wert zwischen 0-1 und in Compare Register A schreiben

    //step um 1 erhöhen und sine_table auch
    step++;
    sinus++;



    //Inkrementieren den Wert des Zeigers "sine_table"
    if(512==step)
    {
        step = 0;
        sinus = (float *)0x3FD590;     //Anfangswert der Tabelle
    }

    /* Interruptquelle freigeben */
    EPwm1Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = 4;
}

/* Initial Funktion fuer PWM A1 (GPIO0) */
void fn_ePWM1A_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;           //  GPAMUX1 auf ePWM1A setzen
    EDIS;

    EPwm1Regs.TBPRD = 179;              // Periode auf 179 Takte setzen (1kHz)
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm1Regs.AQCTLA.bit.CAU = 2;       // PWM set bei steigender Flanke
    EPwm1Regs.AQCTLA.bit.PRD = 1;       // PWM clear Bei Periodenende Output auf low setzen
    EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD/2-1;       // PWM Tastverhaeltnis auf 50% setzen
    EPwm1Regs.ETPS.bit.INTPRD = 1;      // ausfuehren, wenn erster Trigger ausgefuehrt wurde
    EPwm1Regs.ETSEL.bit.INTEN = 1;      // EPwm1 Interrrupt aktivieren
    EPwm1Regs.ETSEL.bit.INTSEL = 2;     // Interrupt ausloesen, wenn TBPRD == CMPA

}

