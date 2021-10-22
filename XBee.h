/* Copyright (c) 2021 Andrew Riachi. All rights reserved.*/

#ifndef XBEE_H
#define XBEE_H

#include <Arduino.h>
#include "Frames.h"
#include "IPAddress.h"

const uint8_t TERMINATE_TCP_SOCKET = 0x02;
const uint8_t PROTOCOL_TLS = 0x04;
const uint8_t PROTOCOL_TCP = 0x01;
const uint16_t PORT_HTTPS = 443;
const uint16_t PORT_HTTP = 80;

class XBee
{
  private:
  Stream& m_serial;
  frame m_rxBuffer;

  public:
  XBee(Stream& serial) : m_serial(serial){}
  bool configure();
  userFrame read(); // Can read one full packet at a time.
  bool shutdownCommandMode();
  void safeReset(unsigned timeout);
  void shutdown(unsigned int timeout, bool reboot);
  void sendFrame(const byte& frameType, const char frameData[], size_t frameDataLen);
  void sendATCommand(uint8_t frameID, const char command[], const char param[], size_t paramLen);
  void sendTCP(IPAddress address, uint16_t destPort, uint16_t sourcePort, uint8_t protocol, uint8_t options, const char payload[], size_t payloadLength);
  bool isShutDown(unsigned timeout);
  bool isConnected(unsigned);
};

#endif
