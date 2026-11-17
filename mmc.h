#ifndef MMC_H
#define MMC_H
#include <stdint.h>
unsigned char SCSI_Inquiry(void);
unsigned char SCSI_TestUnitReady(void);
unsigned char SCSI_RequestSense(void);
unsigned char MMC_ReadTOC(signed char fb);
unsigned char MMC_PlayTrack(); // unsigned char track);
// unsigned char MMC_PlayFirstTrack(void);
unsigned char MMC_Pause(unsigned char resume);
unsigned char MMC_Stop(void);
#endif
