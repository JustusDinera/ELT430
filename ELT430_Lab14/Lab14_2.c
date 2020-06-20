/*
 * Lab14_2.c
 *
 *  Created on:     17.06.2020
 *  Author:         Dinera
 *  Description:    Potentiometer als Leuchtbalken-Geschwindigkeits-Regler
 */

#include "DSP28x_Project.h"

/*** Makro Definition                       */

#define SPEED_STEP 101


/** Globale Variablendeklaration            */
unsigned int
    Flag_50ms=0,                            // Zaehlvariable der 50ms Zyklen
    LED_Intervall,                          // Geschwindigkeitsstufe
    schalter,                               // Schalter S1
    richtung,                               // Richtung des Lauflichts
    position = 1;                           // LED Position

/*** Funktionendeklaration                  */
__interrupt void my_ADCINT1_ISR(void);      //ADC Interrupt Routine
__interrupt void mein_CPU_Timer_0_ISR(void); // Timer0 Interrupt (Flag_50ms hochzaehlen)

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

    /* Initialisierung CPU-Timer0 (20Hz) bei 90MHz CPU-Takt auf 50ms Intervall  */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 50000);
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
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR;     // Eigene Timer0 ISR in ISR Vektor Tabelle eintragen

    /* Initialisierung Watchdog             */
    SysCtrlRegs.WDCR = (5 << 3)|            // WD Check Bit Muster
                       (3);                 // WD Prescaler auf DIV 4 einstellen

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben

    EDIS;                                   // Sperre bei kritischen Registern setzen.

    /* Initialisierung Interrupts           */
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



        if (Flag_50ms >= LED_Intervall) {               // wenn genug 50ms Schleifen durchlaudefen sind, dann wird eine Aenderung an den LEDs vorgenommen

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
            Flag_50ms = 0; // Flag_50ms zuruecksetzen
        }

        // IDLE Mode aktivieren
        asm("   IDLE");
    }
}

__interrupt void mein_CPU_Timer_0_ISR(void)
{
    /* Variablen Deklaration                */
    //static unsigned int position = 1;               // LED Position
    //static unsigned int richtung;                  // Richtung des Lauflichts
    //static unsigned int schalter;                   // Variable fuer Taster

    // Bediehnung des Watchdogs (Good Key part 2)
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;

    Flag_50ms++;


    // Freigabe Interrupt (Acknowledge Flag)
    PieCtrlRegs.PIEACK.all = 1;
}

/* ADC Interrupt Routine */
__interrupt void my_ADCINT1_ISR(void){
    // Variablen deklaration
    unsigned int poti_wert,             // Variable fuer den Potentiometerwert aus dem Register ADCRESULT0
                 i=0;                   // Zaehlvariable der Schleife

    LED_Intervall = 1;
    // Auslesn des ADC Ergebnisregisters (Potentiometer)
    poti_wert = AdcResult.ADCRESULT0;

    // Berechnen der Gescheindigkeit
    // Geschwindigkeitsstufe zuruecksetzen
    do {
        if (poti_wert > (LED_Intervall * SPEED_STEP)) {
            LED_Intervall++;            // Erhoehen der Geschwindigkeit
        } else {
            i = 1;                      // Wenn der ausgeledene Wert kleines als die Geschwindigkeitsstufe * Schwellwerte ist, dann wird die Schleife verlassen
        }
    } while(i==0);

    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
