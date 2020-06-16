/*
 * Lab12_1C.c
 *
 *  Created on:     06.06.2020
 *  Author:         Dinera
 *  Description:    PWM - UEberstrom-Alarm (Teil 3)
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_1_ISR(void);    //Interrupt Routune Timer1 (Pulsweite)
__interrupt void meine_TZ1_ISR(void);           //Interrupt Routine Trip Zone (S1)
void fn_ePWM1_Init(void);                      // Initial Funktion fuer PWM1 (GPIO0 und GPIO1)

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
    GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;   // GPA LEDs auschalten
    GpioDataRegs.GPBCLEAR.all |= 0x8C0000;  // GPB LEDs auschalten

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT1 = &mein_CPU_Timer_1_ISR; // ISR Timer1 (Pulsweitenanpassung)
    PieVectTable.EPWM1_TZINT = &meine_TZ1_ISR;  // ISR Trip Zone 1 (S1)

    EDIS;                                   // Sperre bei kritischen Registern setzen.

    /* konfigurration Interrupts */
    asm("   CLRC INTM");                    // Freigabe der Interrupts
    IER |= (1<<1) | (1<<12);                               // INT2 und INT13 freigeben

    PieCtrlRegs.PIEIER2.bit.INTx1 = 1;      // Interrupt-Leitung 1 freigeben (TZ1)


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

__interrupt void mein_CPU_Timer_1_ISR(void)
{
    /* Variablen Deklaration                */
    static unsigned int pwm_richtung = 1;           // Zunahme (1) oder Abnahme (0) Richtung des CMPA Registers

    // Bediehnung des Watchdogs (Good Key part 2)
    EALLOW;
    SysCtrlRegs.WDKEY = 0x55;
    EDIS;

    // PWM Tastverhaeltnis um 1 verringern
    if ( EPwm1Regs.CMPA.half.CMPA >= EPwm1Regs.TBPRD){
        pwm_richtung = 0;
    }
    if (EPwm1Regs.CMPA.half.CMPA == 0){
        pwm_richtung = 1;
    }
    if (pwm_richtung) {
        EPwm1Regs.CMPA.half.CMPA++;
    } else {
        EPwm1Regs.CMPA.half.CMPA--;
    }
}

__interrupt void meine_TZ1_ISR(void)
{
    // Bediehnung des Watchdogs (Good Key part 1 & 2)
    EALLOW;
    SysCtrlRegs.WDKEY = 0x55;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;

    // LEDs umschalten
    GpioDataRegs.GPATOGGLE.bit.GPIO17 = 1;   // GPA LEDs umschalten
    GpioDataRegs.GPBTOGGLE.all |= 0x8C0000;  // GPB LEDs umschalten


    // Freigabe Interrupt (Acknowledge Flag)
    EALLOW;
    EPwm1Regs.TZCLR.bit.CBC = 1;            // Cycle by Cycle interrupt flag loeschen
    EPwm1Regs.TZCLR.bit.INT = 1;            // Trip Zone interrupt flag loeschen
    EDIS;

    // Freigabe Interrupt (Acknowledge Flag)
    PieCtrlRegs.PIEACK.all = 2;

}

/* Initial Funktion fuer PWM A1 (GPIO0) */
void fn_ePWM1_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;           //  GPAMUX1 auf ePWM1A setzen
    GpioCtrlRegs.GPAMUX1.bit.GPIO1= 1;            //  GPAMUX1 auf ePWM1A setzen
    GpioCtrlRegs.GPAMUX1.bit.GPIO12= 1;           //  GPIO12 im MUX Register auf Trip Zone 1 Stellen

    // Trip Zone Modul
    EPwm1Regs.TZCTL.bit.DCAEVT1 = 3;    //Digital Compare Unit A1 deaktivieren
    EPwm1Regs.TZCTL.bit.DCAEVT2 = 3;    //Digital Compare Unit A2 deaktivieren
    EPwm1Regs.TZCTL.bit.DCBEVT1 = 3;    //Digital Compare Unit B1 deaktivieren
    EPwm1Regs.TZCTL.bit.DCBEVT2 = 3;    //Digital Compare Unit B2 deaktivieren
    EPwm1Regs.TZCTL.bit.TZA = 2;        //Trip Zone Event fuer PWM1A aktiviern
    EPwm1Regs.TZCTL.bit.TZB = 2;        //Trip Zone Event fuer PWM1B aktiviern
    EPwm1Regs.TZSEL.all = 1;      //TZ1 Eingang (GPIO12) als Cycle by Cycle Trip Zone Event aktiviert, alle anderen Bits loeschen
    EPwm1Regs.TZEINT.bit.CBC = 1;      //Cycle by Cycle Interrupt einschalten

    EDIS;

    // // 1kHz
    EPwm1Regs.TBPRD = 45026;           // Periode auf 45026 Takte setzen
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen

    EPwm1Regs.AQCTLA.bit.CAU = 2;       // PWM A set bei steigender Flanke
    EPwm1Regs.AQCTLA.bit.CAD = 1;       // PWM A clear bei fallender Flanke

    EPwm1Regs.CMPA.half.CMPA = 0;       // PWM-Vergleichswert A (Tastverhaeltnis) auf 100% setzen

    //Dead-Band-Unit
    EPwm1Regs.DBFED = 900;              //Einstellen des Register FED auf 900 ==> 10us/11,11ns=900
    EPwm1Regs.DBRED = 900;              //EInstellen des Register RED auf 900
    EPwm1Regs.DBCTL.bit.IN_MODE = 0;    //EPWM1A ist das Basissignal
    EPwm1Regs.DBCTL.bit.POLSEL = 2;     //EPWM1B wird invertiert
    EPwm1Regs.DBCTL.bit.OUT_MODE = 3;   //beide Verzögerungen werden angewendet


}
