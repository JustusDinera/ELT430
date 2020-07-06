/*
 * Lab19_1.c
 *
 *  Created on:     06.06.2020
 *  Author:         Dinera
 *  Description:    SCI (Teil 1) (UART)
 */

#include "DSP28x_Project.h"

/*** Funktionendeklaration                  */
__interrupt void mein_CPU_Timer_0_ISR(void);

void main(void)
{
    /*** Variablendeklaration               */
    char message[] = "Der F28069-UART funktioniert\n\r"; // Nachricht per UART
    unsigned int i;                         // Zaehlvariable

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

    /** Initialisierung kritische Register  */
    /* Initialisierung Watchdog             */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln
    SysCtrlRegs.WDCR = 0x2E;                // Watchdog einschalten und den Prescaler auf 16 setzen

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung SCI                  */
    // Pinkonfiguration
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;    // SCI-A Input gesetzt
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;// SCI-A Output gesetzt
    // SCI Baugruppe initialisieren
    SciaRegs.SCICTL1.bit.SWRESET = 0;       // SCI Konfiguration zur Bearbeitung freigeben
    SciaRegs.SCICCR.bit.STOPBITS = 0;       // Stopbit-anzahl auf 1 setzten
    SciaRegs.SCICCR.bit.PARITY = 0;         // ungerade Paritaet eingestellt
    SciaRegs.SCICCR.bit.PARITYENA = 1;      // Paritaet eingeschalten
    SciaRegs.SCICCR.bit.SCICHAR = 7;        // Zeichenlaenge auf 8 Bit gestellen
    SciaRegs.SCICCR.bit.LOOPBKENA = 0;      // Lopback deaktivieren
    SciaRegs.SCICCR.bit.ADDRIDLE_MODE = 0;  // Idle Line Mode aktivieren
    SciaRegs.SCICTL1.bit.TXENA = 1;         // SCI sendende Leitung freigeben
    SciaRegs.SCICTL1.bit.RXENA = 1;         // SCI empfangende Leitung freigeben
    SciaRegs.SCICTL1.bit.RXERRINTENA = 0;   // RX-Error Interrupt sperren
    SciaRegs.SCICTL1.bit.SLEEP = 0;         // Sleep-Mode sperren
    SciaRegs.SCICTL1.bit.TXWAKE = 0;        // TX-Wake-Mode abschalten
    SciaRegs.SCIHBAUD = 1;                  // obere 8 Bit des Wertes 293 (Baudrate 9600)
    SciaRegs.SCILBAUD = 0x25;               // untere 8 Bit des Wertes 293 (Baudrate 9600)
    SciaRegs.SCICTL1.bit.SWRESET = 1;       // SCI Konfiguration zur Bearbeitung schliessen


    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen
    EDIS;                                   // Sperre bei kritischen Registern setzen.

    IER |= 1;                               // INT1 freigeben
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt-Leitung 7 freigeben
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen

    /*   Low Power Mode einstellen              */
    SysCtrlRegs.LPMCR0.bit.LPM = 0; // IDLE Mode aktivieren

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

        if (CpuTimer0.InterruptCount > 9) {
            // Interrupt Counter auf 0 zuruecksetzen
            CpuTimer0.InterruptCount = 0;

            // Nachricht ueber UART (SCI-Modul) senden
            for (i = 0; message[i] != '\0'; ++i) {
                SciaRegs.SCITXBUF = message[i];         // Zeichen in den Ausgangs Buffer schreiben
                while(SciaRegs.SCICTL2.bit.TXEMPTY == 0);        // Warten bis der Sende Buffer leer ist
            }
        }

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

    // Interruptcounter um 1 erhoehen
    CpuTimer0.InterruptCount++;

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

