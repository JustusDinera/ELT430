/*
 * Lab16_1.c
 *
 *  Created on:     25.06.2020
 *  Author:         Justus Weinhold
 *  Description:    PWM Erzeugung und ADC Messung
 */

#include "DSP28x_Project.h"
#include "IQmathLib.h"

#define getTempSlope() (*(int (*)(void))0x3D7E82)()
#define getTempOffset() (*(int (*)(void))0x3D7E85)()
#define MAX_MESSWERTE 1600                   // Makro um die maximale Anzahl der Messwerte zu bestimmen

/*** Globale Variablendaklaration           */
float CPU_Temperatur,                       // Variable fuer die aktuelle Temperatur
      offset,                               // Temperatur Offset
      slope,                                // Anstieg der Geraden fuer die Temperatur
      verlauf_temp[MAX_MESSWERTE];          // Variable um den Temperaturverlauf darzustellen
unsigned int AdcBuf[300];                   // Abtast Buffer für den ADC1 Eingang
/*** Funktionendeklaration                  */
__interrupt void my_ADCINT1_ISR(void);          //ADC Interrupt 1 Routine
__interrupt void my_ADCINT2_ISR(void);          //ADC Interrupt 2 Routine
__interrupt void mein_CPU_Timer_0_ISR(void);    //Interrupt Routune Timer0
void fn_ePWM1A_Init(void);                      // PWM initialisieren

void main(void)
{
    /*** Initialisierung                    */
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (Uint32)&RamfuncsLoadSize); // kopieren des Programms in den RAM zur Laufzeit und schnellere Zufriffszeiten zu nutzen.

    /* Initialisierung Clock (90MHz)        */
    InitSysCtrl();

    /* Performence optimierung fuer FLASH   */
    InitFlash();

    /* Ininitalisierung Interrupts          */
    InitPieCtrl();
    InitPieVectTable();

    /* Inuitialisierung CPU Timer           */
    InitCpuTimers();

    ConfigCpuTimer(&CpuTimer0, 90, 100000); // Initialisierung CPU-Timer0 (10Hz) bei 90MHz CPU-Takt auf 100ms Intervall
    CpuTimer0Regs.TCR.bit.TSS = 0;          // Timer0 starten

    ConfigCpuTimer(&CpuTimer1, 90, 10);    // Initialisierung CPU-Timer1 (100kHz) bei 90MHz CPU-Takt auf 10us Intervall
    CpuTimer1Regs.TCR.bit.TSS = 0;          // Timer1 starten

    /* PWM initialisieren */
    fn_ePWM1A_Init();

    /* Initialisieren des ADC */
    InitAdc();

    /** Initialisierung kritische Register  */
    EALLOW;                                 // Sperre von kritischen Registern entriegeln

    /* ADC SOC0/SOC1                                */
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 0;         // Single Sampe Mode an SOC0 und SOC1

    /* ADC SOC0 einstellen (Temperaturmessung)      */
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 1;             // CPU Timer0 als SOC0-Trigger eingestellt
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 7;               // ADCINA7 als Analogeingang setzen
    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;               // Abtastfenster auf 7 Takte gestellt

    /* ADC SOC1 einstellen (PWM-Messung)            */
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 2;             // CPU Timer1 als SOC1-Trigger eingestellt
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 9;               // ADCINB1 als Analogeingang setzen
    AdcRegs.ADCSOC1CTL.bit.ACQPS = 6;               // Abtastfenster auf 7 Takte gestellt
    AdcRegs.INTSEL1N2.bit.INT1CONT = 1;             // Interrupt bei jedem EOC Signal
    AdcRegs.INTSEL1N2.bit.INT1E = 1;                // Interrupt ADCINT1 freigeben
    AdcRegs.INTSEL1N2.bit.INT1SEL = 1;              // EOC0 triggert ADCINT1
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;              // Interrupt 1 Leitung 1 freigeben (ADCINT1)
    PieVectTable.ADCINT1 = &my_ADCINT1_ISR;         // Eigene ADC-ISR in Vektortabelle eintragen


    /* Initialisierung Watchdog             */
    SysCtrlRegs.WDCR = 0x28;                // WD einschalten

    /* Initialisierung der GPIO Pins        */
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;     // Eingang setzen (GPIO 12 [S1])
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1;     // Ausgang setzen (GPIO 17 [P1])
    GpioCtrlRegs.GPBDIR.all |= 0x8C0000;    // Ausgang setzen (GPIO 50 [P2], 51 [P3], 55 [P4])

    /* Initialisierung CPU-Interrupts fuer Timer0  */
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;      // Interrupt 1 Leitung 7 freigeben (Timer0)
    PieVectTable.TINT0 = &mein_CPU_Timer_0_ISR; // Eigene ISR aufrufen (Lauflicht)

    /* Initialisierung Interrupts           */
    IER |= 1;                                // INT1 freigeben
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

/* Interrupt Timer0 (Lauflicht)             */
__interrupt void mein_CPU_Timer_0_ISR(void)
{
    /* Variablen Deklaration                */
    static unsigned int position = 1;               // LED Position
    static unsigned int richtung;                  // Richtung des Lauflichts
    static unsigned int schalter;                   // Variable fuer Taster

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
    static unsigned int index_verlauf= 0;              // Zaehlvariable fuer Darstellung Temp in Zeit
    static unsigned int *AdcBufPtr = AdcBuf;                     // Pointer fuer die ADC Messwerte

    // Bediehnung des Watchdogs (Good Key part 1)
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;

    // ADC Messung
    *AdcBufPtr = AdcResult.ADCRESULT1;
    AdcBufPtr++;
    if (&AdcBuf[299] < AdcBufPtr ){
        AdcBufPtr = AdcBuf;
    }

    // Auslesen des ADC Ergebnisregister 1 (Temperatur)
    CPU_Temperatur =  _IQ15toF(slope)*(AdcResult.ADCRESULT0 - offset);
    // Zur Darstellung Temp ueber Zeit
    if (index_verlauf < MAX_MESSWERTE) {
        index_verlauf++;
        verlauf_temp[index_verlauf] = CPU_Temperatur;
    } else {
        index_verlauf = 0;
    }

    // Interruptflag loeschen
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

// ePWM1A initialisieren
void fn_ePWM1A_Init(void){
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;           //  GPAMUX1 auf ePWM1A setzen
    EDIS;

    // 1kHz
    EPwm1Regs.TBPRD = 45026;           // Periode auf 45000 Takte setzen
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     // Clockdivider auf den Wert 1 setzen

    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  // High Speed Time-base Clock Prescale Bits auf Wert 1 setzen
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    // Zaehlmodus auf Up-down-count mode setzen
    EPwm1Regs.AQCTLA.bit.CAD = 1;      // fallende Flanke bei count down
    EPwm1Regs.AQCTLA.bit.CAU = 2;       // steigende Flanke bei count up

    // Duty auf 10% einstellen
    EPwm1Regs.CMPA.half.CMPA = 40523;
}
