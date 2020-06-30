/*
 * Lab18_1.c
 *
 *  Created on:     29.06.2020
 *  Author:         Dinera
 *  Description:    Sinus-Spannungen an den Ausgaengen OUTA und OUTB und Messung dieser an den Eingaengen ADCINA0/ADCINA1 rueckkoppeln
 */

#include "DSP28x_Project.h"
#include "math.h"

/*** Makro-Definition                       */
#define KOEFFIZIENT (double)((double)3.14159265/180) // Koeffizient fuer die Berechnung des Sinus im Gradmass. Multiplikation in der Funktion beschleunigt die Abarbeitung.

/*** Globale Variablen Deklaration          */
unsigned int OutA[360],                     // Variable fuer Messung von Ausgang OutA
             OutB[360];                     // Variable fuer Messung von Ausgang OutB

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);    // Timoer0 Interrupt
__interrupt void my_ADCINT1_ISR(void);          // ADC Interrupt
int spannung(float winkel_deg, float phasenverschiebung);   // Berechnung Sinus-Spannung: Winkel im Bogenmass und die Phasenverschiebung im Gradmass

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

    /* Initialisierung CPU-Timer0 (90MHz) auf 1ms */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 1000);
    CpuTimer0Regs.TCR.bit.TSS = 0;

    /* Initialisieren des ADC */
    InitAdc();

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x2D;                // Watchdog einschalten und den Prescaler auf 16 setzen

    /* Initialisierung der GPIO Pins        */
    // Schalter S1
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    // Lauflicht
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])
    // SPI
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;    // 1 = SPISIMOA
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;    // 1 = SPICLKA
    GpioDataRegs.GPASET.bit.GPIO22 = 1;     // GPIO22 ('SPI-CS) passiv geschaltet
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;     // GPIO22 ('SPI-CS) auf Ausgang stellen

    // INT1 freigeben
    IER |= 1;

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben

    // Initialisierung ADC
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 0;     // SOC0/1 auf "Single Sample Mode" stellen

    // ADCINA0 (OUTA)
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 1;        // Trigger fuer SOC0 auf CPU Timer0 stellen
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;           // ADCINA0 als analogen Eingang waehlen
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;           // Abtastfenster auf 7 Takte stellen

    //ADCINA1 (OUTB)
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 1;         // Quelle fuer SOC1 auf CPU Timer0 stellen
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 1;           // ADCINA1 als analogen Eingang waehlen
    AdcRegs.ADCSOC1CTL.bit.ACQPS = 6;           // Abtastfenster auf 7 Takte stellen

    // Einstellen von ADC Interrupt
    AdcRegs.INTSEL1N2.bit.INT1CONT = 1;         // ausloesen bei jedem EOC
    AdcRegs.INTSEL1N2.bit.INT1E = 1;            // freigeben von ADCINT1
    AdcRegs.INTSEL1N2.bit.INT1SEL = 1;          // ADCINT1 wir bei EOC1 ausgefuehrt
    PieVectTable.ADCINT1 = &my_ADCINT1_ISR;
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;      // Interrupt-Leitung 1 freigeben

    EDIS;                                       // Sperre bei kritischen Registern setzen.
    /* SPI inistialisieren */
    SpiaRegs.SPICCR.bit.SPISWRESET = 0;     // Initialer Reset der SPI Schnittstelle A
    SpiaRegs.SPICCR.all =
            (1<< 6) |                       // Taktpolatitaet auf fallende Flanke stellen
            15;                              // 2 Character (16 Bit) lange Nachrichten
    SpiaRegs.SPICTL.all =
            (1<<3)|                         // Taktphase um einem halben Takt verschieben
            (1<<2)|                         // auf SPI Master-Mode stellen
            (1<<1);                              // Master/Slave Transmit-Mode eingeschaltet
    SpiaRegs.SPIBRR = 44;                    // Baudrate auf 1MBit setzen
    SpiaRegs.SPICCR.bit.SPISWRESET = 1;     // Initialer Reset der SPI Schnittstelle A

    /*   Low Power Mode einstellen              */
    SysCtrlRegs.LPMCR0.bit.LPM = 0; // IDLE Mode aktivieren

    // Interrupts freigeben
    asm("   CLRC INTM");                    // Freigabe der Interrupts

    /*** Main loop                          */
    while(1)
    {
        // Bediehnung des Watchdogs (Good Key part 1)
        EALLOW;
        SysCtrlRegs.WDKEY = 0xAA;
        EDIS;

        // IDLE Mode aktivieren
        asm("   IDLE");
    }
}

__interrupt void mein_CPU_Timer_0_ISR(void)
{
    /* Variablen Deklaration                        */
    static unsigned int position = 1;               // LED Position
    static unsigned int richtung;                   // Richtung des Lauflichts
    static unsigned int schalter;                   // Variable fuer Taster
    static unsigned int LED_durchlauf_zaehler = 0;  // Zaehlvariable fuer LED Lauflicht
    static float Winkel = 0;                     //
    unsigned int msg;                               // Variable deklarieren fuer Nachricht 1 und 2, sowie RX Buffer leeren


    /* LED Lauflicht und WD                         */
    if (LED_durchlauf_zaehler < 200) {
        LED_durchlauf_zaehler++;            // pro Interrupt um 1 erhoehen
    } else {
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

        LED_durchlauf_zaehler = 0;       // Zaehlvariable zuruecksetzen
    }


    /* SPI Nachricht an DAC                         */
    //Berechnung der Spannungen


    // Nachricht 1
    msg =
            ((spannung(Winkel, 0) & 0x3FF)<<2)|       // Sinus-Spannung ohne Phasenverschiebung in die Datenbits 11 - 2
            (1<<14)|                        // Speed controll auf "fast mode" stellen
            (1<<12);                        // Nachricht in den Buffer schieben
    GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;   // aktivieren von /CS
    SpiaRegs.SPITXBUF = msg;                // Die 1. Nachricht in den Ausgangs buffer schieben
    while(~(SpiaRegs.SPISTS.bit.INT_FLAG)&1);   // Warten bis das Bit INT_FLAG auf 1 kippt
    DELAY_US(1);                            // 1us warten
    GpioDataRegs.GPASET.bit.GPIO22 = 1;     // deaktivieren von /CS
    msg = SpiaRegs.SPIRXBUF;                // SPI Vorgang abschliessen (Den Eingangsbuffer durch lesen freigeben)

    // Nachricht 2
    msg =
            ((spannung(Winkel, 180) & 0x3FF)<<2)|      // Sinus-Spannung mit einer Phasenverschiebung von 180 Grad in die Datenbits 11 - 2
            (1<<14)|                        // Speed controll auf "fast mode" stellen
            (1<<15);                         // Die Nachricht in den Ausgang OUTA und den TX Buffer in den Ausgang B Schieben
    GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;   // aktivieren von /CS
    SpiaRegs.SPITXBUF = msg;                // Die 1. Nachricht in den Ausgangs buffer schieben
    while(~SpiaRegs.SPISTS.bit.INT_FLAG &1);   // Warten bis das Bit INT_FLAG auf 1 kippt
    DELAY_US(1);                            // 1us warten
    GpioDataRegs.GPASET.bit.GPIO22 = 1;     // deaktivieren von /CS
    msg = SpiaRegs.SPIRXBUF;                // SPI Vorgang abschliessen (Den Eingangsbuffer durch lesen freigeben)

    // Anpassung des Winkels
    Winkel++;                           // Winkel inkrementieren
    if (Winkel > 359) {
        Winkel = 0;                     // Winkel auf Ausgangswert zureucksetzen (0 Grad)
    }

    // Freigabe Interrupt (Acknowledge Flag)
    PieCtrlRegs.PIEACK.all = 1;
}

/* ADC Interrupt Routine */
__interrupt void my_ADCINT1_ISR(void){
    // Variablen deklaration
    static unsigned int * OutAPtr = OutA;              // Pointer fuer die Messwerte von OUTA
    static unsigned int * OutBPtr = OutB;              // Pointer fuer die Messwerte von OUTB

    // ADC Messung
    *OutAPtr = AdcResult.ADCRESULT0;            // Auslesen  der AdcResult Register 0 (OUTA)
    *OutBPtr = AdcResult.ADCRESULT1;            // Auslesen  der AdcResult Register 1 (OUTB)
    OutAPtr++;                                  // Erhoehen um in naechsten Wert in Feld OutA zugreifen zu koennen.
    OutBPtr++;                                  // Erhoehen um in naechsten Wert in Feld OutB zugreifen zu koennen.
    if (OutAPtr > &OutA[359]){                 // Pointer auf die ersten Eintraege der Felder zuruecksetzen
        OutAPtr = OutA;
        OutBPtr = OutB;
    }


    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

// Sinus-Spannungsberechnung
int spannung(float winkel_deg, float phasenverschiebung){   // Winkel im Bogenmass und die Phasenverschiebung im Gradmass
    double winkel_rad = ((winkel_deg + phasenverschiebung) * KOEFFIZIENT); // Berechnung des Winkels mit Phasenverschiebung im Bogenmass
    return (int)(sin(winkel_rad) * 511 + 512);      // Berechnung: Sinus * Amplitude (1024/2-1) + Offset (1024/2)
}
