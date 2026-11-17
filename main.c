#include "CH559.h"
#include "USBHost.h"
#include "mmc.h"
#include "msc.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>

static unsigned char mscReady = 0;
static unsigned char maxLun = 0;

static void printMenu(void)
{
  printf("\r\nCommands: i=INQUIRY t=TEST UNIT READY s=SENSE r=READ TOC\r\n");
  printf("          p=play track, f=forward, b=backward\r\n");
  printf("          a=pause, c=resume, x=stop ?=menu\r\n");
}

static void attachMSC(void)
{
  MSC_Bind(&usbMscInfo);
  MSC_GetMaxLUN(&maxLun);
  printf("MSC interface=%u IN=%02X/%u OUT=%02X/%u LUN=%u\r\n",
              usbMscInfo.interfaceNumber, usbMscInfo.epInAddress,
              usbMscInfo.epInMaxPacket, usbMscInfo.epOutAddress,
              usbMscInfo.epOutMaxPacket, maxLun);
  mscReady = 1;
  printf("USB MSC device ready.\r\n");
  SCSI_Inquiry();
  if (SCSI_TestUnitReady() != 0) {
    printf(
        "Drive not ready; REQUEST SENSE follows. Insert an audio CD.\r\n");
    SCSI_RequestSense();
  } else {
    printf("Drive ready.\r\n");
  }
  printMenu();
}

static void command(char c)
{
  unsigned char r = 0;
  switch (c) {
  case 'i':
    r = SCSI_Inquiry();
    break;
  case 't':
    r = SCSI_TestUnitReady();
    printf("TUR=%u\r\n", r);
    break;
  case 's':
    r = SCSI_RequestSense();
    break;
  case 'r':
    r = MMC_ReadTOC(0);
    break;
  case 'b':
    r = MMC_ReadTOC(-1);
    if (!r)
      r = MMC_PlayTrack(); // r = MMC_PlayFirstTrack();
    printf("PLAY=%u\r\n", r);
    break;
  case 'f':
    r = MMC_ReadTOC(1);
    if (!r)
      r = MMC_PlayTrack(); // r = MMC_PlayFirstTrack();
    printf("PLAY=%u\r\n", r);
    break;
  case 'p':
    r = MMC_ReadTOC(0);
    if (!r)
      r = MMC_PlayTrack(); // r = MMC_PlayFirstTrack();
    printf("PLAY=%u\r\n", r);
    break;
  case 'a':
    r = MMC_Pause(0);
    printf("PAUSE=%u\r\n", r);
    break;
  case 'c':
    r = MMC_Pause(1);
    printf("RESUME=%u\r\n", r);
    break;
  case 'x':
    r = MMC_Stop();
    printf("STOP=%u\r\n", r);
    break;
  case '?':
    printMenu();
    break;
  default:
    break;
  }
  if (r != 0 && c != 's' && c != 'i') {
    printf("Command result=%u; sense:\r\n", r);
    SCSI_RequestSense();
  }
}

void main(void)
{
  unsigned char c;
  initClock();
  initUART0(115200, 1);
  printf("\r\nCH559 USB CD-DA controller prototype\r\n");
  printf(
      "Target: USB MSC/BOT + SCSI/MMC; analog audio remains in drive.\r\n");
  resetHubDevices(0);
  resetHubDevices(1);
  initUSB_Host();
  printf("USB host initialized. Connect CRW-F1UX.\r\n");
  printMenu();
  while (1) {
    checkRootHubConnections();
    if (usbMscInfo.valid && !mscReady)
      attachMSC();
    if (!usbMscInfo.valid && mscReady) {
      mscReady = 0;
      printf("USB MSC disconnected.\r\n");
    }
    if (RI) {
      RI = 0;
      c = SBUF;
      if (c == '\r' || c == '\n')
        continue;
      if (mscReady)
        command(c);
      else if (c == '?')
        printMenu();
    }
  }
}
