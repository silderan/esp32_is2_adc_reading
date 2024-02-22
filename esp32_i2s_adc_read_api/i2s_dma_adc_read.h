/**
 * This version of i2s DMA ACS read es more suitable for API.
 * For that, all possible configurations are done via class functions.
 * Defines here are for variables that are firmware/hardware/library dependant and
   should be modified if that changes.
 * There are also some convenient macros.
 */

#ifndef I2S_DMA_ADC_READ_H
#define I2S_DMA_ADC_READ_H
#include <driver/i2s.h>

// The I2S to use.
// There are 2: NUM_0 and NUM_1
// BUT, only NUM_0 can be binded to ADC
// So, don't change this.
#define I2S_NUM   (I2S_NUM_0)

// There are 2 ADC units in 
// BUT only ADC_UNIT_1 can be binded to IS2
// So, don't change this.
#define ADC_UNIT  (ADC_UNIT_1)

// This is the amount of bites used to send ADC1 channel to use.
// This value es also fixed and is hardcoded in ESP32 firmware.
// So, don't change this unless some ESP32 revision or other future changes.
#define ADC1_CHANNEL_BITS_PER_FRAME  (4)

// ADC Resoluttion
// This is the ADC bits resolution. 
// This value es also fixed and is hardcoded in ESP32 firmware.
// So, don't change this unless some ESP32 revision or other future changes.
#define ADC_RESOLUTION  (12)

// This macro calculates maximum value readed by ADC.
// Being 12 bits (actual value) it's 0xFFF (or 4096)
// While bits resolution firmware value keeps below max bits of int, it can be calculated.
#if ADC_RESOLUTION == 32
#define ADC_MAX_VALUE   (unsigned int(0xFFFFFFFF))
#else
#define ADC_MAX_VALUE   (unsigned((1 << ADC_RESOLUTION)-1))
#endif

// ADC max voltage allowed in millivolts
// This is the maximum voltage that can be readed by ADC.
// It's a fixed value up to ESP32 design. Even far from firmware.
// So, don't change this unless some ESP32 revision or other future changes.
#define ADC_MAX_MVOLTS    (3300)

// Some handly macros for ADC readed values.
#define ADC_VALUE_TO_MVOLTS(val)    (val*ADC_MAX_MVOLTS/ADC_MAX_VALUE)
#define ADC_VALUE_TO_VOLTS(val)     (ADC_VALUE_TO_MVOLTS(val)/1000.0)
#define ADC_MVOLTS_TO_VALUE(mv)     (mv*ADC_MAX_VALUE/ADC_MAX_MVOLTS)
#define ADC_VOLTS_TO_VALUE(vol)     (ADC_MVOLTS_TO_VALUE(vol*1000))

// This macro gets ADC1 channel from frame value.
// I'll keep this macro that don't matter about frame variable type.
// Don't change this unless some ESP32 revision or other future changes.
#define ADC1_CHANNEL_FROM_FRAME(frameValue)  (frameValue >> sizeof(frame_t)*8 - ADC1_CHANNEL_BITS_PER_FRAME)
// Actually, this other macro will do the trick if you use 16bits sampling
//#define ADC1_CHANNEL_FROM_FRAME(frameValue)   (frameValue >> 12)

// This defines frame resolution (the amount of bits used for I2S)
// I2S is able to read up to 32 bits.
// But internat ADC is only 12 bits and uses the 4 extra bits to store ADC channel in use
// So, perfect sampling is 16 bits, but you can use other to see what changes in readed values.
// Note: that we cannot use any of the I2S_BITS_PER_FRAME_XBIT names here because this actually
//       are enums and not preprocessor definitions. So to mute the code up to sampling resolution,
//       we must create this define.
// Note: For now, I2S only can operate in 32, 24 or 16 bits. Others will raise an error.
//       8 bits are allowed by 12S, but it uses 2 consecutive bits so its nonsense to allow here
//       because just adds complexity.
#define SAMPLING_RESOLUTION (16)

#if SAMPLING_RESOLUTION == 32
# warning "Using 32 bits sampling resolution. Frame variable will be 32 bits too."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_32BIT
typedef uint32_t frame_t;
// Real value from frame. Its 12 bits lower bits in the 16 higher bits.
# define ADC_VALUE_FROM_FRAME(frameValue)        ((frameValue >> 16) & 0xFFF)

#elif SAMPLING_RESOLUTION == 24
# warning "Using 24 bits sampling resolution. Frame variable will be 32 bits."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_24BIT
typeddef uint32_t frame_t;
// Real value from frame. Its 12 bits lower bits in the 16 higher bits.
# define ADC_VALUE_FROM_FRAME(frameValue)        ((frameValue >> 16) & 0xFFF)

#elif SAMPLING_RESOLUTION == 16
# warning "Using 16 bits sampling resolution. Frame variable will be 16 bits too."
# define I2S_BITS_PER_FRAME I2S_BITS_PER_SAMPLE_16BIT
typedef uint16_t frame_t;
// Real value from frame. Its 12 bits lower bits in value.
# define ADC_VALUE_FROM_FRAME(frameValue)        (frameValue & 0xFFF)

#else
# error "SAMPLING RESOLUTION CAN ONLY BE 32, 24 or 16"

#endif SAMPLING_RESOLUTION

class I2sDmaAdcClass
{
private:
  adc1_channel_t mAdcInputPin;
  uint16_t mSamplingRate;
  uint16_t mFramearrayLen;
  uint8_t mI2sFrameBits;
  uint16_t mOffset;
  frame_t *mFrames;
  size_t mFrameBytesSize;

public:
  adc1_channel_t adcInputPin() const                    { return mAdcInputPin;  }
  adc1_channel_t adcInputPin(const adc1_channel_t pin)  { return mAdcInputPin = pin;   }

  uint16_t samplingRate() const             { return mSamplingRate;   }
  /**
   @brief sets sampling rate for I2S
   @return sampling rate set.
   @details There is not hard-coded limit here to set it, but as the I2S is bond to ADC
   its samplingRate is limited to ADC.
   Documentation says that I2S reading to ADC limits sampling rate to 150Khz.
   But, my tests shows me that I2S fill framebuffer arround 1,4 Msps.
   Exactly, reports me 1402368 sps (0.36 micro seconds per ADC read)
   */
  uint16_t samplingRate(const uint16_t sps) { return mSamplingRate = sps;    }

  uint16_t framearrayLen() const             { return mFramearrayLen;   }
  uint16_t framearrayLen(const uint16_t len) { return mFramearrayLen = len;    }

  uint8_t i2sFrameResolution() const             { return mI2sFrameBits;   }
  uint8_t i2sFrameResolution(const uint8_t bits) { return mI2sFrameBits = bits;   }

  uint16_t offsetValue() const                { return mOffset;         }
  uint16_t offsetValue(const uint16_t off)    { return mOffset = off;   }

  uint16_t offsetMVolts() const               { return ADC_VALUE_TO_MVOLTS(offsetValue());    }
  uint16_t offsetMVolts(const uint16_t mv)    { return ADC_VALUE_TO_MVOLTS(offsetValue(ADC_MVOLTS_TO_VALUE(mv))); }

  uint16_t calculateOffsetValue(uint32_t mFindingTime);
  uint16_t efectiveValue(uint16_t frame) const;

  frame_t *init();
  /**
  @brief reads i2S data and writes to the buffer to be used by this thread.
  @return Amount of frames readed (beware, not bytes!)
  @details Reading when I2S-DMA internal buffer has readed ADC data, is as
  quick as 15 micro seconds per 1024 frames with a 40000 sample rate and 2 buffers
  If you keep reading until I2S-DMA internal buffer empties, read speed will match sampling-rate speed.
  For example, with a sampling rate of 4096 and a buffer of 1024, you cannot raead more than 4 times
  per second at 15 micro seconds.
  With a 40000 sps, ADC will read every 1/40000=25uSec. So, 1024 buffer needs 25ms.
  So. after read fom I2S, we need to wait (or do other things) this 25ms to be able to read un just 24uSecs.
  Of course, if with 25 ms is good for you, you will be reading at live time.
  With 40.000 sps and 1024 readings using 25ms, and a display with 25FPS (40ms), gives you a 15 ms to do other things.
  With this 15ms doing other things, ADC keeps reading, so buffer is half filled so reading frames will reduce
  and you'll have more time to parse and display.
  As you see, it's not as simple as seems.
  @see init()
  @see frames()
  */
  frame_t *doRead(size_t *framesReaded);

  frame_t *buildSignal(uint32_t fr, uint16_t amp = 100);

  /**
  @brief returns frame array.
  @return interfam fram array direction.
  @details variable type is, ant its size, is up to frame resolution defined in i2s_dma_ade_read.h
  */
  frame_t *frames()  { return mFrames; }

  I2sDmaAdcClass() 
    : mAdcInputPin(ADC1_CHANNEL_4)
    , mSamplingRate(40000)            // 40000 are suitable for audio VU spectrum.
    , mFramearrayLen(1024)            // bin count is based on this value. See above. So, 40000/1024 == 29Hz per bin.
    , mI2sFrameBits(16)               // 16 bits match with ESP32 ADC build in.
    , mOffset(0)                      // Half of 12 bits (0-4096) is the perfect offset. But use calculateOffset function for get your actual source offsett.
    , mFrames(nullptr)
    , mFrameBytesSize(0)
  { }
  ~I2sDmaAdcClass()
  {
    delete [] mFrames;
    mFrames = nullptr;
  }
};

#endif