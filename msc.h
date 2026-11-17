#ifndef MSC_H
#define MSC_H
#include <stdint.h>
#include "USBHost.h"

#define MSC_OK             0
#define MSC_ERR_TRANSPORT  1
#define MSC_ERR_CSW        2
#define MSC_ERR_COMMAND    3
#define MSC_ERR_PHASE      4
#define MSC_ERR_ARGUMENT   5

void MSC_Bind(const USB_MSC_INFO *info);
unsigned char MSC_GetMaxLUN(unsigned char *lun);
unsigned char MSC_Command(const unsigned char *cdb, unsigned char cdbLength,
                          unsigned char direction, unsigned char __xdata *data,
                          unsigned long dataLength);
unsigned char MSC_ResetRecovery(void);
#endif
