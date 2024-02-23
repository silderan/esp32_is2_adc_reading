#include "SpectrumBarsDisplay.h"

void SpectrumLCDDisplay::init(uint8_t barsCount, uint16_t width, uint16_t height, uint8_t falldownTimeInterval, uint8_t falldownHeight)
{
  TFT_eSPI::init();
  setRotation(1);

  fillScreen(mBackgroundClr);

  mBarsCount = barsCount;
  mWidth = width;
  mHeight = height;
  mFalldownTimeInterval = falldownTimeInterval;
  mFalldownHeight = falldownHeight;
  mBarWidth = mWidth / mBarsCount;
  mBarOldValues.resize(mBarsCount);
  Serial.printf("LCD Display configured to hold %d bars in a %dx%d pixels screen.\n", mBarsCount, mWidth, mHeight);
}

void SpectrumLCDDisplay::drawBars(const std::vector<uint32_t> &barValues)
{
  static uint32_t mNextFallTime;
  uint32_t curVal;
  uint8_t doNowSlowFall = mNextFallTime < millis();

  if( doNowSlowFall )
    mNextFallTime = millis() + mFalldownTimeInterval;

  for( uint16_t b = 0; b < mBarsCount; ++b )
  {
    curVal = barValues[b];
//    Serial.print(b);
//    Serial.print("-");
//    Serial.print(curVal);
//    Serial.print(" ");
    if( curVal > mHeight )
      curVal = mHeight;

    // Paint bar.
    if( mBarOldValues[b] < curVal )
    {
//      Serial.print("Paint. ");
      fillRect(b * mBarWidth, mHeight - curVal, mBarWidth, curVal, TFT_BLUE);
      mBarOldValues[b] = curVal;
    }
    else
    // Erase (paint in background color) bar.
    if( doNowSlowFall && mBarOldValues[b] && (mBarOldValues[b] != curVal) )
    {
//      Serial.print("Erase ");
      if( mBarOldValues[b] <= mFalldownHeight )
        mBarOldValues[b] = 0;
      else
        mBarOldValues[b] -= min(mBarOldValues[b] - curVal, uint32_t(mFalldownHeight));
      fillRect(b * mBarWidth, 0, mBarWidth, mHeight - mBarOldValues[b], mBackgroundClr);
    }
//    else
//      Serial.print("Nothing");
  }
//  Serial.println("");
}

void SpectrumLCDDisplay::printDetailedInfo() const
{
  Serial.printf("Display is %dx%d pixel screen.\n", mWidth, mHeight);
  Serial.printf("There are %d bars displaying with %d pixels width each.\n", mBarsCount, mBarWidth);
  Serial.printf("Every %d milliseconds the bars will falldown %d pixels.\n", mFalldownTimeInterval, mFalldownHeight);
}