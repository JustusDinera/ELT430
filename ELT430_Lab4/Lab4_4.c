/*
 * Lab4_1.c
 *  - Prozessor mit 90MHz arbeiten
 *  - Lauflicht
 *  - CPU-Timer0 umständlicher weg (Registerzugriff)
 *
 *  Created on: 14.04.2020
 *      Author: Justus Schneider
 *
 */

#include "DSP28x_Project.h"

void main (void)
{
    //Lokale Variablen********************************************************************************************************************************
    unsigned int richtung;  //Richtung des Lichtes
    unsigned int zaehler = 1;   //Variable für das Zählen
    unsigned int schalter;  //Variable für S1 ==> zur Rückgabe

    //Aufrufen der Funktionen*************************************************************************************************************************
    InitSysCtrl();

    //CPU-Timer0 mit Registerzugriff*******************************************************************************************************************
    CpuTimer0Regs.PRD.all = 18000000;   //18000000 Takte für 200ms Periode
    CpuTimer0Regs.TCR.bit.TRB = 1;  //reload aktivieren
    CpuTimer0Regs.TCR.bit.TSS = 0; // CPU-Timer rücksetzen

    //Grundeinstellungen für Ausgänge usw ************************************************************************************************************
    EALLOW;     //Makro= Zugang Freischaltung zu wichtigen Registern

    //Watchdog einstellen
    SysCtrlRegs.WDCR = 0x2D;    //Watchdog einschalten und Prescaler (Faktor für Ausgabe eines Impules (Zeitschleife) des Watchdogs) auf 16 setzen

    //Initialisierung der GPIO-Pins
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0; //Eingang auf 0 setzen für den Schalter
    GpioCtrlRegs.GPADIR.bit.GPIO17 = 1; //GPBDIR= besteht aus 32-Bit; Eingang/Ausgang freischalten der einzelnen Leitungen
    GpioCtrlRegs.GPBDIR.bit.GPIO50 = 1;
    GpioCtrlRegs.GPBDIR.bit.GPIO51 = 1;
    GpioCtrlRegs.GPBDIR.bit.GPIO55 = 1;

    EDIS;   //Sperrt zugang zu den Registern

    //Hauptfunktionen***********************************************************************************************************************************
    while(1)
    {
        while(CpuTimer0Regs.TCR.bit.TIF == 1)
        {
          //LEDs definieren (Ausgangszustand)*******************************************************************************************************
          GpioDataRegs.GPBCLEAR.bit.GPIO55 = 1; //LED P4 aus
          GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1; //LED P2 aus
          GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;   //LED P1 aus
          GpioDataRegs.GPBCLEAR.bit.GPIO51 = 1;   //LED P3 aus

           //LED binär setzen und laufen lassen******************************************************************************************************
          switch(zaehler){

          case 1: //Binär 1
              GpioDataRegs.GPASET.bit.GPIO17 = 1 ;   //LED P1 ein
              richtung=1; //Anfang
              break;

          case 2: //Binär 2
              GpioDataRegs.GPBSET.bit.GPIO50 = 1 ; //LED P2 ein
              break;

          case 4: //Binär 4
              GpioDataRegs.GPBSET.bit.GPIO51 = 1 ;   //LED P3 ein
              break;

          case 8: //Binär 8 für Zählerwert
              GpioDataRegs.GPBSET.bit.GPIO55 = 1 ; //LED P4 ein
              richtung=0; //Ende
              break;

          default:
              break;
          }

          //Richtung des Lauflichtes festlegen und weiterschieben***************************************************************************************
          if(richtung)
          {
              zaehler=zaehler<<1;
          }else
          {
              zaehler=zaehler>>1;
          }

          //Kriterien für erlauben das Ausführens bzw bis abgebrochen werden soll************************************************************************
         do{
         //Watchdog (Zähler auf 0 zurücksetzen damit Zähler nicht überlaufen wird) = "Good Key" ==> zweimal in Register "WDKEY" schreiben
             EALLOW;   //Makro = erlaubt beschreiben des kritischen Registers
             SysCtrlRegs.WDKEY = 0x55;
             SysCtrlRegs.WDKEY = 0xAA;
             EDIS;

             //CPU-Timer zurücksetzen
             CpuTimer0Regs.TCR.bit.TIF=1;

             //Schalter auslesen
             schalter= GpioDataRegs.GPADAT.bit.GPIO12;    //Auslesen des Wertes vom Schalter (ob 0 oder 1 bzw on oder off) ==> Pegelabfrage
           }while(!schalter);

        }//Ende von while (CpuTimer0Regs.TCR.bit.TIF == 1)
  }
}

