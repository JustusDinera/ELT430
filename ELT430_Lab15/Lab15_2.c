/*
 * Lab15_2.c
 *
 *  Created on:     25.06.2020
 *  Author:         Dinera
 *  Description:    Messung der Chip-Temperatur bei 10 MHz
 */

#include "DSP28x_Project.h"
#include "IQmathLib.h"

/*** Makrodefinition                        */
#define getTempSlope() (*(int (*)(void))0x3D7E82)()
#define getTempOffset() (*(int (*)(void))0x3D7E85)()
#define MAX_MESSWERTE 1600                   // Makro um die maximale Anzahl der Messwerte zu bestimmen

/** Benutzer Makros                         */
#define IDLE_AN                             // Idle Mode aktivieren
//#define CPU_5MHZ                            // CPU CLOCK auf 5MHz stellen
//#define CPU_10MHZ                           // CPU CLOCK auf 10MHz stellen
//#define CPU_20MHZ                           // CPU CLOCK auf 20MHz stellen
//#define CPU_40MHZ                           // CPU CLOCK auf 40MHz stellen
#define CPU_60MHZ                           // CPU CLOCK auf 60MHz stellen

/*** Variablendeklaration                   */
unsigned int Temperatur,                    // Globale Variable fuer die Anzeige der Temperatur
             offset,                        // Temperatur Offset
             slope,                         // Anstieg der Geraden fuer die Temperatur

             index_verlauf= 0;              // Zaehlvariable fuer Darstellung Temp in Zeit
float verlauf_temp[MAX_MESSWERTE],          // Variable um den Temperaturverlauf darzustellen
      CPU_Temperatur;                       // Momentanwert der CPU Temperatur

/*** Funktionendeklaration                  */
__interrupt void my_ADCINT1_ISR(void);      //ADC Interrupt Routine

void main(void)
{
    /*** Initialisierung                    */
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (Uint32)&RamfuncsLoadSize); // kopieren des Programms in den RAM zur Laufzeit und schnellere Zufriffszeiten zu nutzen.

    /* Variablendefinition                  */
    offset = getTempOffset();               // Offset-Wert von Adresse 0x3D7E85 in Variable speichern
    slope = getTempSlope();                 // Slope-Wert von Adresse 0x3D7E82 in Variable speichern

    /* Initialisierung Clock (90MHz)        */
    InitSysCtrl();

    /* Einstellen der CPU Clock auf 5 MHz */
#ifdef CPU_5MHZ
    InitPll(1, 1);
#endif
    /* Einstellen der CPU Clock auf 10 MHz */
#ifdef CPU_10MHZ
    InitPll(1, 2);
#endif
    /* Einstellen der CPU Clock auf 20 MHz */
#ifdef CPU_20MHZ
    InitPll(1, 3);
#endif
    /* Einstellen der CPU Clock auf 40 MHz */
#ifdef CPU_40MHZ
    InitPll(2, 3);
#endif
    /* Einstellen der CPU Clock auf 60 MHz */
#ifdef CPU_60MHZ
    InitPll(3, 3);
#endif

    /* Performence optimierung fuer FLASH */
    InitFlash();

    /* Ininitalisierung Interrupts          */
    InitPieCtrl();
    InitPieVectTable();

    /* Initialisierung der CPU Timer        */
    InitCpuTimers();

    /* Initialisierung CPU-Timer0 bei 5MHz CPU-Takt auf 100ms Intervall  */
#ifdef CPU_5MHZ
    ConfigCpuTimer(&CpuTimer0, 5, 100000);
#endif
    /* Initialisierung CPU-Timer0 bei 10MHz CPU-Takt auf 100ms Intervall  */
#ifdef CPU_10MHZ
    ConfigCpuTimer(&CpuTimer0, 10, 100000);
#endif
    /* Initialisierung CPU-Timer0 bei 20MHz CPU-Takt auf 100ms Intervall  */
#ifdef CPU_20MHZ
    ConfigCpuTimer(&CpuTimer0, 20, 100000);
#endif
    /* Initialisierung CPU-Timer0 bei 40MHz CPU-Takt auf 100ms Intervall  */
#ifdef CPU_40MHZ
    ConfigCpuTimer(&CpuTimer0, 40, 100000);
#endif
    /* Initialisierung CPU-Timer0 bei 60MHz CPU-Takt auf 100ms Intervall  */
#ifdef CPU_60MHZ
    ConfigCpuTimer(&CpuTimer0, 60, 100000);
#endif


    CpuTimer0Regs.TCR.bit.TSS = 0;          // Starten CPU Timer0

    /* Initialisieren des ADC */
    InitAdc();

    /** Initialisierung kritische Register  */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln

    /* ADC einstellen */
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 0;         // Single Sampe Mode an SOC0 und SOC1
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 1;             // CPU Timer0 als SOC0-Trigger eingestellt
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 5;               // ADCINA5 als Analogeingang setzen (Temperatursensor)
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;               // Abtastfenster auf 7 Takte gestellt
    AdcRegs.ADCCTL1.bit.TEMPCONV = 1;               // Temperatursensor einschalten

    AdcRegs.INTSEL1N2.bit.INT1CONT = 1;             // Interrupt bei jedem EOC Signal
    AdcRegs.INTSEL1N2.bit.INT1E = 1;                // Interrupt ADCINT1 freigeben
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0;              // EOC0 triggert ADCINT1
    IER |= 1;                                       // INT1 freigeben
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;              // Interrupt 1 Leitung 1 freigeben (ADCINT1)
    PieVectTable.ADCINT1 = &my_ADCINT1_ISR;         // Eigene ADC-ISR in Vektortabelle eintragen

    /* Initialisierung Watchdog             */
    SysCtrlRegs.WDCR = 0x2E;                // WD einschalten (PS = 1)

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 3;    // Clock Signal auf GPIO18 legen

    /* Initialisierung Interrupts           */
    EDIS;                                   // Sperre bei kritischen Registern setzen.
    asm("   CLRC INTM");                    // Freigabe der Interrupts

    /*   Low Power Mode einstellen          */
    SysCtrlRegs.LPMCR0.bit.LPM = 0; // IDLE Mode aktivieren

    /*** Main loop                          */
    while(1)
    {
        // Bediehnung des Watchdogs (Good Key part 1)
        EALLOW;
        SysCtrlRegs.WDKEY = 0x55;
        EDIS;

        // IDLE Mode aktivieren wenn IDLE_An definiert wurde
#ifdef IDLE_AN
        asm("   IDLE");
#endif

    }
}

/* ADC Interrupt Routine */
__interrupt void my_ADCINT1_ISR(void){
    /* Variablen Deklaration                */
    static unsigned int position = 1;               // LED Position
    static unsigned int richtung;                  // Richtung des Lauflichts
    unsigned int schalter;                         // Variable fuer den Taster

    // Bediehnung des Watchdogs (Good Key part 1)
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;

    // Auslesn des ADC Ergebnisregisters (Temperatur)
    Temperatur = AdcResult.ADCRESULT0;
    CPU_Temperatur =  _IQ15toF(slope)*(Temperatur - offset);
    // Zur Darstellung Temp ueber Zeit
    if (index_verlauf < MAX_MESSWERTE) {
        index_verlauf++;
        verlauf_temp[index_verlauf] = CPU_Temperatur;
    }


    // Schalter S1 auslesen
    schalter = GpioDataRegs.GPADAT.bit.GPIO12;

    // Pruefung, ob der Schalter S1 geschlossen ist
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

    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
