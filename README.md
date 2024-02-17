# esp32_is2_adc_reading

This is a code for ESP32 chip, in C for use in Arduino IDE, that demostrates how you can use I2S interface along with ADC.

It is based on code found here: https://www.instructables.com/The-Best-Way-for-Sampling-Audio-With-ESP32/ which is also based on codes I've seen in other sites.
Variable types here are consistent. Those weren't in other places is seen. Not a big issue because, for example, size_t and int can be interchanged most of the times. But is more consistent and correct using size_t to store array sizes instead of ussing signed int types.

## Some basis
I2S is the standard name for transmiting serialized digital sound data.

But I'll use "I2S" here not for the standard name but for the "device" embedded into ESP32 that uses this protocol.

As explained in instructables' web, I2S has the ability to read ADC (Analog to Digital Converter) by their own, without using CPU. Furthermore, I2S will use the DMA.\
Direct Memory Access (DMA) is a feature that allows some "devices", the ESP32's I2S, in this case, to access the memory wihout the needs of the CPU.

So, configuring the I2S device properly, we instruct it to read from ADC storing data into DMA memory without CPU aware.\
Later and continously, we ask I2S for the data readed, and it will make a simple memcpy() from the memory addressed by DMA to the memory we are using in our code.

### I2S-DMA-ADC Readed data details among buffers.
I've not seen anywhere the exact implementationm, but what I guess and readed in the web is this:\
The I2S-DMA configuration needs you to specify two values:\
1. Buffer size\
   This is the I2S internal memory that I2S-DMA will fill with the readed data from ADC
3. Buffer quantity\
   This says how many buffers of internal memory I2S-DMA will allocate and use.

IS2-DMA will fillup buffers one after another and restarting writing to the first one once it fill the last buffer (circular cycle)
When from outside of I2S DMA we ask for the data, I2S won't give us the "current" buffers that are filling. It could be for some reasons (threads, securiry, reduce complexity, speedup reading process.. whatever), but it's irrellevant now and you must take it as it is.

That limitation implies...
1. You cannot get live ADS data. It's allways delayed by the time I2S-DMA-ADC needs to read one full buffer.
2. As it's delayed, longer buffer, longer delays.
3. You must configure I2S to use at least 2 buffers (8 maximum, as per now): one for DMA writing and the other for we to read from the main code/thread
4. If you configure more than two buffers, data readed from main code will come from the oldest written buffer.
5. So, data is delayed even more. Generaly, it's real_delay = buffer_reading_time * (buffer_count - 1), if we read when all buffers are full  (see later explanation about that)

There is something more to say.\
If you read from I2S fast enough as I2S is able to read ADC, your read will be slowed down because I2S will wait until internal memory buffer is filled so you'll get all buffer data.\
This behaviour can be changed by defining a maximum of CPU ticks allowed for you to read from I2S. So, after that time expires, i2s_read will return with bufferLen < bufferReadedFrames.\
IMHO, this is a bad desing idea, because we'll need to calculate this CPU ticks if our code is able to process any data lenght readed as quick as it comes rather that wait until it's full. I didn't make any tests limiting CPU ticks when reading from I2S.\
ESP32 doc (https://docs.espressif.com/projects/esp-idf/en/v4.2.3/esp32/api-reference/peripherals/i2s.html#_CPPv48i2s_read10i2s_port_tPv6size_tP6size_t10TickType_t) is far from good at this (and other function) details. No word about what happen if no or some data is available before ticks run out.\
I can't imagine the exact behaviour. I need to test or find it somewhere in web.

#### So, how to choose buffer size and buffer number.
It's hard to decide and you need to make tests, and think it carefully. Keep in mind this thoughts:
- Do you care about lossing data? more buffers
- Do you need many data? bigger buffer
- Do you spend many time processing data? More buffers.

And, here again, there is another thing to talk about.\
Internal I2S buffer size and i2s_read main buffer array size could not be the same.\
Again, no word in official documentation about:
- What happens if main buffer is smaller that internal I2S buffer.
- What happens if main buffer is larger that internal I2S buffer.
For example, if main buffer is half than internal one and we call i2s_read twice, will or buffer be filled from the same internal buffer, the first read with the first half of internal buffer and the last half in the second read?\
Or i2s_read will read from two differents buffers?\
If it's the second scenario, will be our data the "most recent half" (the second half) from buffer or the "less recent" (the first half)?

## About the code purpose
This code has many many comments for learning purpose, based on my own experience, that will focus on several important details that could be easily skipped. Take a look.

## About the hardware
For testing it, you don't need analog AC signal. IS2 + ADC is "just" a analog-to-digital high speed and CPU resource free converter. Doesn't matter if you read AC or DC voltage. Furthermore, as this code prints out bunchs of readed values, it's more convenient to read DC values that doesn't change over time.

I recomend to use a variable resistor so you can manually change voltage from 0 to 3.3 volts in the input pin.

### Diagram

![ESP32 I2S ADC Demo Diagram](https://github.com/silderan/esp32_is2_adc_reading/blob/main/esp32_i2s_adc_read/esp21_i2s_adc_demo.png?raw=true)

Potenciometer value is quite irrelevant here. If you don't lower impedance enough to burn it or increase it up to make impossible to send current to input pin, it's fine.
I guess, any value from 2k to 100K should do the trick.


This is my board. Quite simple to make it work!

![ESP32 I2S ADC Demo Diagram](https://github.com/silderan/esp32_is2_adc_reading/blob/main/esp32_i2s_adc_read/esp32_i2s_adc_read_demo_real_.jpeg?raw=true)
