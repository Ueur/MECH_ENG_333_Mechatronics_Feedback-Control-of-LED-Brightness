#include "NU32.h"  // constants, functions for startup and UART
#include "stdio.h" //added this to use sprintf

#define NUMSAMPS 1000                   // number of points in waveform
#define PLOTPTS 200                     // number of data points to plot
#define DECIMATION 10                   // plot every 10th point
#define SAMPLE_TIME 10                  // add this to be able to use adc_sample_convert function
static volatile int Waveform[NUMSAMPS]; // waveform
static volatile int ADCarray[PLOTPTS];  // measured values to plot
static volatile int REFarray[PLOTPTS];  // reference values to plot
static volatile int StoringData = 0;    // if this flag = 1, currently storing
// plot data
static volatile float Kp = 0, Ki = 0; // control gains
static volatile int u = 0, unew = 0, e = 0, eint = 0;

unsigned int adc_sample_convert(int pin); // add this so we can use it in the ISR, since it is defined after

void __ISR(_TIMER_2_VECTOR, IPL5SOFT) Controller(void)
{
  static int counter = 0; // initialize counter once
  static int plotind = 0; // index for data arrays; counts up to PLOTPTS
  static int decctr = 0;  // counts to store data one every DECIMATION
  static int adcval = 0;  //

  OC1RS = (unsigned int)((unew / 100.0) * PR3);

  adcval = adc_sample_convert(0); //sample pin 0 for AN0

  e = Waveform[counter] - adcval;
  eint = eint + e;
  u = Kp * e + Ki * eint;
  unew = u + 50.0;
  if (unew > 100.0)
  {
    unew = 100.0;
  }
  else if (unew < 0.0)
  {
    unew = 0.0;
  }

  if (StoringData)
  {
    decctr++;
    if (decctr == DECIMATION)
    {                             // after DECIMATION control loops,
      decctr = 0;                 // reset decimation counter
      ADCarray[plotind] = adcval; // store data in global arrays
      REFarray[plotind] = Waveform[counter];
      plotind++; // increment plot data index
    }
    if (plotind == PLOTPTS)
    {                  // if max number of plot points plot is reached,
      plotind = 0;     // reset the plot index
      StoringData = 0; // tell main data is ready to be sent to MATLAB
    }
  }
  counter++; // add one to counter every time ISR is entered
  if (counter == NUMSAMPS)
  {
    counter = 0; // rollover counter over when end of waveform reached
  }
  IFS0bits.T2IF = 0;
}

void makeWaveform()
{
  int i = 0, center = 500, A = 300; //from the instructions
  for (i = 0; i < NUMSAMPS; ++i)
  {
    if (i < NUMSAMPS / 2)
    {
      Waveform[i] = center + A;
    }
    else
    {
      Waveform[i] = center - A;
    }
  }
}

unsigned int adc_sample_convert(int pin)
{ // sample & convert the value on the given
  // adc pin the pin should be configured as an
  // analog input in AD1PCFG
  unsigned int elapsed = 0, finish_time = 0;
  AD1CHSbits.CH0SA = pin; // connect chosen pin to MUXA for sampling
  AD1CON1bits.SAMP = 1;   // start sampling
  elapsed = _CP0_GET_COUNT();
  finish_time = elapsed + SAMPLE_TIME;
  while (_CP0_GET_COUNT() < finish_time)
  {
    ; // sample for more than 250 ns
  }
  AD1CON1bits.SAMP = 0; // stop sampling and start converting
  while (!AD1CON1bits.DONE)
  {
    ; // wait for the conversion process to finish
  }
  return ADC1BUF0; // read the buffer with the result
}

int main(void)
{
  char message[100];            // message to and from MATLAB
  float kptemp = 0, kitemp = 0; // temporary local gains
  int i = 0;                    // plot data loop counter
  NU32_Startup();               // cache on, interrupts on, LED/button init, UART init
  makeWaveform();
  T3CONbits.TCKPS = 0;    // Timer3 prescaler N=1 (1:1)
  PR3 = 3999;             // calculated in 24.2.1
  TMR3 = 0;               // initial TMR3 count is 0
  OC1CONbits.OCM = 0b110; // PWM mode without fault pin; other OC1CON bits are defaults
  OC1RS = 3000;           // duty cycle = OC1RS/(PR3+1) = 75%
  OC1R = 3000;            // initialize before turning OC1 on; afterward it is read-only
  T3CONbits.ON = 1;       // turn on Timer3
  OC1CONbits.ON = 1;      // turn on OC1

  OC1CONbits.OCTSEL = 1; // use timer3 for OC
  AD1PCFGbits.PCFG0 = 0; // use AN0 as adc pin
  AD1CON3bits.ADCS = 2;  // ADC clock period is Tad = 2*(ADCS+1)*Tpb =2*3*12.5ns = 75ns
  AD1CON1bits.ADON = 1;  // turn on A/D converter

  _CP0_SET_COUNT(0); // delay 4 seconds to see the 75% duty cycle on a 'scope
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
    NU32_ReadUART3(message, sizeof(message)); // wait for a message from MATLAB
    sscanf(message, "%f %f", &kptemp, &kitemp);
    __builtin_disable_interrupts(); // keep ISR disabled as briefly as possible
    Kp = kptemp;                    // copy local variables to globals used by ISR
    Ki = kitemp;
    __builtin_enable_interrupts(); // only 2 simple C commands while ISRs disabled
    StoringData = 1;               // message to ISR to start storing data
    while (StoringData)
    {   // wait until ISR says data storing is done
      ; // do nothing
    }
    for (i = 0; i < PLOTPTS; i++)
    { // send plot data to MATLAB
      // when first number sent = 1, MATLAB knows we’re done
      sprintf(message, "%d %d %d\r\n", PLOTPTS - i, ADCarray[i], REFarray[i]);
      NU32_WriteUART3(message);
    }
  }
  return 0;
}
