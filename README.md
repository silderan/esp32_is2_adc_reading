# esp32_is2_adc_reading

This is a code for ESP32 chip, in C for use in Arduino IDE, that demostrates how yo use I2S interface along with ADC.

It is based on code found here: https://www.instructables.com/The-Best-Way-for-Sampling-Audio-With-ESP32/ which is also based on codes I've seen in other sites.
Variable types here are consistent. That weren't in other places. Not a big issue because, for example, size_t and int can be interchanged most of the times. But is more consistent.

## Some basis
The I2S is a standard for transmiting serialized digital sound data. But I'll use "I2S" here not for the standard but for the "device" embedded into ESP32 that uses this protocol.
As explained in instructables' web, I2S has the ability to read ADC (Analog to Digital Converter) by their own, without using CPU. Furthermore, I2S will use the DMA. Direct Memory Access (DMA) is a feature that allows some "devices", the I2S in this case, to access the memory wihout the needs of the CPU.
So, configuring I2S properly, we instruct it to read ADC and write data into memory without CPU aware.
Later and continously, we ask I2S for the data readed and it will make a simple memcpy() from the memory addressed by DMA to the memory we are using in our code.
The readed data is allways the most recent one in the older DMA buffer.
For example, if IS2 reads 2000 samples per second and we configure 4 buffers, but our code is too slow and cannot read more than 500 samples from it, we'll get the most recet 500 frames of the 4th buffer. So our reads will be "delayed".
This is quite well explained here: https://www.youtube.com/watch?v=ejyt-kWmys8

## About the code purpose
This code has many many comments for learning purpose, based on my own experience, that will focus on several important details that could be easily skiped. That was for me, at least. Take a look.

## About the hardware
For testing it, you don't need analog AC signal. IS2 + ADC is "just" a analog-to-digital high speed and CPU resource free converter. Doesn't matter if you read AC or DC voltage. Furthermore, as this code prints out bunchs of readed values, it's more convenient to read DC values that doesn't change over time.

I recomend to use a variable resistor so you can manually change voltage from 0 to 3.3 volts in the input pin.

### Diagram

![ESP32 I2S ADC Demo Diagram](https://github.com/silderan/esp32_is2_adc_reading/blob/main/esp32_i2s_adc_read/esp21_i2s_adc_demo.png?raw=true)

Potenciometer value is quite irrelevant if you don't lower impedance enough to burn it or increase it up to make impossible to send current to input pin.
I guess, any value from 2k to 100K should do the trick.


This is my board. Quite simple to make it work!

![ESP32 I2S ADC Demo Diagram](https://github.com/silderan/esp32_is2_adc_reading/blob/main/esp32_i2s_adc_read/esp32_i2s_adc_read_demo_real_.jpeg?raw=true)


## TODO!
For 8 bits resolution, 12 + 4 bits are stored using 2 consecutive bytes. So, I need to make many changes on the code to make it work with this low resolution.
