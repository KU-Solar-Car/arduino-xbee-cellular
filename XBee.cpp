/* Copyright (c) 2021 Andrew Riachi. All rights reserved.*/

#include "XBee.h"
#include "Frames.h"
#include "IPAddress.h"
#include <Arduino.h>


bool XBee::configure()
{
  m_serial.write("+++");
  delay(1500);
  String ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("ATAP 1\r");
  delay(2000);
  ok = "";
  ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("CN\r");
  return true;
}

void XBee::sendFrame(const byte& frameType, const char frameData[], size_t frameDataLen)
{
  uint16_t checksum = frameType;
  uint16_t frameLength = frameDataLen + 1; // length of frameData + length of frameType
  for (uint16_t i = 0; i < frameDataLen; i++)
    checksum += frameData[i];
  checksum = 0xFF - checksum;

  const uint8_t start = 0x7E;
  byte len_msb = (uint8_t) (frameLength >> 8);
  byte len_lsb = (uint8_t) frameLength;

  Serial.print("TX: ");
  m_serial.write(start);
  m_serial.write(len_msb);
  m_serial.write(len_lsb);
  m_serial.write(frameType);
  m_serial.write(frameData, frameDataLen);
  m_serial.write(checksum);
  Serial.println("");
}

void XBee::sendATCommand(uint8_t frameID, const char command[], const char param[], size_t paramLen)
{
  char* frameData = new char[3 + paramLen];
  frameData[0] = frameID;
  memcpy(frameData+1, command, 2);
  memcpy(frameData+3, param, paramLen);
  sendFrame(0x08, frameData, 3+paramLen);
  delete[] frameData;
}

void XBee::safeReset(unsigned timeout)
{
  if (isShutDown(timeout))
  {
    sendATCommand(1, "FR", '\0', 0);
  }
  else
  {
    shutdown(timeout, true);
  }
  const unsigned long myTime = millis();
  userFrame resp;
  do
  {
    resp = read();
    if (resp.frameType == 0x8A && resp.frameData[0] == 0x02)
      return;
  } while (millis() < myTime+timeout);
  Serial.println("Timed out.");
}

void XBee::shutdown(unsigned int timeout, bool reboot)
{
  uint8_t param;
  if (reboot)
  {
    param = 0x01;
  }
  else
  {
    param = 0x00;
  }
  sendATCommand(1, "SD", (char*) &param, 1);
  unsigned int startTime = millis();
  userFrame response;
  do
  {
    response = read();
    if (response.frameType == 0x88 && strncmp(response.frameData+1, "SD", 2) == 0 && response.frameData[3] == 0x00)
      return;
  } while (millis() < (startTime + timeout));
  Serial.println("Timed Out.");
  
}

bool XBee::shutdownCommandMode()
{
  m_serial.write("+++");
  delay(1500);
  String ok = m_serial.readStringUntil('\r');
  if (ok != "OK")
    return false;
  m_serial.write("ATSD 0\r");
  m_serial.setTimeout(120000);
  ok = "";
  ok = m_serial.readStringUntil('\r');
  m_serial.setTimeout(1000);
  if (ok != "OK")
    return false;
  m_serial.write("CN\r");
  return true;
}

userFrame XBee::read()
{
  int recvd;
  const unsigned timeout = 3000;
  m_rxBuffer.clear();
  // Serial.println("Bytes in buffer: " + String(m_serial.available()));
  do
  {
    recvd = m_serial.read();
    if (recvd == -1) 
    {
      // Serial.println("Start delimiter not found.");
      return NULL_USER_FRAME; 
    }
  } while (recvd != 0x7E);
  Serial.println("Got start delimiter 0x7E");
  unsigned long lastRead = millis();
  
  while (m_rxBuffer.bytes_recvd < m_rxBuffer.length())
  {
    int bytes_available = m_serial.available();
    if (bytes_available > 0)
    {
      // Serial.println("Bytes available: " + String(bytes_available));
      char ptr[30];
      // sprintf(ptr, "Writing to address %p", m_rxBuffer.buf+m_rxBuffer.bytes_recvd);
      // Serial.println(ptr);
      m_rxBuffer.buf[m_rxBuffer.bytes_recvd] = m_serial.read();
      m_rxBuffer.bytes_recvd++;
      lastRead = millis();
    }
    else if (millis() > lastRead + timeout)
    {
      Serial.println("Timed out in the middle of receiving a frame.");
      m_rxBuffer.clear();
      return NULL_USER_FRAME;
    }
  }
  Serial.println("Available for write: " + String(Serial.availableForWrite()));
  Serial.println("BEGIN RX");
  
  char len_str[21];
  sprintf(len_str, "Length: %02X %02X (%d)", m_rxBuffer.buf[0], m_rxBuffer.buf[1], m_rxBuffer.frameLength());
  Serial.println(len_str);

  char type_str[15];
  sprintf(type_str, "Frame Type: %02X", m_rxBuffer.buf[2]);
  Serial.println(type_str);
  
  Serial.print("Frame Data: ");
  Serial.write(m_rxBuffer.frameData(), m_rxBuffer.frameDataLength());
  Serial.println();

  Serial.print("Frame Data Hex: ");
  for (int i = 0; i < m_rxBuffer.frameDataLength(); i++)
  {
    char c[1];
    sprintf(c, "%02X ", m_rxBuffer.frameData()[i]);
    Serial.print(c);
  }
  Serial.println();

  char sum_str[13];
  sprintf(sum_str, "Checksum: %02X", m_rxBuffer.buf[m_rxBuffer.length()-1]);
  Serial.println(sum_str);

  Serial.println("END RX");

  return {m_rxBuffer.frameType(), m_rxBuffer.buf+3, m_rxBuffer.frameDataLength()};
}

void XBee::sendTCP(IPAddress address, uint16_t destPort, uint16_t sourcePort, uint8_t protocol, uint8_t options, const char payload[], size_t payloadLength)
{
  size_t bufSize = 11 + payloadLength;
  char* buf = new char[bufSize];
  buf[0] = 0; // frameID = 0
  uint32_t addr_array = address;
  memcpy(buf+1, &addr_array, 4);
  buf[5] = (char) (destPort >> 8); // The arduino is little-endian but tht digi wants big-endian here
  buf[6] = (char) (destPort);
  buf[7] = (char) (sourcePort >> 8);
  buf[8] = (char) (sourcePort);
  buf[9] = protocol;
  buf[10] = options;
  memcpy(buf+11, payload, payloadLength);

  sendFrame(0x20, buf, bufSize);

  delete[] buf;
}

bool XBee::isConnected(unsigned timeout)
{
  sendATCommand(1, "AI", nullptr, 0);
  userFrame resp;
  const unsigned long myTime = millis();
  do
  {
    resp = read();
    if (resp.frameType == 0x88 && strncmp(resp.frameData+1, "AI", 2) == 0)
    {
      // Serial.print("Got here: ");
      // Serial.write(resp.frameData+1, 2);
      return (resp.frameData[4] == 0x00);
    }
  } while (millis() < myTime+timeout);
  Serial.println("Connectivity check timed out");
  return false;
}

bool XBee::isShutDown(unsigned timeout)
{
  sendATCommand(1, "AI", nullptr, 0);
  userFrame resp;
  const unsigned long myTime = millis();
  do
  {
    resp = read();
    if (resp.frameType == 0x88 && strncmp(resp.frameData+1, "AI", 2) == 0)
    {
      // Serial.print("Got here: ");
      // Serial.write(resp.frameData+1, 2);
      return (resp.frameData[4] == 0x2D);
    }
  } while (millis() < myTime+timeout);
  Serial.println("Connectivity check timed out");
  return false;
}
