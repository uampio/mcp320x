/**
 * @file Mcp3208.cpp
 * @author  Patrick Rogalla <patrick@labfruits.com>
 */
#include "Mcp3208.h"

// divide n by d and round to next integer
#define div_round(n,d) (((n) + ((d) >> 2)) / (d))


MCP3208::MCP3208(uint16_t vref, uint8_t csPin, SPIClass *spi) :
    mVref(vref),
    mCsPin(csPin),
    mSplSpeed(0),
    mSpi(spi) {}

MCP3208::MCP3208(uint16_t vref, uint8_t csPin) :
    MCP3208(vref, csPin, &SPI) {}

void MCP3208::calibrate(Channel ch) {
  mSplSpeed = testSplSpeed(ch, 256);
}

uint16_t MCP3208::read(Channel ch) {
  // transfer spi data
  return transfer(createCmd(ch));
}

template <typename T>
void MCP3208::read(Channel ch, T *data, uint16_t num) {
  // create command data
  SpiData cmd = createCmd(ch);

  // transfer spi data
  for (uint16_t i=0; i < num; i++)
    data[i] = static_cast<T>(transfer(cmd));
}

template <typename T>
void MCP3208::read(Channel ch, T *data, uint16_t num, uint32_t splFreq) {
  // create command data
  SpiData cmd = createCmd(ch);
  // required delay
  uint16_t delay = getSplDelay(ch, splFreq);

  // transfer spi data
  for (uint16_t i=0; i < num; i++) {
    data[i] = static_cast<T>(transfer(cmd));
    delayMicroseconds(delay);
  }
}

uint32_t MCP3208::testSplSpeed(Channel ch) {
  return testSplSpeed(ch, 64);
}

uint32_t MCP3208::testSplSpeed(Channel ch, uint16_t num) {
  // start time
  uint32_t t1 = micros();
  // perform sampling
  for (uint16_t i = 0; i < num; i++) read(ch);
  // stop time
  uint32_t t2 = micros();

  // return average sampling speed
  return div_round((t2 - t1) * 1000, num);
}

uint32_t MCP3208::testSplSpeed(Channel ch, uint16_t num, uint32_t splFreq) {
  // required delay
  uint16_t delay = getSplDelay(ch, splFreq);

  // start time
  uint32_t t1 = micros();
  // perform sampling
  for (uint16_t i = 0; i < num; i++) {
    read(ch);
    delayMicroseconds(delay);
  }
  // stop time
  uint32_t t2 = micros();

  // return average sampling speed
  return div_round((t2 - t1) * 1000, num);
}

uint16_t MCP3208::toAnalog(uint16_t raw) {
  return (static_cast<uint32_t>(raw) * mVref) / kRes;
}

uint16_t MCP3208::toDigital(uint16_t val) {
  return (static_cast<uint32_t>(val) * kRes) / mVref;
}

uint16_t MCP3208::getVref() {
  return mVref;
}

uint16_t MCP3208::getAnalogRes() {
  return (static_cast<uint32_t>(mVref) * 1000) / kRes;
}

MCP3208::SpiData MCP3208::createCmd(Channel ch) {
  // base command structure
  // 0b000001cccc000000
  // c: channel config
  return {
    // add channel to basic command structure
    .value = static_cast<uint16_t>((0x0400 | (ch << 6)))
  };
}

uint16_t MCP3208::getSplDelay(Channel ch, uint32_t splFreq) {
  // requested sampling period (ns)
  uint32_t splTime = div_round(1000000000, splFreq);

  // measure speed if uncalibrated
  if (!mSplSpeed) calibrate(ch);

  // calculate delay in us
  int16_t delay =  (splTime - mSplSpeed) / 1000;
  return (delay < 0) ? 0 : static_cast<uint16_t>(delay);
}

uint16_t MCP3208::transfer(SpiData cmd) {

  SpiData adc;

  // activate ADC with chip select
  digitalWrite(mCsPin, LOW);

  // send first command byte
  mSpi->transfer(cmd.hiByte);
  // send second command byte and receive first(msb) 4 bits
  adc.hiByte = mSpi->transfer(cmd.loByte) & 0x0F;
  // receive last(lsb) 8 bits
  adc.loByte = mSpi->transfer(0x00);

  // deactivate ADC with slave select
  digitalWrite(mCsPin, HIGH);

  return adc.value;
}

/*
 * Explicit template instantiation for the supported data array types.
 */
template void MCP3208::read<uint16_t>(Channel, uint16_t*, uint16_t);
template void MCP3208::read<uint32_t>(Channel, uint32_t*, uint16_t);
template void MCP3208::read<float>(Channel, float*, uint16_t);
template void MCP3208::read<double>(Channel, double*, uint16_t);

template void MCP3208::read<uint16_t>(Channel, uint16_t*, uint16_t, uint32_t);
template void MCP3208::read<uint32_t>(Channel, uint32_t*, uint16_t, uint32_t);
template void MCP3208::read<float>(Channel, float*, uint16_t, uint32_t);
template void MCP3208::read<double>(Channel, double*, uint16_t, uint32_t);