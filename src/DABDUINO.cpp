/*
 *  DABDUINO.cpp - Library for DABDUINO - DAB/DAB+ digital radio shield for Arduino.
 *  Created by Tomas Urbanek, Montyho technology Ltd., Januar 2, 2017.
 *  www.dabduino.com
 *  @license  BSD (see license.txt)
 */

#include "DABDUINO.h"

DABDUINO::DABDUINO(HardwareSerial& serial, int8_t RESET_PIN, int8_t DAC_MUTE_PIN, int8_t SPI_CS_PIN) : _s(serial) {

  _Serial = &serial;
  resetPin = RESET_PIN;
  dacMutePin = DAC_MUTE_PIN;
  spiCsPin = SPI_CS_PIN;

}

/* 
 * GPIO functions
 */
enum {
  DATA_IN,
  DATA_OUT,
  MUTE,
  I2S_DATA_IN,
  I2S_LRCLK_IN,
  I2S_SSCLK_IN,
  I2S_BCLK_IN,
  I2S_DATA_OUT,
  I2S_LRCLK_OUT,
  I2S_SSCLK_OUT,
  I2S_BCLK_OUT,
  SPDIF_OUT,
  SPI_DO,
  SPI_CLK,
  SPI_CS,
  SPI_DI
}

/* 
 * Driving strength on GPIO pins
 */
enum {
  STRENGTH_2MA,
  STRENGTH_4MA,
  STRENGTH_6MA,
  STRENGTH_8MA
}

/*
 * Convert two byte char from DAB to one byte char. Add next chars...
 */
byte DABDUINO::charToAscii(byte byte1, byte byte0) {

  if (byte1 == 0x00) {

    if (byte0 == 0x0) {
      return (byte0);
    }

    if (byte0 < 128) {
      return (byte0);
    }

    switch (byte0)
    {
    case 0x8A: return (0x53); break;
    case 0x8C: return (0x53); break;
    case 0x8D: return (0x54); break;
    case 0x8E: return (0x5a); break;
    case 0x8F: return (0x5a); break;
    case 0x9A: return (0x73); break;
    case 0x9D: return (0x74); break;
    case 0x9E: return (0x7a); break;
    case 0xC0: return (0x41); break;
    case 0xC1: return (0x41); break;
    case 0xC2: return (0x41); break;
    case 0xC3: return (0x41); break;
    case 0xC4: return (0x41); break;
    case 0xC5: return (0x41); break;
    case 0xC7: return (0x43); break;
    case 0xC8: return (0x45); break;
    case 0xC9: return (0x45); break;
    case 0xCA: return (0x45); break;
    case 0xCB: return (0x45); break;
    case 0xCC: return (0x49); break;
    case 0xCD: return (0x49); break;
    case 0xCE: return (0x49); break;
    case 0xCF: return (0x49); break;
    case 0xD0: return (0x44); break;
    case 0xD1: return (0x4e); break;
    case 0xD2: return (0x4f); break;
    case 0xD3: return (0x4f); break;
    case 0xD4: return (0x4f); break;
    case 0xD5: return (0x4f); break;
    case 0xD6: return (0x4f); break;
    case 0xD8: return (0x4f); break;
    case 0xD9: return (0x55); break;
    case 0xDA: return (0x55); break;
    case 0xDB: return (0x55); break;
    case 0xDC: return (0x55); break;
    case 0xDD: return (0x59); break;
    case 0xE0: return (0x61); break;
    case 0xE1: return (0x61); break;
    case 0xE2: return (0x61); break;
    case 0xE3: return (0x61); break;
    case 0xE4: return (0x61); break;
    case 0xE5: return (0x61); break;
    case 0xE7: return (0x63); break;
    case 0xE8: return (0x65); break;
    case 0xE9: return (0x65); break;
    case 0xEA: return (0x65); break;
    case 0xEB: return (0x65); break;
    case 0xEC: return (0x69); break;
    case 0xED: return (0x69); break;
    case 0xEE: return (0x69); break;
    case 0xEF: return (0x69); break;
    case 0xF1: return (0x6e); break;
    case 0xF2: return (0x6f); break;
    case 0xF3: return (0x6f); break;
    case 0xF4: return (0x6f); break;
    case 0xF5: return (0x6f); break;
    case 0xF6: return (0x6f); break;
    case 0xF9: return (0x75); break;
    case 0xFA: return (0x75); break;
    case 0xFB: return (0x75); break;
    case 0xFC: return (0x75); break;
    case 0xFD: return (0x79); break;
    case 0xFF: return (0x79); break;
    }
  }

  if (byte1 == 0x01) {
    switch (byte0)
    {
    case 0x1B: return (0x65); break; // ě > e
    case 0x48: return (0x6e); break; // ň > n
    case 0x59: return (0x72); break; // ř > r
    case 0x0D: return (0x63); break; // č > c
    case 0x7E: return (0x7A); break; // ž > z
    case 0x0C: return (0x43); break; // Č > C
    }
  }

  return  (0x20);
}

void DABDUINO::init() {

  // DAC MUTE
  pinMode(dacMutePin, OUTPUT);
  digitalWrite(dacMutePin, HIGH);

  // SPI CS
  pinMode(spiCsPin, OUTPUT);
  digitalWrite(spiCsPin, LOW);

  // DAB module SERIAL
  _Serial->begin(57600);
  _Serial->setTimeout(50);

  // DAB module RESET
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, LOW);
  delay(100);
  digitalWrite(resetPin, HIGH);
  delay(1000);

  while (!isReady()) {
    delay(100);
  }
}

int8_t DABDUINO::isEvent() {
  return _Serial->available();
}

/*
 * Read event
 * Type 0x07: NOTIFY events
 * Event 0x00: CMD_NOTIFY_SCAN_TERMINATE
 * Event 0x01: CMD_NOTIFY_NEW_PROGRAM_TEXT
 * Event 0x02: CMD_NOTIFY_DAB_RECONFIG
 * Event 0x03: CMD_NOTIFY_DAB_SORTING_CHANGE
 * Event 0x04: CMD_NOTIFY_RDS_RAW_DATA
 * Event 0x05: CMD_NOTIFY_NEW_DLS_CMD
 * Event 0x06: CMD_NOTIFY_SCAN_FREQUENCY
 * 
 */
int8_t DABDUINO::readEvent() {
  byte eventData[16];
  byte dabReturn[6];
  byte isPacketCompleted = 0;
  uint16_t byteIndex = 0;
  uint16_t dataIndex = 0;
  byte serialData = 0;
  uint8_t eventDataSize = 128;
  unsigned long endMillis = millis() + 200; // timeout for answer from module = 200ms
  while (millis() < endMillis && dataIndex < DAB_MAX_DATA_LENGTH) {
    if (_Serial->available() > 0) {
      serialData = _Serial->read();
      if (serialData == 0xFE) {
        byteIndex = 0;
        dataIndex = 0;
      }
      if (eventDataSize && dataIndex < eventDataSize) {
        eventData[dataIndex++] = serialData;
      }
      if (byteIndex <= 5) {
        dabReturn[byteIndex] = serialData;
      }
      if (byteIndex == 5) {
        eventDataSize = (((long)dabReturn[4] << 8) + (long)dabReturn[5]);
      }
      if ((int16_t)(byteIndex - eventDataSize) >= 5 && serialData == 0xFD) {
        isPacketCompleted = 1;
        break;
      }
      byteIndex++;
    }
  }
  while (_Serial->available() > 0) {
    _Serial->read();
  }
  if (isPacketCompleted == 1 && dabReturn[1] == 0x07) {
    return dabReturn[2] + 1;
  } else {
    return 0;
  }
}

/*
 *  Send command to DAB module and wait for answer
 */
int8_t DABDUINO::sendCommand(byte dabCommand[], byte dabData[], uint32_t *dabDataSize) {

  byte dabReturn[6];
  byte isPacketCompleted = 0;
  uint16_t byteIndex = 0;
  uint16_t dataIndex = 0;
  byte serialData = 0;
  *dabDataSize = 0;
  while (_Serial->available() > 0) {
    _Serial->read();
  }
  while (byteIndex < 255) {
    if (dabCommand[byteIndex++] == 0xFD) break;
  }
  _Serial->write(dabCommand, byteIndex);
  _Serial->flush();
  byteIndex = 0;
  unsigned long endMillis = millis() + 200; // timeout for answer from module = 200ms
  while (millis() < endMillis && dataIndex < DAB_MAX_DATA_LENGTH) {
    if (_Serial->available() > 0) {
      serialData = _Serial->read();
      if (serialData == 0xFE) {
        byteIndex = 0;
        dataIndex = 0;
      }
      if (*dabDataSize && dataIndex < *dabDataSize) {
        dabData[dataIndex++] = serialData;
      }
      if (byteIndex <= 5) {
        dabReturn[byteIndex] = serialData;
      }
      if (byteIndex == 5) {
        *dabDataSize = (((long)dabReturn[4] << 8) + (long)dabReturn[5]);
      }
      if ((int16_t)(byteIndex - *dabDataSize) >= 5 && serialData == 0xFD) {
        isPacketCompleted = 1;
        break;
      }
      byteIndex++;
    }
  }
  if (isPacketCompleted == 1 && !(dabReturn[1] == 0x00 && dabReturn[2] == 0x02)) {
    return 1;
  } else {
    return 0;
  }
}

// *************************
// ***** SYSTEM ***********
// *************************

/*
 *   Reset DAB module only
 *   SYSTEM_Reset 0x01
 */
int8_t DABDUINO::reset() {
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x00, 0x01, 0x00, 0x00, 0x01, 0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    init();
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Clean DAB module database and reset module
 *   SYSTEM_Reset 0x01
 */
int8_t DABDUINO::resetCleanDB() {
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x00, 0x01, 0x00, 0x00, 0x01, 1, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    init();
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Clean DAB module database only
 *   SYSTEM_Reset 0x01
 */
int8_t DABDUINO::clearDB() {
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x00, 0x01, 0x00, 0x00, 0x01, 2, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
  *   Test for DAB module is ready for communication
  *   SYSTEM_GetSysRdy 0x00
 */
int8_t DABDUINO::isReady() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

// SYSTEM_GetMCUVersion 0x02

// SYSTEM_GetBootVersion 0x03

// SYSTEM_GetASPVersion 0x04

// SYSTEM_GetAllVersion 0x05

// SYSTEM_GetModuleVersion 0x06


// *************************
// ***** STREAM ************
// *************************

/*
 *   Play DAB program
 *   programIndex = 1..9999999 (see programs index)
 *   Command 0x00: STREAM_Play
 */
int8_t DABDUINO::playDAB(uint32_t programIndex) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabCommand[12] = { 0xFE, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Play FM program
 *   frequency = 87500..108000 (MHz)
 *   Command 0x00: STREAM_Play
 */
int8_t DABDUINO::playFM(uint32_t frequency) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte Byte0 = ((frequency >> 0) & 0xFF);
  byte Byte1 = ((frequency >> 8) & 0xFF);
  byte Byte2 = ((frequency >> 16) & 0xFF);
  byte Byte3 = ((frequency >> 24) & 0xFF);
  byte dabCommand[12] = { 0xFE, 0x01, 0x00, 0x00, 0x00, 0x05, 0x01, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

int8_t DABDUINO::playBEEP(uint32_t frequency = 503, uint32_t beepTime = 500, uint32_t silentTime = 200) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte freqByte0 = ((frequency >> 0) & 0xFF);
  byte freqByte1 = ((frequency >> 8) & 0xFF);
  byte freqByte2 = ((frequency >> 16) & 0xFF);
  byte freqByte3 = ((frequency >> 24) & 0xFF);
  byte btByte0 = ((beepTime >> 0) & 0xFF);
  byte btByte1 = ((beepTime >> 8) & 0xFF);
  byte btByte2 = ((beepTime >> 16) & 0xFF);
  byte btByte3 = ((beepTime >> 24) & 0xFF);
  byte stByte0 = ((silentTime >> 0) & 0xFF);
  byte stByte1 = ((silentTime >> 8) & 0xFF);
  byte stByte2 = ((silentTime >> 16) & 0xFF);
  byte stByte3 = ((silentTime >> 24) & 0xFF);
  byte dabCommand[16] = { 0xFE, 0x01, 0x00, 0x00, 0x00, 0x05, 0x03,
    freqByte3, freqByte2, freqByte1, freqByte0, 
    btByte3, btByte2, btByte1, btByte0,
    stByte3, stByte2, stByte1, stByte0,
    0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Play STOP
 *   Command 0x02: STREAM_Stop
 */
int8_t DABDUINO::playSTOP() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x02, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Search DAB bands for programs
 * zone: 1=BAND-3, 2=CHINA-BAND, 3=L-BAND
 * Command 0x04: STREAM_AutoSearch
 */

int8_t DABDUINO::searchDAB(uint32_t band = 1) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;

  byte chStart = 0;
  byte chEnd = 40;

  switch (band) {
  case 1:
    chStart = 0;
    chEnd = 40;
    break;
  case 2:
    chStart = 41;
    chEnd = 71;
    break;
  case 3:
    chStart = 72;
    chEnd = 94;
    break;
  }

  byte dabCommand[9] = { 0xFE, 0x01, 0x04, 0x00, 0x00, 0x02, chStart, chEnd, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Seek FM program - searchDirection: 0=backward, 1=forward
 * Command 0x03: STREAM_Search
 */
int8_t DABDUINO::searchFM(uint32_t searchDirection) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  if (searchDirection < 0) searchDirection = 0;
  if (searchDirection > 1) searchDirection = 1;
  byte dabCommand[8] = { 0xFE, 0x01, 0x03, 0x00, 0x00, 0x01, searchDirection, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Radio module play status
 *   return data: 0=playing, 1=searching, 2=tuning, 3=stop
 *   Change event bitmask.
 *   BIT0: Program name change
 *   BIT1: Program text change
 *   BIT2: DLS command change
 *   BIT3: Stereo mode change
 *   BIT4: Service update
 *   BIT5: Sort change
 *   BIT6: Frequency change
 *   BIT7: Time change
 *   Command 0x10:STREAM_GetPlayStatus
 */
int8_t DABDUINO::playStatus(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x10, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Radio module play mode
 *   return data: 0=DAB, 1=FM, 2=BEEP, 255=Stream stop
 *   Command 0x11: STREAM_GetPlayMode
 */
int8_t DABDUINO::playMode(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x11, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      if (dabData[0] != 0xFF) {
        *data = (uint32_t)dabData[0];
        return 1;
      } else {
        *data = (uint32_t)dabData[0]; // huh ???
        return 1;
      }
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 * Get DAB station index, get tuned FM station frequency
 * Command 0x12: STREAM_GetPlayIndex
 */
int8_t DABDUINO::getPlayIndex(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x12, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize == 4) {
      *data = (((long)dabData[0] << 24) + ((long)dabData[1] << 16) + ((long)dabData[2] << 8) + (long)dabData[3]);
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Get signal strength
 * DAB: signalStrength=0..18, bitErrorRate=
 * FM: signalStrength=0..100
 * Command 0x15: STREAM_GetSignalStrength
 */
int8_t DABDUINO::getSignalStrength(uint32_t *signalStrength, uint32_t *bitErrorRate) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x15, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    *signalStrength = (uint32_t)dabData[0];
    *bitErrorRate = 0;
    if (dabDataSize > 1) {
      *bitErrorRate =  (((long)dabData[1] << 8) + (long)dabData[2]);
    }
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Set stereo mode
 *   true=stereo, false=force mono
 *   Command 0x20: STREAM_SetStereoMode
 */
int8_t DABDUINO::setStereoMode(boolean stereo = true) {

  int32_t value = 0;
  if (stereo == true) {
    value = 1;
  }
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x20, 0x00, 0x00, 0x01, value, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Get stereo mode
 *   0=force mono, 1=auto detect stereo
 *   Command 0x21: STREAM_GetStereoMode
 */
int8_t DABDUINO::getStereoMode(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x21, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Get stereo type
 *   return data: 0=stereo, 1=joint stereo, 2=dual channel, 3=single channel (mono)
 *   Command 0x16: STREAM_GetStereo
 */
int8_t DABDUINO::getStereoType(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x16, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Set volume
 *   volumeLevel = 0..16
 *   Command 0x22: STREAM_SetVolume
 */
int8_t DABDUINO::setVolume(uint32_t volumeLevel) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  if (volumeLevel < 0) volumeLevel = 0;
  if (volumeLevel > 16)  volumeLevel = 16;
  byte dabCommand[8] = { 0xFE, 0x01, 0x22, 0x00, 0x00, 0x01, volumeLevel, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Get volume
 *   return set volumeLevel: 0..16
 *   Command 0x23: STREAM_GetVolume
 */
int8_t DABDUINO::getVolume(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x23, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Get program type
 *   0=N/A, 1=News, 2=Curent Affairs, 3=Information, 4=Sport, 5=Education, 6=Drama, 7=Arts, 8=Science, 9=Talk, 10=Pop music, 11=Rock music, 12=Easy listening, 13=Light Classical, 14=Classical music, 15=Other music, 16=Weather, 17=Finance, 18=Children's, 19=Factual, 20=Religion, 21=Phone in, 22=Travel, 23=Leisure, 24=Jazz & Blues, 25=Country music, 26=National music, 27=Oldies music, 28=Folk Music, 29=Documentary, 30=undefined, 31=undefined
 *   Command 0x2C: STREAM_GetProgrameType
 */
int8_t DABDUINO::getProgramType(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x2C, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 * Get DAB station short name
 * Command 0x2D: STREAM_GetProgrameName
 */
int8_t DABDUINO::getProgramName(uint32_t programIndex, char text[]) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[12] = { 0xFE, 0x01, 0x2D, 0x00, 0x00, 0x05, Byte3, Byte2, Byte1, Byte0, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    uint32_t j = 0;
    for (uint32_t i = 0; i < dabDataSize; i = i + 2) {
      text[j++] = (char)charToAscii(dabData[i], dabData[i + 1]);
    }
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get DAB text event
 * return: 1=new text, 2=text is same, 3=no text
 * dabText: text
 * 0x2E: STREAM_GetProgrameText
 */
int8_t DABDUINO::getProgramText(char text[]) {

  char textLast[DAB_MAX_TEXT_LENGTH];
  memcpy(textLast, text, sizeof(text[0])*DAB_MAX_TEXT_LENGTH);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x2E, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize == 1) {
      text[0] = dabData[0]; // 0=There is no text in programe, 1=The program text received, but no text available
      text[1] = 0x00;
      return 3; // No error, but no text
    }
    int32_t j = 0;
    for (uint32_t i = 0; i < dabDataSize; i = i + 2) {
      text[j++] = (char)charToAscii(dabData[i], dabData[i + 1]);
    }
    for (uint32_t i = 0; i < j; i++) {
      if (text[i] != textLast[i]) {
        return 1; // New dab text
      }
    }
    return 2; // Same dab text
  } else {
    return 0;
  }
}

/*
 *   Get sampling rate (DAB/FM)
 *   return data: 1=32kHz, 2=24kHz, 3=48kHz
 *   Command 0x18: STREAM_GetSamplingRate
 */
int8_t DABDUINO::getSamplingRate(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x18, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Get data rate (DAB)
 *   return data: data rate in kbps
 *   Command 0x47: STREAM_GetDataRate
 */
int8_t DABDUINO::getDataRate(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x47, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (((long)dabData[0] << 8) + (long)dabData[1]);
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Get DAB signal quality
 *   return: 0..100
 *   0..19 = playback stop
 *   20..30 = the noise (short break) appears
 *   100 = the bit error rate is 0
 *   Command 0x48: STREAM_GetSignalQuality
 */
int8_t DABDUINO::getSignalQuality(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x48, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 *   Get DAB frequency for program index
 *   return: frequency index
 *   0=174.928MHz, 1=176.64, 2=178.352,...
 *
 *  // TODO: add conversion table for index2freqency
 *  Command 0x46: STREAM_GetFrequency
 */
int8_t DABDUINO::getFrequency(uint32_t programIndex, uint32_t *data) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[11] = { 0xFE, 0x01, 0x46, 0x00, 0x00, 0x04, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 * Get DAB program ensemble name
 * Command 0x41: STREAM_GetEnsembleName
 */
int8_t DABDUINO::getEnsembleName(uint32_t programIndex, char text[]) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[12] = { 0xFE, 0x01, 0x41, 0x00, 0x00, 0x05, Byte3, Byte2, Byte1, Byte0, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    uint32_t j = 0;
    for (uint32_t i = 0; i < dabDataSize; i = i + 2) {
      text[j++] = (char)charToAscii(dabData[i], dabData[i + 1]);
    }
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get DAB stations index (number of programs in database)
 * Command 0x13: STREAM_GetTotalProgram
 */
int8_t DABDUINO::getProgramIndex(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x13, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize == 4) {
      *data = (((long)dabData[0] << 24) + ((long)dabData[1] << 16) + ((long)dabData[2] << 8) + (long)dabData[3]);
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 *   Test DAB program is active (on-air)
 *   return: 0=off-air, 1=on-air
 *   Command 0x44: STREAM_IsActive
 */
int8_t DABDUINO::isProgramOnAir(uint32_t programIndex) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[11] = { 0xFE, 0x01, 0x44, 0x00, 0x00, 0x04, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      return (uint32_t)dabData[0];
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

/*
 * Get DAB program service short name
 * Command 0x42: STREAM_GetServiceName
 */
int8_t DABDUINO::getServiceName(uint32_t programIndex, char text[]) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[12] = { 0xFE, 0x01, 0x42, 0x00, 0x00, 0x05, Byte3, Byte2, Byte1, Byte0, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    uint32_t j = 0;
    for (uint32_t i = 0; i < dabDataSize; i = i + 2) {
      text[j++] = (char)charToAscii(dabData[i], dabData[i + 1]);
    }
    return 1;
  } else {
    return 0;
  }
}


/*
 * Get DAB search index (number of programs found in search process)
 * Command 0x14: STREAM_GetSearchProgram
 */
int8_t DABDUINO::getSearchIndex(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x14, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}


/*
 * Get DAB program service component type (ASCTy)
 * return data: 0=DAB, 1=DAB+, 2=Packet data, 3=DMB (stream data)
 * Command 0x40: STREAM_GetServCompType
 */
int8_t DABDUINO::getServCompType(uint32_t programIndex, uint32_t *data) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[11] = { 0xFE, 0x01, 0x40, 0x00, 0x00, 0x04, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 *   Set preset
 *   programIndex = DAB: programIndex, FM: frequency
 *   presetIndex = 0..9
 *   presetMode = 0=DAB, 1=FM
 *   Command 0x24: STREAM_SetPreset
 */
int8_t DABDUINO::setPreset(uint32_t programIndex, uint32_t presetIndex, uint32_t presetMode) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[13] = { 0xFE, 0x01, 0x24, 0x00, 0x00, 0x06, presetMode, presetIndex, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *  Get preset
 *  presetIndex = 0..9
 *  presetMode = 0=DAB, 1=FM
 *  Command 0x25: STREAM_GetPreset
 */
int8_t DABDUINO::getPreset(uint32_t presetIndex, uint32_t presetMode, uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[9] = { 0xFE, 0x01, 0x25, 0x00, 0x00, 0x02, presetMode, presetIndex, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if(dabDataSize) {
      *data = (((long)dabData[0] << 24) + ((long)dabData[1] << 16) + ((long)dabData[2] << 8) + (long)dabData[3]);
    }
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get program info
 * return serviceId = service id of DAB program
 * return ensembleId = ensemble id of DAB program
 * Command 0x49: STREAM_ProgrameInfo
 */
int8_t DABDUINO::getProgramInfo(uint32_t programIndex, uint32_t *serviceId, uint32_t *ensembleId) {

  byte Byte0 = ((programIndex >> 0) & 0xFF);
  byte Byte1 = ((programIndex >> 8) & 0xFF);
  byte Byte2 = ((programIndex >> 16) & 0xFF);
  byte Byte3 = ((programIndex >> 24) & 0xFF);
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[11] = { 0xFE, 0x01, 0x49, 0x00, 0x00, 0x04, Byte3, Byte2, Byte1, Byte0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *serviceId = (((long)dabData[0] << 24) + ((long)dabData[1] << 16) + ((long)dabData[2] << 8) + (long)dabData[3]);
      *ensembleId = (((long)dabData[4] << 8) + (long)dabData[5]);
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Get program sorter
 * return data = 0=sort by ensembleID, 1=sort by service name, 2=sort by active and inactive program
 * Command 0x2B: STREAM_GetSorter
 */
int8_t DABDUINO::getProgramSorter(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x2B, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Set program sorter
 * sortMethod = 0=sort by ensembleID, 1=sort by service name, 2=sort by active and inactive program
 * Command 0x2A: STREAM_SetSorter
 */
int8_t DABDUINO::setProgramSorter(uint32_t sortMethod) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x2A, 0x00, 0x00, 0x01, sortMethod, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get DRC
 * return data = 0=DRC off, 1=DRC low, 2=DRC high
 * Command 0x4D: STREAM_GetDrc
 */
int8_t DABDUINO::getDRC(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x4D, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Set DRC
 * setDRC = 0=DRC off, 1=DRC low, 2=DRC high
 * Command 0x4C: STREAM_SetDrc
 */
int8_t DABDUINO::setDRC(uint32_t setDRC) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x4C, 0x00, 0x00, 0x01, setDRC, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}


/*
 * Prune programs - delete inactive programs (!on-air)
 * Command 0x45: STREAM_PruneStation
 */
int8_t DABDUINO::prunePrograms(uint32_t *prunedTotalPrograms, uint32_t *prunedProgramIndex) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x45, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *prunedTotalPrograms = (((long)dabData[0] << 8) + (long)dabData[1]);
      *prunedProgramIndex = (((long)dabData[2] << 8) + (long)dabData[3]);
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Get ECC
 * return ECC (Extended Country Code)
 * return countryId (Country identification)
 * Command 0x4B: STREAM_GetECC
 */
int8_t DABDUINO::getECC(uint32_t *ECC, uint32_t *countryId) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x4B, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *ECC = (uint32_t)dabData[0];
      *countryId = (uint32_t)dabData[1];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Get FM RDS PI code
 * return PI code
 * Command 0x68: STREAM_GetRdsPIcode
 */
int8_t DABDUINO::getRdsPIcode(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x68, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (((long)dabData[0] << 8) + (long)dabData[1]);
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Set FMstereoThdLevel
 * RSSItresholdLevel = 0..10
 * Command 0x66: STREAM_SetFMSeekNoiseThd
 */
int8_t DABDUINO::setFMstereoThdLevel(uint32_t RSSItresholdLevel) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x66, 0x00, 0x00, 0x01, RSSItresholdLevel, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get FMstereoThdLevel
 * data return = 0..10
 * 0x67: STREAM_GetFMSeekNoiseThd
 */
int8_t DABDUINO::getFMstereoThdLevel(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x67, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}


/*
 * Set FMseekTreshold
 * RSSItreshold = 0..100
 * Command 0x64: STREAM_SetFMSeekRSSIThd
 */
int8_t DABDUINO::setFMseekTreshold(uint32_t RSSItreshold) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x64, 0x00, 0x00, 0x01, RSSItreshold, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get FMseekTreshold
 * data return = 0..100
 * Command 0x65: STREAM_GetFMSeekRSSIThd
 */
int8_t DABDUINO::getFMseekTreshold(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x65, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Set FMstereoTreshold
 * Set FM noice threshold value for stereo switching
 * RSSIthreshold = 0..10
 * Command 0x62: STREAM_SetFMStereoNoiseThd
 */
int8_t DABDUINO::setFMstereoTreshold(uint32_t RSSIstereoTreshold) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x01, 0x62, 0x00, 0x00, 0x01, RSSIstereoTreshold, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get FMstereoTreshold
 * data return = 0..10
 * Command 0x63: STREAM_GetFMStereoNoiseThd
 */
int8_t DABDUINO::getFMstereoTreshold(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x01, 0x63, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}


// *************************
// ***** RTC ***************
// * command type 0x02 RTC *
// *************************

/*
 * Set RTC clock
 * year: 2017=17,2018=18, month: 1..12, day: 1..31, hour: 0..23, minute: 0..59, second: 0..59 
 * Command 0x00: RTC_SetClock
 */
int8_t DABDUINO::setRTCclock(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[14] = { 0xFE, 0x02, 0x00, 0x00, 0x00, 0x07, second, minute, hour, day, 0x00, month, year, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get RTC ckock
 * year: 2017=17,2018=18, month: 1..12, week: 0(sat)..6(fri), day: 1..31, hour: 0..23, minute: 0..59, second: 0..59 
 * Command 0x01: RTC_GetClock
 */
int8_t DABDUINO::getRTCclock(uint32_t *year, uint32_t *month, uint32_t *week, uint32_t *day, uint32_t *hour, uint32_t *minute, uint32_t *second) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x02, 0x01, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *second = (uint32_t)dabData[0];
      *minute = (uint32_t)dabData[1];
      *hour = (uint32_t)dabData[2];
      *day = (uint32_t)dabData[3];
      *week = (uint32_t)dabData[4];
      *month = (uint32_t)dabData[5];
      *year = (uint32_t)dabData[6];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Set RTC sync clock from stream enable
 * 0x02: RTC_EnableSyncClock
 */
int8_t DABDUINO::RTCsyncEnable() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x02, 0x02, 0x00, 0x00, 0x01, 1, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *  Set RTC sync clock from stream disable
 */
int8_t DABDUINO::RTCsyncDisable() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[8] = { 0xFE, 0x02, 0x02, 0x00, 0x00, 0x01, 0, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get RTC sync clock status
 * return data: 0=disable, 1=enable 
 * Command 0x03: RTC_GetSyncClockStatus
 */
int8_t DABDUINO::getRTCsyncStatus(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x02, 0x03, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

/*
 * Get RTC clock status
 * return data: 0=unset, 1=set 
 * Command 0x04: RTC_GetClockStatus
 */
int8_t DABDUINO::getRTCclockStatus(uint32_t *data) {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[7] = { 0xFE, 0x02, 0x04, 0x00, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    if (dabDataSize) {
      *data = (uint32_t)dabData[0];
      return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

// ********************************************
// ***** MOT (Multimedia Object Transfer) *****
// ********************************************




// *************************
// ***** NOTIFY ************
// command type 0x07 NOTIFY
// *************************

/*
 *   Enable event notification
 */
int8_t DABDUINO::eventNotificationEnable() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[9] = { 0xFE, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x7F, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 *   Disable event notification
 */
int8_t DABDUINO::eventNotificationDisable() {

  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[9] = { 0xFE, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}


// *************************
// ***** GPIO **************
// command type 0x08 GPIO
// *************************

/* 
 * set GPIO pin function
 * drivingStrength: 0: 2mA, 1: 4mA, 2: 6mA, 3: 8mA 
 */
int8_t DABDUINO::setGPIO(uint32_t gpioIndex, uint32_t gpioFunction, uint32_t drivingStrength = STRENGTH_2MA) {
  byte dabData[DAB_MAX_DATA_LENGTH];
  uint32_t dabDataSize;
  byte dabCommand[10] = { 0xFE, 0x08, 0x00, 0x00, 0x00, 0x03, gpioIndex, gpioFunction, drivingStrength, 0xFD };
  if (sendCommand(dabCommand, dabData, &dabDataSize)) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Set i2s mode
 * true=on, false=off
 * GPIO pins are hardcoded
 * I2S_DATA_OUT pin 43
 * I2S_LRCLK_OUT pin 55
 * I2S_SSCLK_OUT pin 54
 * I2S_BCLK_OUT pin 53
 */
int8_t DABDUINO::setI2sMode(boolean toggle = true) {

  int32_t gpioFunction = MUTE;
// set our I2S_DATA_OUT pin
  if (toggle == true) {
    gpioFunction = I2S_DATA_OUT;
  }
  setGPIO(0x2B, gpioFunction, STRENGTH_2MA); // 0x2B = GPIO 43
// set our I2S_LRCLK_OUT pin
  if (toggle == true) {
    gpioFunction = I2S_LRCLK_OUT;
  }
  setGPIO(0x37, gpioFunction, STRENGTH_2MA); // 0x37 = GPIO 55
// set our I2S_SSCLK_OUT pin
  if (toggle == true) {
    gpioFunction = I2S_SSCLK_OUT;
  }
  setGPIO(0x36, gpioFunction, STRENGTH_2MA); // 0x36 = GPIO 54
// set our I2S_BCLK_OUT pin
  if (toggle == true) {
    gpioFunction = I2S_BCLK_OUT;
  }
  setGPIO(0x35, gpioFunction, STRENGTH_2MA); // 0x35 = GPIO 53

  return 0;
}




