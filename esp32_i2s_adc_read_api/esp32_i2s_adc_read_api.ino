/*
 This code is exactly the same as esp32_i2s_adc_read, but makes use of an external class
 witch is more suitable to use it anywere.

 Please, note that the I2sDmaAdcClass is not complete yet.
 */

#include "i2s_dma_adc_read.h"

// Defines ADC input channel in use.
// Look at your actual ESP32 to be sure where it's connected.
// In my ESP32 dev kit:
// ADC1_CH4 -> GPIO32
// ADC1_CH5 -> GPIO33

// This are not usable: only ADC1 can be binded to I2S.
// ADC2_CH0 -> GPIO04
// ADC2_CH1 -> GPIO0 (With pull-up resistor)
// ADC2_CH2 -> GPIO2
// ADC2_CH3 -> GPIO15
// ADC2_CH4 -> GPIO13
// ADC2_CH5 -> GPIO12
// ADC2_CH6 -> GPIO14
#define ADC_INPUT         (ADC1_CHANNEL_4)

// Sampling rate (how many times per second the I2S will read from ADC)
// Using internal ADC, this is limited to 150000 (150Khz)
#define SAMPLING_FREQUENCY 40000

// Define this to see just one value in serial port, so it can be seen in plotter graphic view
// That value es for "how many times the average will be calculated and print to serial"
#define PLOTTER_AVERAGE_VIEW_PER_SECOND (25)  // 25 times per second. Not sure if it's too many or too few.

// Frame array lenght.
// This value may impact in performance and/or acurate when reading high frecuencies (audio, for example)
// With lower values, more CPU and lower latency.
// With Higher values, less CPU and more latency.
// Lower values sweets fine for discrete value printing to see them un Monitor
// A value of 16 is good for this matter.
// Higher values can be used for average value printing to see it in plotter.
// Trying 1025
// Note: We use "lengh" and not "bytes". This is because there are a consideration:
//       When traversing frame data, we must do it up to its variable size.
//       That is, uintXX_t size and not by byte.
//       But, in some i2s_xxx functions, asks for size in bytes. That "size" must be calculated via sizeof()
#ifdef PLOTTER_AVERAGE_VIEW_PER_SECOND
# define FRAMEARRAY_LEN 1024
#else
# define FRAMEARRAY_LEN 16
#endif

#define INITIAL_PERFORMANCE_TEST_TIME  (2000)

I2sDmaAdcClass i2sDmaAdc = I2sDmaAdcClass();

void setupSerial()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial initialized.");
}

void setupI2S()
{
  i2sDmaAdc.framearrayLen(FRAMEARRAY_LEN);
  i2sDmaAdc.samplingRate(SAMPLING_FREQUENCY);
  i2sDmaAdc.adcInputPin(ADC_INPUT);
  i2sDmaAdc.init();
}

void setup()
{
  setupSerial();
  setupI2S();

  performanceTest( );
}

void loop()
{
  size_t framesReaded;
  printBufferData( i2sDmaAdc.doRead(&framesReaded), framesReaded );
}

void performanceTest()
{
#ifdef INITIAL_PERFORMANCE_TEST_TIME
  // Let's make a performance test during some time.
  // Note: As the amount of samples is fixed by SAMPLING_FREQUENCY value, this report shall allways
  //       match that SAMPLING_FREQUENCY.
  //       It will never go beyong SAMPLING_FREQUENCY, at least.
  //       But, it can be lower: If processing frames code is time consuming, ADC-SPI will overflow DMA buffer.
  static uint32_t t;
  if( !t )
  {
    uint32_t thisFramesReaded, accFramesReaded = 0;
    Serial.printf("Initiating performance test for %d seconds\n", INITIAL_PERFORMANCE_TEST_TIME / 1000);
    t = millis() + INITIAL_PERFORMANCE_TEST_TIME;
    while( t > millis() )
    {
      i2sDmaAdc.doRead(&thisFramesReaded);
      accFramesReaded += thisFramesReaded;
    }
    Serial.printf("Performance test ends reading %d frames in %d seconds. That is %d frames/sec\n", accFramesReaded, INITIAL_PERFORMANCE_TEST_TIME / 1000, accFramesReaded/(INITIAL_PERFORMANCE_TEST_TIME / 1000));
    Serial.printf("In other words, every analog read did take %f micro seconds.\n", 1/(accFramesReaded/1000000.0));
    Serial.println("Waiting 5 seconds for printing reads...");
    delay(5000);
  }
#endif
}

#ifndef PLOTTER_AVERAGE_VIEW_PER_SECOND
void printBufferData(const frame_t *frames, size_t framesReaded)
{
  Serial.printf("%'0'2d frames from channel %d: ", framesReaded, ADC1_CHANNEL_FROM_FRAME(frames[0]));
  for( size_t i = 0; i < framesReaded; i++ )
    Serial.printf("%X, ", ADC_VALUE_FROM_FRAME(frames[i]) );
  Serial.printf("\n"); 
}
#else // defined PLOTTER_AVERAGE_VIEW_PER_SECOND
void printBufferData(const frame_t *frames, size_t framesReaded)
{
  const int fps = 1000/PLOTTER_AVERAGE_VIEW_PER_SECOND;
  static uint32_t t;
  static size_t totalFramesReaded;
  static uint32_t totalValueReaded;
  if( !t )
    t = millis() + fps;
  totalFramesReaded += framesReaded;
  for( size_t i = 0; i < framesReaded; i++ )
    totalValueReaded += ADC_VALUE_FROM_FRAME(frames[i]);
  if( t < millis() )
  {
    t = millis() + fps;
    Serial.printf("%d,%d,%d\n", 1, 4000, totalValueReaded/totalFramesReaded);
    totalValueReaded = 0;
    totalFramesReaded = 0;
  }
}
#endif //PLOTTER_AVERAGE_VIEW_PER_SECOND