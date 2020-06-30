/*
 * Lab17_2.c
 *
 *  Created on:     28.06.2020
 *  Author:         Dinera
 *  Description:    Saegezahnspannungen an den Ausgaengen OUTA und OUTB zurueck gefuehrt auf ADCINA0/ADCINA0
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);

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

    /* Initialisierung CPU-Timer0 (90MHz) auf 1ms */
    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0, 90, 1000);
    CpuTimer0Regs.TCR.bit.TSS = 0;

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
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;     // GPIO22 ('SPI-CS) auf Ausgang stellen
    GpioDataRegs.GPASET.bit.GPIO22 = 1;     // GPIO22 ('SPI-CS) passiv geschaltet

    // INT1 freigeben
    IER |= 1;

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen
    EDIS;                                   // Sperre bei kritischen Registern setzen.
    asm("   CLRC INTM");                    // Freigabe der Interrupts
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben

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
    static int Spannung_A = 1023;                   //
    static int Spannung_B = 0;                      //
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
    // Nachricht 1
    msg =
            ((Spannung_B & 0x3FF)<<2)|       // Spannung B in die Datenbits 11 - 2
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
            ((Spannung_A & 0x3FF)<<2)|      // Spannung A in die Datenbits 11 - 2
            (1<<14)|                        // Speed controll auf "fast mode" stellen
            (1<<15);                         // Die Nachricht in den Ausgang OUTA und den TX Buffer in den Ausgang B Schieben
    GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;   // aktivieren von /CS
    SpiaRegs.SPITXBUF = msg;                // Die 1. Nachricht in den Ausgangs buffer schieben
    while(~SpiaRegs.SPISTS.bit.INT_FLAG &1);   // Warten bis das Bit INT_FLAG auf 1 kippt
    DELAY_US(1);                            // 1us warten
    GpioDataRegs.GPASET.bit.GPIO22 = 1;     // deaktivieren von /CS
    msg = SpiaRegs.SPIRXBUF;                // SPI Vorgang abschliessen (Den Eingangsbuffer durch lesen freigeben)

    // Anpassung der Spannungen
    if (Spannung_A > 0) {
        Spannung_A--;                       // Spannung A dekrementieren
        Spannung_B++;                       // Spannung B inkrementieren
    } else {
        Spannung_A = 1023;                  // Spannung A auf Ausgangswert zureucksetzen
        Spannung_B = 0;                     // Spannung B auf Ausgangswert zureucksetzen
    }

    // Freigabe Interrupt (Acknowledge Flag)
    PieCtrlRegs.PIEACK.all = 1;
}

