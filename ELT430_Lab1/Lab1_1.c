/*
 * Lab1_1.c
 *
 *  Created on: 11.03.2020
 *      Author: dinera
 */

#include "DSP28x_Project.h"
void main(void) {
    EALLOW;
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1; //Ausgang setzen
    GpioCtrlRegs.GPBDIR.bit.GPIO39 = 1; // Ausgang setzen
    SysCtrlRegs.WDCR = 0xE8;            // WD abschalten
    EDIS;
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;
    GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;
    unsigned long i;

    while(1)
    {
        for(i=0;i<360000;i++);
        GpioDataRegs.GPBTOGGLE.all = 0x84;
    }
}



