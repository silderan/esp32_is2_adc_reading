#ifndef SpectrumLCDDisplay_H
#define SpectrumLCDDisplay_H

#include <vector>
#include <TFT_eSPI.h>

class SpectrumLCDDisplay : protected TFT_eSPI
{
  uint8_t mFalldownTimeInterval;
  uint8_t mFalldownHeight;
  uint8_t mBarsCount;
  uint16_t mWidth;
  uint16_t mHeight;
  uint16_t mBarWidth;
  uint32_t mBackgroundClr;
  std::vector<uint32_t> mBarOldValues;

public :
  void init(uint8_t barsCount, uint16_t width, uint16_t height, uint8_t falldownTimeInterval = 0, uint8_t falldownHeight = 0);
  void drawBars(const std::vector<uint32_t> &barValues);
  void printDetailedInfo() const;

  uint8_t falldownTimeInterval() const    { return mFalldownTimeInterval;  }
  uint8_t falldownTimeInterval(uint8_t t) { return mFalldownTimeInterval = t;  }

  uint8_t falldownHeight() const          { return mFalldownHeight;  }
  uint8_t falldownHeight(uint8_t h)       { return mFalldownHeight = h;  }

  uint8_t barsCount() const               { return mBarsCount;  }

  uint32_t backgroundClr() const          { return mBackgroundClr;  }
  uint32_t backgroundClr(uint32_t c)      { return mBackgroundClr = c;  }

  uint16_t width() const                  { return mWidth;  }
  uint16_t height() const                 { return mHeight;  }

  SpectrumLCDDisplay()
   : TFT_eSPI()
   , mBackgroundClr(TFT_BLACK)
  {  }
};

#endif
