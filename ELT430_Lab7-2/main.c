/*
 * Lab7_2.c
 *
 *  Created on: 29.04.2020
 *      Author: dinera
 *      Description: Sinusberechnung mit HW Unterstuetzung.
 */

#include <DSP28x_Project.h>
#include <math.h>

int main(void)
{
  float x,y[360];
  int i;

  EALLOW;
  SysCtrlRegs.WDCR = 0xE8;            // WD abschalten
  EDIS;
  while (1)
    {
      for (i = 0; i < 360; ++i) {
	  x = (float)i * 0.017453289;
	  y[i]= sin(x);
      }
    }

}
