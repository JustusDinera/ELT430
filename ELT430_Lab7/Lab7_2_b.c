/*
 * Lab7_2.c
 *
 *  Created on: 29.04.2020
 *      Author: dinera
 *      Description: Berechnung "y = sin(x)"
 */

#include <DSP28x_Project.h>
#include <math.h>

void main(void)
{
  // Variablendeklaration
  float x;
  float y[360];
  int i;

  // WD abschalten
  EALLOW;
  SysCtrlRegs.WDCR = 0xE8;
  EDIS;

  while(1)
    {
      for (i = 0; i < 360; i++) {
	  x = ((float)i * (float)3.14159) / 180;
	  y[i] = sin(x);
      }
    }
}



