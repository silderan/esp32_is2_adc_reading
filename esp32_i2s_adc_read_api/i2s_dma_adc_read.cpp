#include "i2s_dma_adc_read.h"
#include <Arduino.h>

frame_t *I2sDmaAdcClass::init()
{
  Serial.println("Configuring I2S...");
  esp_err_t err;

  if( mFrames )
  {
    i2s_driver_uninstall(I2S_NUM);
    delete [] mFrames;
  }
  mFrames = new frame_t[framearrayLen()];
  mFrameBytesSize = sizeof(frame_t)*framearrayLen();

  const i2s_config_t i2sConfig = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER                   // Is a master, not slave of another I2S.
                         | I2S_MODE_RX                       // We're reading, not writing
                         | I2S_MODE_ADC_BUILT_IN)            // We're going to bind to a internal ADC
      ,.sample_rate           = mSamplingRate
      ,.bits_per_sample       = I2S_BITS_PER_FRAME
      ,.channel_format        = I2S_CHANNEL_FMT_ALL_RIGHT    // If channel is mono, stereo... Actually, doesn't matter what you choose here. Maybe because we're using internat ADC
      ,.communication_format  = I2S_COMM_FORMAT_I2S_MSB      // Some fine tuning format. Actually, doesn't matter what you choose here. Maybe because we're using internat ADC
      ,.intr_alloc_flags      = ESP_INTR_FLAG_LEVEL1         // Not sure.
      ,.dma_buf_count         = 8                            // Number of buffers. It's used for I2S-DMA to fill one buffer while main thread can read the other.
      ,.dma_buf_len           = framearrayLen()              // Frames per buffer. Note: Not bytes!. It's framearrayLen(), but could be larger.
      ,.use_apll              = false                        // I2S using APLL as main I2S clock, enable it to get accurate clock. Not necessary.
//    ,.tx_desc_auto_clear    = false,                       // I2S auto clear tx descriptor if there is underflow condition. We're not transmiting. 
//    ,.fixed_mclk            = 1                            // Don't know what is this.
  };

  switch( err = i2s_driver_install(I2S_NUM, &i2sConfig,  0, NULL) )
  {
  case ESP_OK:
    Serial.println("I2S driver installed.");
    break;
  case ESP_ERR_INVALID_ARG:
    Serial.println("Parameter error in i2s_driver_install. Halting...\n");
    while( true )
      ;
  case ESP_ERR_NO_MEM :
    Serial.println("Not enough memory installing I2S driver. Halting...\n");
    while( true )
      ;
  default:
    Serial.printf("Unknown %d error while installing I2S driver.\n", err);
    while( true )
      ;
  }

  switch( err = i2s_set_adc_mode(ADC_UNIT, adcInputPin()) )
  {
  case ESP_OK:
    Serial.printf("I2S ADC Mode activaded. ADC unit 1 channel %d binded.\n", adcInputPin());
    break;
  default:
    Serial.printf("Unknown %d error while setting up ADC mode (binding to ADC Channel). Halting...\n", adcInputPin(), err);
    while( true )
      ;
  }
  Serial.println("I2S driver installed.");
  return mFrames;
}

frame_t *I2sDmaAdcClass::doRead(size_t *framesReaded)
{
  esp_err_t err = i2s_read(I2S_NUM,
                          (void*)mFrames,
                          mFrameBytesSize,
                          framesReaded,     // Actually, this i2s_read is "bytes readed", but I can use this variable for now.
                          portMAX_DELAY);
  if( err != ESP_OK )
    switch( err )
    {
    case ESP_ERR_INVALID_ARG:
      Serial.println("Parameter error in i2s_read. Halting...\n");
      while( true )
        ;
    default:
      Serial.println("Unknown error in i2s_read. Halting...\n");
      while( true )
        ;
    }

  if( *framesReaded != mFrameBytesSize )
    Serial.printf("doRead-warning. Could only read %u bytes of %u in i2s_read()\n", *framesReaded, mFrameBytesSize);
  // Convert bytes readed to frames readed. Far more usefull for loops.
  *framesReaded /= sizeof( frame_t );
  return mFrames;
}

/**
 @brief Reads I2S data to calculate average that ussualy is the offset (DC component) of data.
 @param calcMSec time, in milliseconds, for calculating offset. Defaults to 2000 ms.
 @return average calculated.
 @details It uses current buffers and holds process until done. Take this into account.
 This funcions is usefull when you want to get some kind of average value so you wanna
 use effectiveValue() function that needs actual offset.
 Remember to set offsetvalue via offsetValue(calculateOffsetValue(...));
 */
uint16_t I2sDmaAdcClass::calculateOffsetValue(uint32_t calcMSec)
{
  uint32_t t = 0;
  uint32_t totalValueReaded = 0;
  uint32_t allFramesReaded = 0;
  uint32_t thisFramesReaded = 0;
  if( !t )
  {
    uint32_t framesReaded = 0;
    t = millis() + calcMSec;
    while( t > millis() )
    {
      doRead(&thisFramesReaded);
      allFramesReaded += thisFramesReaded;
      while( --thisFramesReaded )
        totalValueReaded += ADC_VALUE_FROM_FRAME(mFrames[thisFramesReaded]);
    }
  }
  return totalValueReaded/allFramesReaded;
}


/**
 * @brief Given a frame vale, returns its efective.
 *
 * Efective value eliminates offset and returns absolute value.
 * Before using this function, you must fix offset value.
 * @param frame It's the raw value readed, without any modification needed.
 * @return efective value if the signal readed by frame.
 * @details 
 * offset value is the DC component of the AC signal introduced to be read here.
 * ADC can read only posivite values, but analog AC signal has both: positive and negative.
 * So, all analog AC input signal must be added with some DC. This introduces two things to
 * take into account:
 * 1: ADC max readable value is 3.3V So, "perfect" offset is 1.615
 * 2: As for ADC max value and offset, max ADC is 1.615Vpp (voltang peak to peak)
 * You can know offset by reading microfone spects or, you already know if you design the circuit.
 * But, you can run some test, and can run some test to find it.
 */
frame_t I2sDmaAdcClass::efectiveValue(frame_t frame) const
{
  frame = ADC_VALUE_FROM_FRAME(frame);
  return frame > mOffset ? frame-mOffset : mOffset-frame;
}

frame_t *I2sDmaAdcClass::buildSignal(uint32_t fr, uint16_t amp)
{
  static uint32_t lastfr;
  if( lastfr != fr )
  {
    lastfr = fr;
    double ratio = 6.28318531 * fr / samplingRate(); // Fraction of a complete cycle stored at each sample (in radians)
    Serial.printf("Construyendo onda de %d Hz\n", fr);
    for( uint16_t i = 0; i < framearrayLen(); i++ )
    {
      mFrames[i] = amp * sin(i * ratio) / 2.0 + 1500;/* Build data with positive and negative values*/

//      Serial.println(int(mFrames[i])-1500);
      //vReal[i] = uint8_t((amplitude * (sin(i * ratio) + 1.0)) / 2.0);/* Build data displaced on the Y axis to include only positive values*/
  //    vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
    }
  }
  return mFrames;
}