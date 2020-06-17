/*
 * Lab14_1.c
 *
 *  Created on:     16.06.2020
 *  Author:         Dinera
 *  Description:    Potentiometer als Leuchtbalken-Regler
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void my_ADCINT1_ISR(void);      //ADC Interrupt Routine

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

    /* Initialisierung CPU-Timer0 (10kHz) bei 90MHz CPU-Takt auf 100us Intervall  */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 100);
    CpuTimer0Regs.TCR.bit.TSS = 0;

    /* Initialisieren des ADC */
    InitAdc();

    /** Initialisierung kritische Register  */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln

    /* ADC einstellen */
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 0;         // Single Sampe Mode an SOC0 und SOC1
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 1;             // CPU Timer0 als SOC0-Trigger eingestellt
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 7;               // ADCINA7 als Analogeingang setzen
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;               // Abtastfenster auf 7 Takte gestellt
    AdcRegs.INTSEL1N2.bit.INT1CONT = 1;             // Interrupt bei jedem EOC Signal
    AdcRegs.INTSEL1N2.bit.INT1E = 1;                // Interrupt ADCINT1 freigeben
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0;              // EOC0 triggert ADCINT1
    IER |= (1 << 9);                                // INT10 freigeben
    PieCtrlRegs.PIEIER10.bit.INTx1 = 1;             // Interrupt 10 Leitung 1 freigeben (ADCINT1)
    PieVectTable.ADCINT1 = &my_ADCINT1_ISR;         // Eigene ADC-ISR in Vektortabelle eintragen

    /* Initialisierung Watchdog             */
    SysCtrlRegs.WDCR = 0x28;                // WD einschalten (PS = 32)

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung Interrupts           */
    EDIS;                                   // Sperre bei kritischen Registern setzen.
    asm("   CLRC INTM");                    // Freigabe der Interrupts

    /*   Low Power Mode einstellen          */
    SysCtrlRegs.LPMCR0.bit.LPM = 0; // IDLE Mode aktivieren

    /*** Main loop                          */
    while(1)
    {
        // IDLE Mode aktivieren
        asm("   IDLE");

        // Bediehnung des Watchdogs (Good Key part 1)
        EALLOW;
        SysCtrlRegs.WDKEY = 0x55;
        EDIS;
    }
}

/* ADC Interrupt Routine */
__interrupt void my_ADCINT1_ISR(void){
    // Variablen deklaration
    static unsigned int poti_wert;  // Variable fuer den Potentiometerwert aus dem Register ADCRESULT0

    // Bediehnung des Watchdogs (Good Key part 1)
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;

    /* Anzeige des Potoeniometerwerts */
    // Auslesn des ADC Ergebnisregisters (Potentiometer)
    poti_wert = AdcResult.ADCRESULT0;

    // LEDs auschalten
    GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;
    GpioDataRegs.GPBCLEAR.all |= 0x8C0000;

    // Auswerten des ADC Ergebnisregisters (Potentiometer)
    if (poti_wert > 0) {
        GpioDataRegs.GPBSET.bit.GPIO55 = 1; // P4
        if (poti_wert < 512) {
            GpioDataRegs.GPBSET.bit.GPIO51 = 1; // P3
            if (poti_wert < 1024) {
                GpioDataRegs.GPBSET.bit.GPIO50 = 1; // P2
                if (poti_wert > 1536) {
                    GpioDataRegs.GPASET.bit.GPIO17 = 1; // P1
                }
            }
        }
    }

    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
