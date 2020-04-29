/*
 * Lab7_2.c
 *
 *  Created on: 29.04.2020
 *      Author: dinera
 *      Description: Berechnung "y = sin(x)"
 */

#include <DSP28x_Project.h>

void main(void)
{
  // Variablendeklaration
  float x;
  float y[360];

  // WD abschalten
  EALLOW;
  SysCtrlRegs.WDCR = 0xE8;
  EDIS;

  while(1)
    {
      for (x = 0; x < 359; x++) {
	  y[x] = sin(x);
      }
    }
}



