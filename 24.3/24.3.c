#include "NU32.h" // constants, functions for startup and UART

#define NUMSAMPS 1000                   // number of points in waveform
static volatile int Waveform[NUMSAMPS]; // waveform

void __ISR(_TIMER_2_VECTOR, IPL5SOFT) Controller(void)
{                         // _TIMER_2_VECTOR = 8
  static int counter = 0; // initialize counter once
  OC1RS = Waveform[counter];
  counter++; // add one to counter every time ISR is entered
  if (counter == NUMSAMPS)
  {
    counter = 0; // roll the counter over when needed
  }
  IFS0bits.T2IF = 0;
}

void makeWaveform()
{
  int i = 0, center = 2000, A = 1000; //A is (PR3+1)/4 = (3999+1)/4 = 1000. center is (3999+1)/2
  for (i = 0; i < NUMSAMPS; ++i)
  {
    if (i < NUMSAMPS / 2)
    {
      Waveform[i] = center + A; //this is 3000, which is 75%
    }
    else
    {
      Waveform[i] = center - A; //this is 1000, which is 25%
    }
  }
}

int main(void)
{
  NU32_Startup(); // cache on, interrupts on, LED/button init, UART init
  makeWaveform();
  T3CONbits.TCKPS = 0;    // Timer3 prescaler N=1 (1:1)
  PR3 = 3999;             // calculated in 24.2.1
  TMR3 = 0;               // initial TMR3 count is 0
  OC1CONbits.OCM = 0b110; // PWM mode without fault pin; other OC1CON bits are defaults
  OC1RS = 3000;           // duty cycle = OC1RS/(PR3+1) = 75%
  OC1R = 3000;            // initialize before turning OC1 on; afterward it is read-only
  T3CONbits.ON = 1;       // turn on Timer3
  OC1CONbits.ON = 1;      // turn on OC1
  _CP0_SET_COUNT(0);      // delay 4 seconds to see the 75% duty cycle on a 'scope
  while (_CP0_GET_COUNT() < 4 * 40000000)
  {
    ;
  }
  OC1RS = 3000; // keep duty cycle at 75%

  __builtin_disable_interrupts();
  T2CONbits.TCKPS = 0b001; // prescalar of 2, since we can't use prescalar of 1, otherwise it would have P be greater than 2^16-1
  PR2 = 39999;             //want t=1ms, T = (P + 1) × N × 12.5 ns -> (1*10^-3)/((12.5*10^-9)*2)-1 = P
  TMR2 = 0;
  T2CONbits.ON = 1;
  IPC2bits.T2IP = 5; //priority
  IPC2bits.T2IS = 0;
  IFS0bits.T2IF = 0;
  IEC0bits.T2IE = 1;
  __builtin_enable_interrupts();

  while (1)
  {
    ; // infinite loop
  }
  return 0;
}
