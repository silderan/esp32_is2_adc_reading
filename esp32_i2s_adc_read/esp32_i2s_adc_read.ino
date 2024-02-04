#include <driver/i2s.h>

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
#define ADC_INPUT ADC1_CHANNEL_5

// Sampling rate (how many times per second the I2S will read from ADC)
// Using internal ADC, this is limited to 150000 (150Khz)
#define SAMPLING_REQUENCY 10

// Frame array lenght.
// This value may impact in performance and/or acurate when reading high frecuencies (audio, for example)
// I haven't done any test right now.
// A value of 8 is good for this code that only prints value on console.
// Note: We use "lengh" and not "bytes". This is because there are a consideration:
//       When traversing frame data, we must do it up to its variable size.
//       That is, uintXX_t size and not by byte.
//       But, in some i2s_xxx functions, asks for size in bytes. That "size" must be calculated via sizeof()
#define FRAMEARRAY_LEN 8

// This defines frame resolution (the amount of bits used for I2S)
// I2S is able to read up to 32 bits.
// But internat ADC is only 12 bits and uses the 4 extra bits to store ADC channel in use
// So, perfect sampling is 16 bits, but you can use other to see what changes in readed values.
// Note: that we cannot use any of the I2S_BITS_PER_FRAME_XBIT names here because this actually
//       are enums and not preprocessor definitions. So to mute the code up to sampling resolution,
//       we must create this define.
// Note: For now, I2S only can operate in 32, 24, 16 or 8 bits. Others will raise an error.
#define SAMPLING_RESOLUTION (16)

#if SAMPLING_RESOLUTION == 32
# warning "Using 32 bits sampling resolution. Frame variable will be 32 bits too."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_32BIT
# define FRAME_VARIABLE_TYPE uint32_t
// Real value from frame. Its 12 bits lower bits in the 16 higher bits.
# define VALUE_FROM_FRAME(frameValue)        ((frameValue >> 16) & 0xFFF)

#elif SAMPLING_RESOLUTION == 24
# warning "Using 24 bits sampling resolution. Frame variable will be 32 bits."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_24BIT
# define FRAME_VARIABLE_TYPE uint32_t
// Real value from frame. Its 12 bits lower bits in the 16 higher bits.
# define VALUE_FROM_FRAME(frameValue)        ((frameValue >> 16) & 0xFFF)

#elif SAMPLING_RESOLUTION == 16
# warning "Using 16 bits sampling resolution. Frame variable will be 16 bits too."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_16BIT
# define FRAME_VARIABLE_TYPE uint16_t
// Real value from frame. Its 12 bits lower bits in value.
# define VALUE_FROM_FRAME(frameValue)        (frameValue & 0xFFF)

#elif SAMPLING_RESOLUTION == 8
# warning "Using 8 bits sampling resolution. Frame variable will be 8 bits too."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_8BIT
# define FRAME_VARIABLE_TYPE uint8_t
// Real value from frame. Not sure, yet, if data are 8 bits or 4
# define VALUE_FROM_FRAME(frameValue)        (frameValue & 0xFF)

#else
# error "SAMPLING RESOLUTION CAN ONLY BE 32, 24, 16 or 8"

#endif SAMPLING_RESOLUTION

// Actual Frame array.
// As we're using internat ADC and this generates 12+4 bits of data,
// 12 bits for value plus 4 bits for ADC channel, 16 bits variable is perfect.
// Variable type used is up to SAMPLING_RESOLUTION.
// Actually, you can use any variable type you like because i2s_read writes in this
// array byte per byte and is limited to its size. So, you'll never get overflow.
// But, your data will be messed and hard to read if you use higher sampling resolution
// than frame variable size.
FRAME_VARIABLE_TYPE frames[FRAMEARRAY_LEN];

// This defines the size of the array in bytes.
// Some I2S api functions need it.
// You don't need to modify it and never use it in for or while loops; use FRAMEARRAY_LEN instead.
// Note that with 16 bits sambles (2 bytes) this will double FRAMEARRAY_LEN
#define FRAMEARRAY_BYTES (sizeof(frames))


// The I2S to use.
// There are 2: NUM_0 and NUM_1
// BUT, only NUM_0 can be binded to ADC
// So, don't change this.
#define I2S_NUM I2S_NUM_0

// This is the amount of bites used to send ADC1 channel to use.
// This value es also fixed and is hardcoded in ESP32 firmware.
// So, don't change this unless some ESP32 revision or other future changes.
#define ADC1_CHANNEL_BITS_PER_FRAME  (4)

// This macro gets ADC1 channel from frame value.
// I'll keep this macro that don't matter about frame variable type.
// Don't change this unless some ESP32 revision or other future changes.
#define ADC1_CHANNEL_FROM_FRAME(frameValue)  (frameValue >> sizeof(frames[0])*8 - ADC1_CHANNEL_BITS_PER_FRAME)
// Actually, this other macro will do the trick if you use 16bits sampling
//#define ADC1_CHANNEL_FROM_FRAME(frameValue)   (frameValue >> 12)

void setupSerial()
{
  Serial.begin(115200);
  Serial.println("Serial initialized");
}

void setup()
{
  setupSerial();
  setupI2S();
}

void loop()
{
  printBufferData(readI2SData());
}

void setupI2S()
{
  Serial.println("Configuring I2S...");
  esp_err_t err;


  const i2s_config_t i2sConfig = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER                   // Is a master, not slave of another I2S.
                         | I2S_MODE_RX                       // We're reading, not writing
                         | I2S_MODE_ADC_BUILT_IN)            // We're going to bind to a internat ADC
      ,.sample_rate           = SAMPLING_REQUENCY
      ,.bits_per_sample       = I2S_BITS_PER_FRAME
      ,.channel_format        = I2S_CHANNEL_FMT_ALL_RIGHT    // If channel is mono, stereo... Actually, doesn't matter what you choose here. Maybe because we're using internat ADC
      ,.communication_format  = I2S_COMM_FORMAT_I2S_MSB      // Some fine tuning format. Actually, doesn't matter what you choose here. Maybe because we're using internat ADC
      ,.intr_alloc_flags      = ESP_INTR_FLAG_LEVEL1         // Not sure.
      ,.dma_buf_count         = 4                            // Number of buffers. It's used for I2S to fill if main thread cannot read from I2S quick enouth. Not sure if that matters.
      ,.dma_buf_len           = FRAMEARRAY_LEN               // Frames per buffer. Note: Not bytes!
      ,.use_apll              = false                        // I2S using APLL as main I2S clock, enable it to get accurate clock. Not necessary.
//    ,.tx_desc_auto_clear    = false,                       // I2S auto clear tx descriptor if there is underflow condition. We're not transmiting. 
//    ,.fixed_mclk            = 1                            // Don't know what's this.
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

  switch( err = i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT) )
  {
  case ESP_OK:
    Serial.printf("I2S ADC Mode activaded. ADC unit 1 channel %d binded.\n", ADC_INPUT);
    break;
  default:
    Serial.printf("Unknown %d error while setting up ADC mode (binding to ADC Channel). Halting...\n", ADC_INPUT, err);
    while( true )
      ;
  }
  Serial.println("I2S driver installed.");
}

/**
 @brief reads data from I2S and stores it in frames array.
 @return Frames readed. Beware, not bytes! You dont need to divide this with sizeof(frames[0]). It's done already
*/
size_t readI2SData()
{
  size_t bytesReaded = 0;
  i2s_read(I2S_NUM,
          (void*)frames,
          FRAMEARRAY_BYTES,
          &bytesReaded,
          portMAX_DELAY); // no timeout

  if( bytesReaded != FRAMEARRAY_BYTES )
    Serial.printf("Could only read %u bytes of %u in FillBufferI2S()\n", bytesReaded, FRAMEARRAY_BYTES);
  return bytesReaded / sizeof(frames[0]);
}

void printBufferData(size_t framesReaded)
{
  Serial.printf("%'0'2d frames from channel %d: ", framesReaded, ADC1_CHANNEL_FROM_FRAME(frames[0]));
  for( size_t i = 0; i < framesReaded; i++ )
    Serial.printf("%X, ", VALUE_FROM_FRAME(frames[i]) );
  Serial.printf("\n"); 
}
