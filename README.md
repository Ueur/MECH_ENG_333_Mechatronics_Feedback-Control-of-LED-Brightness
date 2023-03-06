# Mechatronics - Feedback Control of LED Brightness
This is the second to last project of the course MECH_ENG_333, which I took in the winter of 2021, which uses C, PuTTY, and MATLAB to use feedback control to alter the brightness of an LED. This project uses counter/timer, output compare, and analog input peripherals, as well as the parallel master port for the LCD screen. The sensor that measures the brightness of the LED tracks the desired brightness, as shown by the square waves in MATLAB in the video, demonstrating that the feedback control is successful.

## How to run

1. Wire the circutboard as shown in the picture in the assignment writeup.
2. Load talkingPIC.c navigating to the 24.8 folder in the command prompt. 
3. Build the project by typing ```bash make```, which would assemble the C files into O object files.
4. Put the PIC32 into receive mode by pressing the RESET and USER buttons on the microcontroller.
5. Run ```bash make write```, which would activate the bootloader utility nu32utility and program the PIC32 with out.hex.
6. Open a serial port in MATLAB by running talkingPIC.m. 
7. PIC32 will send 200 plot points to MATLAB running the code in pid_plot.m

## Notes

The final iteration of the project is in 24.8, but the steps along the way are recorded in the other folders. The resistor values, amongst others, can be changed and modified according to need. A working video is available in this repository, as well as the assignment write-up.

## License

[MIT](https://choosealicense.com/licenses/mit/)
