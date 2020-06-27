/*
 * Lab16_1.c
 *
 *  Created on:     25.06.2020
 *  Author:         Dinera
 *  Description:    PWM Erzeugung und ADC Messung
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void my_ADCINT1_ISR(void);      //ADC Interrupt Routine
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0

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

    /* Initialisierung CPU-Timer0 (10Hz) bei 90MHz CPU-Takt auf 100ms Intervall  */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 100000);
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
    IER |= 1;                                       // INT1 freigeben
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;              // Interrupt 1 Leitung 1 freigeben (ADCINT1)
    PieVectTable.ADCINT1 = &my_ADCINT1_ISR;         // Eigene ADC-ISR in Vektortabelle eintragen

    /* Initialisierung Watchdog             */
    SysCtrlRegs.WDCR = 0x28;                // WD einschalten (PS = 1)

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen (Lauflicht)

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

/* Interrupt Timer0 (Lauflicht)
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


/* ADC Interrupt Routine */
__interrupt void my_ADCINT1_ISR(void){
    // Variablen deklaration

    // Bediehnung des Watchdogs (Good Key part 1)
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;


    }

    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
