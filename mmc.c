#include "CH559.h"
#include "mmc.h"
#include "msc.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MSC_DIR_OUT 0
#define MSC_DIR_IN 1
static unsigned char __xdata dataBuf[256];
// static unsigned char __xdata tocBuf[256];
static unsigned char firstTrack = 0, lastTrack = 0, tocValid = 0, currTrack = 0;
static unsigned char leadoutMSF[3], currMSF[3];
static unsigned char haveLeadout = 0;

void printBuf(unsigned char *p, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    DEBUG_OUT("%02x%s", *(p + i), (i % 16 == 15) ? "\r\n" : " ");
  }
  if (i % 16)
    DEBUG_OUT("\r\n");
}

// INQUIRY Command
unsigned char SCSI_Inquiry(void)
{
#define INQ_SIZE 96 // 44
  unsigned char cdb[6] = {
      0x12,     // Operation Code 12h:INQUIRY
      0,        //
      0,        // Page Code
      0,        // Allocation length(MSB)
      INQ_SIZE, // Allocation length(LSB)
      0         // Control
  };
  memset(dataBuf, 0, INQ_SIZE);
  unsigned char r = MSC_Command(cdb, 6, MSC_DIR_IN, dataBuf, INQ_SIZE);
  DEBUG_OUT("Inquery:\r\n");
  printBuf(dataBuf, INQ_SIZE);
  if (r) {
    printf("INQUIRY error=%u\r\n", r);
    return r;
  }
  printf("Vendor : %.8s\r\n", &dataBuf[8]);
  printf("Product: %.16s\r\n", &dataBuf[16]);
  printf("Rev    : %.4s\r\n", &dataBuf[32]);
  return 0;
}

// TEST UNIT READY
unsigned char SCSI_TestUnitReady(void)
{
  unsigned char cdb[6] = {0, // Operation Code 00h:Test Unit Ready
                          0, 0, 0, 0, 0};
  return MSC_Command(cdb, 6, MSC_DIR_OUT, 0, 0);
}

// REQUEST SENSE
unsigned char SCSI_RequestSense(void)
{
#define SENSE_SIZE 252                   // 18 //
  unsigned char cdb[6] = {0x03,          // Operation Code
                          0,             // DESC:0
                          0,          0, // Reserved
                          SENSE_SIZE, 0};
  memset(dataBuf, 0, SENSE_SIZE);
  DEBUG_OUT("RequseSense:\r\n");
  unsigned char r = MSC_Command(cdb, 6, MSC_DIR_IN, dataBuf, SENSE_SIZE);
  printBuf(dataBuf, SENSE_SIZE);
  if (!r) {
    printf("SenseKey=%02X ASC=%02X ASCQ=%02X\r\n", dataBuf[2] & 0x0F,
              dataBuf[12], dataBuf[13]);
  } else {
    printf("REQUEST SENSE error=%u\r\n", r);
  }
  return r;
}

// READ TOC
unsigned char MMC_ReadTOC(signed char fb)
{
  unsigned char cdb[10] = {0x43, // OperationCode
                           0x02, // bit 1: 1=TIME, 0=LBA
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  unsigned char r, track;
  unsigned short i, total, bytes;
  unsigned short alloc = 256; // for TIME,

  cdb[7] = (unsigned char)(alloc >> 8);
  cdb[8] = (unsigned char)alloc;

  tocValid = 0;
  haveLeadout = 0;
  memset(dataBuf, 0, alloc);
  r = MSC_Command(cdb, 10, MSC_DIR_IN, dataBuf, alloc);
  if (r) {
    printf("READ TOC error=%u\r\n", r);
    return r;
  }
  total = ((unsigned short)dataBuf[0] << 8) | dataBuf[1];
  if (total < 2 || total > 254) {
    printf("TOC length invalid=%u\r\n", total);
    return 1;
  }
  bytes = total + 2;
  firstTrack = dataBuf[2];
  lastTrack = dataBuf[3];
  currTrack += fb;
  if (currTrack < firstTrack || currTrack > lastTrack) currTrack = firstTrack; // currTrack = 0;
  printf("First=%u Last=%u TOC bytes=%u\r\n", firstTrack, lastTrack, bytes);

  for (i = 4; i + 7 < bytes; i += 8) {
    track = dataBuf[i + 2];
    if (track == 0xAA) {
      // Lead out
      leadoutMSF[0] = dataBuf[i + 5];
      leadoutMSF[1] = dataBuf[i + 6];
      leadoutMSF[2] = dataBuf[i + 7];
      haveLeadout = 1;
      printf("Lead-out %02u:%02u:%02u\r\n", leadoutMSF[0], leadoutMSF[1],
                leadoutMSF[2]);
    } else {
      printf("Track %u %02u:%02u:%02u\r\n", track, dataBuf[i + 5],
                dataBuf[i + 6], dataBuf[i + 7]);

      if (track == currTrack) {
        currMSF[0] = dataBuf[i + 5];
        currMSF[1] = dataBuf[i + 6];
        currMSF[2] = dataBuf[i + 7];
      }
    }
  }
  // memcpy(tocBuf, dataBuf, (size_t)bytes);
  tocValid = 1;
  return 0;
}

/* This first prototype plays the first track only. The end address is the
 * next track's start, or the TOC lead-out for a single-track disc. */
unsigned char MMC_PlayTrack() // unsigned char track)
{
  unsigned char cdb[10] = {0x47, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char *end;

  if (!tocValid || !haveLeadout) //  || track != firstTrack)
    return 1;

  if (currTrack == 0 || currTrack > lastTrack)
    currTrack = firstTrack;

  end = leadoutMSF;

  cdb[3] = currMSF[0];
  cdb[4] = currMSF[1];
  cdb[5] = currMSF[2];

  cdb[6] = end[0];
  cdb[7] = end[1];
  cdb[8] = end[2];

  printf("Play tack %d\r\n", currTrack);
  
  return MSC_Command(cdb, 10, MSC_DIR_OUT, 0, 0);
}

/* unsigned char MMC_PlayCurrTrack(void) */
/* { */
/*   if (!tocValid) */
/*     return 1; */

/*   return MMC_PlayTrack(currTrack); */
/* } */

unsigned char MMC_Pause(unsigned char resume)
{
  unsigned char cdb[10] = {0x4B, 0, 0, 0, 0, 0, 0, 0, (resume ? 1 : 0), 0};
  return MSC_Command(cdb, 10, MSC_DIR_OUT, 0, 0);
}

unsigned char MMC_Stop(void)
{
  unsigned char cdb[6] = {0x1B, 0, 0, 0, 0, 0};
  return MSC_Command(cdb, 6, MSC_DIR_OUT, 0, 0);
}
