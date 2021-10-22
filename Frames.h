/* Copyright (c) 2021 Andrew Riachi. All rights reserved.*/

#ifndef FRAMES_H
#define FRAMES_H

#include <Arduino.h>

struct frame
{
  // const uint16_t MAX_BUFFER_SIZE = 1550;
  static const uint16_t MAX_BUFFER_SIZE{1800};
  char buf[MAX_BUFFER_SIZE]; //0x7E will NOT be in the buffer.
  uint16_t bytes_recvd;
  
  // frame();
  ~frame();
  frame& operator=(const frame& other);

  uint16_t length() const; // the length field of the packet, plus the length of the length field itself, plus 1 byte for checksum
  uint16_t frameLength() const;
  uint8_t frameType() const;
  char* frameData();
  uint16_t frameDataLength() const;
  uint8_t checksum() const;
  void clear();
};

inline uint16_t frame::length() const
{
  return frameLength() + 3;
}

inline uint16_t frame::frameLength() const
{
  return (bytes_recvd >= 2) ? (buf[1] | (buf[0] << 8)) : 0;
}

inline uint16_t frame::frameDataLength() const
{
  return frameLength() - 1;
}

inline uint8_t frame::frameType() const
{
  return buf[2];
}

inline char* frame::frameData()
{
  return buf+3; // +2 to get past length, +1 to get past frameType
}

inline uint8_t frame::checksum() const
{
  return buf[frameLength()];
}

struct userFrame
{
  bool operator==(const userFrame& other);
  
  uint8_t frameType;
  const char* frameData;
  uint16_t frameDataLength;
};

const userFrame NULL_USER_FRAME = {0, nullptr, 0};

#endif
