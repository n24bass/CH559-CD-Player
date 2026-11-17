#ifndef __USBHOST_H__
#define __USBHOST_H__

#define ROOT_HUB_COUNT 2

#define MAX_EXHUB_PORT_COUNT 4
#define EXHUB_PORT_NONE 0xff
#define MAX_INTERFACE_COUNT 4
#define MAX_ENDPOINT_COUNT 4
#define MAX_EXHUB_LEVEL 1
#define ENDPOINT_OUT 0
#define ENDPOINT_IN 1

#define ERR_SUCCESS 0x00
#define ERR_USB_CONNECT 0x15
#define ERR_USB_DISCON 0x16
#define ERR_USB_BUF_OVER 0x17
#define ERR_USB_DISK_ERR 0x1F
#define ERR_USB_TRANSFER 0x20
#define ERR_USB_UNSUPPORT 0xFB
#define ERR_USB_UNKNOWN 0xFE

#define ROOT_DEVICE_DISCONNECT 0
#define ROOT_DEVICE_CONNECTED 1
#define ROOT_DEVICE_FAILED 2
#define ROOT_DEVICE_SUCCESS 3

/*#define DEV_TYPE_KEYBOARD   ( USB_DEV_CLASS_HID | 0x20 )
#define DEV_TYPE_MOUSE      ( USB_DEV_CLASS_HID | 0x30 )
#define DEV_TYPE_JOYSTICK      ( USB_DEV_CLASS_HID | 0x40 )
#define DEV_TYPE_GAMEPAD      ( USB_DEV_CLASS_HID | 0x50 )*/

#define HID_SEG_KEYBOARD_MODIFIER_INDEX 0
#define HID_SEG_KEYBOARD_VAL_INDEX 1
#define HID_SEG_BUTTON_INDEX 2
#define HID_SEG_X_INDEX 3
#define HID_SEG_Y_INDEX 4
#define HID_SEG_WHEEL_INDEX 5
#define HID_SEG_COUNT 6

typedef struct _EndPoint {
  unsigned char EndpointAddr;
  unsigned short MaxPacketSize;

  unsigned char EndpointDir : 1;
  unsigned char TOG : 1;
} EndPoint, *PEndPoint;

typedef struct _HIDSegmentStructure {
  unsigned char KeyboardReportId;
  unsigned char MouseReportId;

  struct {
    unsigned char start;
    unsigned char size;
    unsigned char count;
  } HIDSeg[HID_SEG_COUNT];
} HIDSegmentStructure;

#define HID_KEYBOARD_VAL_LEN 6
#define MAX_HID_KEYBOARD_BIT_VAL_LEN 15

typedef struct _KeyboardParseStruct {
  unsigned char KeyboardVal[HID_KEYBOARD_VAL_LEN];
  unsigned char KeyboardBitVal[MAX_HID_KEYBOARD_BIT_VAL_LEN];
} KeyboardParseStruct;

typedef struct _Interface {
  unsigned char InterfaceClass;
  unsigned char InterfaceProtocol;

  unsigned char EndpointCount;
  EndPoint endpoint[MAX_ENDPOINT_COUNT];

  HIDSegmentStructure HidSegStruct;

  KeyboardParseStruct KeyboardParseStruct;
} Interface, *PInterface;

typedef struct _UsbDevice {
  unsigned char DeviceClass;
  unsigned char MaxPacketSize0;

  unsigned short VendorID;
  unsigned short ProductID;
  unsigned short bcdDevice;

  unsigned char DeviceAddress;
  unsigned char DeviceSpeed;
  unsigned char InterfaceCount;
  Interface interface[MAX_INTERFACE_COUNT];

  unsigned char HubPortNum;
} UsbDevice, *PUsbDevice;

void resetRootHub(unsigned char i);
void initUSB_Host();
unsigned char checkRootHubConnections();

void resetHubDevices(unsigned char hubindex);
void pollHIDdevice();

/* USB MSC transport access for the CD-player application. */
typedef struct _USB_MSC_INFO {
  unsigned char valid;
  unsigned char rootHub;
  unsigned char interfaceNumber;
  unsigned char epInAddress;
  unsigned char epOutAddress;
  unsigned short epInMaxPacket;
  unsigned short epOutMaxPacket;
} USB_MSC_INFO;

extern USB_MSC_INFO __xdata usbMscInfo;
unsigned char USBHost_Control(const unsigned char __xdata *setup,
                              unsigned char __xdata *data,
                              unsigned short *actual);
unsigned char USBHost_BulkIn(unsigned char endpointAddress,
                             unsigned char *toggle, unsigned short maxPacket,
                             unsigned char __xdata *data,
                             unsigned short requested, unsigned short *actual);
unsigned char USBHost_BulkOut(unsigned char endpointAddress,
                              unsigned char *toggle, unsigned short maxPacket,
                              const unsigned char __xdata *data,
                              unsigned short length);
unsigned char USBHost_ClearHalt(unsigned char endpointAddress);

#endif
