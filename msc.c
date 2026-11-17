#include "CH559.h"
#include "USBHost.h"
#include "msc.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MSC_DIR_OUT 0
#define MSC_DIR_IN 1
#define CBW_SIGNATURE 0x43425355UL
#define CSW_SIGNATURE 0x53425355UL
#define MSC_REQ_RESET 0xFF
#define MSC_REQ_GET_MAX_LUN 0xFE

extern void printBuf(unsigned char *p, int len);

static unsigned char mscIf;
static unsigned char epIn, epOut;
static unsigned short mpsIn, mpsOut;
static unsigned char toggleIn, toggleOut;
static unsigned char lun = 0;
static unsigned long nextTag = 1;
static unsigned char __xdata cbw[31];
static unsigned char __xdata csw[13];

static void put32le(unsigned char *p, unsigned long v)
{
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
  p[2] = (unsigned char)(v >> 16);
  p[3] = (unsigned char)(v >> 24);
}
static unsigned long get32le(const unsigned char *p)
{
  return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
         ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

void MSC_Bind(const USB_MSC_INFO *info)
{
  mscIf = info->interfaceNumber;
  epIn = info->epInAddress;
  epOut = info->epOutAddress;
  mpsIn = info->epInMaxPacket;
  mpsOut = info->epOutMaxPacket;
  toggleIn = toggleOut = 0;
  lun = 0;
  nextTag = 1;

  DEBUG_OUT("mpsIn=%d,mpsOut=%d\r\n", mpsIn, mpsOut);
}

unsigned char MSC_GetMaxLUN(unsigned char *maxLun)
{
  unsigned char __xdata setup[8];
  unsigned short actual = 0;
  unsigned char __xdata response[1];
  setup[0] = 0xA1;
  setup[1] = MSC_REQ_GET_MAX_LUN;
  setup[2] = 0;
  setup[3] = 0;
  setup[4] = mscIf;
  setup[5] = 0;
  setup[6] = 1;
  setup[7] = 0;
  if (USBHost_Control(setup, response, &actual) != ERR_SUCCESS || actual != 1) {
    /* Some BOT devices STALL GET MAX LUN when only LUN 0 exists. */
    *maxLun = 0;
    /* A STALL on GET MAX LUN is permitted when only LUN 0 exists.
     * The next SETUP token recovers endpoint zero; do not CLEAR_FEATURE EP0. */
    return 0;
  }
  *maxLun = response[0];
  return 0;
}

unsigned char MSC_ResetRecovery(void)
{
  unsigned char __xdata setup[8];
  unsigned short actual = 0;

  DEBUG_OUT("MSC_ResetRecovery\r\n");

  setup[0] = 0x21;
  setup[1] = MSC_REQ_RESET;
  setup[2] = 0;
  setup[3] = 0;
  setup[4] = mscIf;
  setup[5] = 0;
  setup[6] = 0;
  setup[7] = 0;
  if (USBHost_Control(setup, 0, &actual) != ERR_SUCCESS)
    return 1;
  if (USBHost_ClearHalt(epIn) != ERR_SUCCESS)
    return 2;
  if (USBHost_ClearHalt(epOut) != ERR_SUCCESS)
    return 3;
  toggleIn = toggleOut = 0;
  return 0;
}

#define CSW_NAK_RETRY_MAX 100 // max retry for NAK

static unsigned char readCSW(unsigned long tag, unsigned long expectedLength,
                             unsigned long transferred)
{
  unsigned char s;
  unsigned short actual;
  unsigned char retry;
  unsigned char nakRetry = 0;

  for (retry = 0; retry < 3; retry++) {
    actual = 0;

    memset(csw, 0, 13);
    s = USBHost_BulkIn(epIn, &toggleIn, mpsIn, csw, 13, &actual);
    DEBUG_OUT("readCSW(actual=%d,s=0x%x):\r\n", actual, s);
    if (actual)
      printBuf(csw, 13);
    if (s == ERR_SUCCESS)
      break;

    if (s == (USB_PID_NAK | ERR_USB_TRANSFER)) {
      /* NAK to retry */
      if (++nakRetry >= CSW_NAK_RETRY_MAX)
        return MSC_ERR_TRANSPORT;
      retry--;
      delay(50);
      continue;
    } else if (s == (USB_PID_STALL | ERR_USB_TRANSFER)) {
      /* STALL: Clear Feature ENDPOINT_HALT */
      if (USBHost_ClearHalt(epIn) != ERR_SUCCESS)
        return MSC_ERR_TRANSPORT;
      toggleIn = 0; /* restart with DATA0 after ClearHalt */
      continue;
    }

    return MSC_ERR_TRANSPORT;
  }

  if (s != ERR_SUCCESS || actual != 13) return MSC_ERR_TRANSPORT;
  /* CSW Signature */
  if (get32le(&csw[0]) != 0x53425355UL) return MSC_ERR_TRANSPORT;
  /* Tag */
  if (get32le(&csw[4]) != tag) return MSC_ERR_TRANSPORT;
  /* Residue */
  if (get32le(&csw[8]) != (expectedLength - transferred)) return MSC_ERR_TRANSPORT;
  /* Status */
  if (csw[12] == 1) return MSC_ERR_COMMAND;
  if (csw[12] == 2) return MSC_ERR_PHASE;
  if (csw[12] != 0) return MSC_ERR_TRANSPORT;

  return MSC_OK;
}

unsigned char MSC_Command(const unsigned char *cdb, unsigned char cdbLength,
                          unsigned char direction, unsigned char __xdata *data,
                          unsigned long dataLength)
{
  unsigned char i, s;
  unsigned short actual;
  unsigned long offset = 0, tag;
  unsigned short chunk;
  unsigned short len; //

  if (!cdb || cdbLength == 0 || cdbLength > 16 || (dataLength && !data) ||
      (direction != MSC_DIR_IN && direction != MSC_DIR_OUT))
    return MSC_ERR_ARGUMENT;

  memset(cbw, 0, 31); // for (i = 0; i < 31; i++) cbw[i] = 0;
  tag = nextTag++;
  put32le(cbw, CBW_SIGNATURE);
  put32le(&cbw[4], tag);
  put32le(&cbw[8], dataLength);
  cbw[12] = (direction == MSC_DIR_IN) ? 0x80 : 0x00;
  cbw[13] = lun;
  cbw[14] = cdbLength;
  for (i = 0; i < cdbLength; i++)
    cbw[15 + i] = cdb[i];

  s = USBHost_BulkOut(epOut, &toggleOut, mpsOut, cbw, 31);
  if (s != ERR_SUCCESS) {
    MSC_ResetRecovery();
    return MSC_ERR_TRANSPORT;
  }

  while (offset < dataLength) {
    // chunk = (dataLength-offset > 512UL) ? 512 : (unsignedshort)(dataLength-offset);
    if (direction == MSC_DIR_IN) {
      chunk = (dataLength - offset > mpsIn)? mpsIn: (unsigned short)(dataLength - offset);
      s = USBHost_BulkIn(epIn, &toggleIn, mpsIn, data + offset, chunk, &actual);
      DEBUG_OUT("BulkIn(actual=%d):\r\n", actual);
      printBuf(data + offset, actual);
      if (s != ERR_SUCCESS) {
        /* A data-stage STALL is cleared before reading the CSW. */
        if (s == (USB_PID_STALL | ERR_USB_TRANSFER)) {
          if (USBHost_ClearHalt(epIn) != ERR_SUCCESS) {
            MSC_ResetRecovery();
            return MSC_ERR_TRANSPORT;
          }
          toggleIn = 0;
        } else {
          MSC_ResetRecovery();
          return MSC_ERR_TRANSPORT;
        }
        break;
      }
      offset += actual;
      if (actual < chunk)
        break; // short data
    } else {
      chunk = (dataLength - offset > mpsOut)? mpsOut: (unsigned short)(dataLength - offset);
      s = USBHost_BulkOut(epOut, &toggleOut, mpsOut, data + offset, chunk);
      if (s != ERR_SUCCESS) {
        if (s == (USB_PID_STALL | ERR_USB_TRANSFER)) {
          if (USBHost_ClearHalt(epOut) != ERR_SUCCESS) {
            MSC_ResetRecovery();
            return MSC_ERR_TRANSPORT;
          }
          toggleOut = 0;
        } else {
          MSC_ResetRecovery();
          return MSC_ERR_TRANSPORT;
        }
        break;
      }
      offset += chunk;
    }
  }

  s = readCSW(tag, dataLength, offset);
  if (s == MSC_ERR_PHASE)
    MSC_ResetRecovery();
  return s;
}
