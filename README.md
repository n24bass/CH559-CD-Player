# CH559 + YAMAHA CRW-F1UX CD-DA Control Prototype

## Objective
This prototype uses the CH559’s USB host functionality—via the USB Mass Storage Bulk-Only Transport (BOT) protocol—to issue SCSI/MMC commands to a YAMAHA CRW-70 or CRW-F1UX drive for CD-DA playback. Audio data is not transferred over USB; instead, the drive’s analog LINE OUT port is used.

## Build
1. Extract the ZIP archive.
2. Run `compile.bat` in a Windows environment (requires the `SDCC` folder included in the original repository).
3. Upon successful compilation, `CH559_CDPlayer.hex` and `CH559_CDPlayer.bin` will be generated.
4. The current `compile.bat` does not perform automatic flashing. Please verify your CH559 board and USB power circuitry before attempting to flash the firmware.
5. For Linux environments, use `sdcc`...

## UART
UART0 is configured at 115200 bps, using the same alternate pin settings as the original project (P0.2 for TX, P0.3 for RX).
The following commands are used to control the CD player:

- `i`: INQUIRY
- `t`: TEST UNIT READY
- `s`: REQUEST SENSE
- `r`: READ TOC
- `p`: Play current track (after READ TOC)
- `f`: Play next track (current track + 1; after READ TOC)
- `b`: Play previous track (current track - 1; after READ TOC)
- `a`: PAUSE
- `c`: RESUME
- `x`: STOP
- `?`: Menu

## Connections and Power
- Power the CRW-F1UX using its original AC adapter. Do not attempt to power the drive from the 5V USB supply of the CH559 board.
- Ensure that the USB host connector on the CH559 board has sufficient VBUS power delivery capacity. A VBUS switch and overcurrent protection are recommended.
- Connect analog audio from the LINE OUT RCA L/R jacks on the rear of the CRW-F1UX to an amplifier.

## Important Limitations and Unverified Aspects
This source code is an "initial port for hardware evaluation," extending the `CH559sdccUSBHost` host transfer implementation to include MSC/BOT (Mass Storage Class / Bulk-Only Transport) support. It assumes a single USB device—the CRW-70 (CRW-F1UX)—is connected directly to the Root Hub; USB hubs and multi-device configurations are not supported.

The following points, in particular, require verification and adjustment on the actual hardware:

1. The original `USBHost.c` was a sample designed for HID devices; the USB enumeration logic remains largely unchanged. It has been extended to recognize MSC interface descriptors and Bulk IN/OUT endpoints.
2. The USB Host controller’s DMA buffer utilizes the original implementation’s fixed 64-byte Rx/Tx buffers. Ensure that the Bulk endpoint's maximum packet size is 64 bytes or less (the maximum for Full-Speed ​​Bulk transfers is 64 bytes).
3. The implementation of BOT-related features—such as handling CSW residue, STALL recovery, short DATA IN transfers, and NAK retries—is minimal. Further adjustments may be necessary based on logs captured from the actual hardware.
4. If a device returns a STALL in response to a `GET MAX LUN` request, it is treated as having only a single LUN (LUN 0).
5. The `READ TOC` command is implemented to retrieve up to 256 bytes using Format 0 (MSF). Current track playback logic is limited to the first track, using either the start position of the subsequent track or the TOC "Lead-out" as the termination point.
6. The `PLAY AUDIO MSF` command is implemented as a 10-byte CDB (Command Descriptor Block).
7. It has not yet been verified whether the USB bridge in the CRW-F1UX correctly passes through MMC commands such as `PLAY AUDIO MSF` or `PAUSE`/`RESUME`. Please begin by verifying `INQUIRY` and `READ TOC` commands before attempting playback. 8. If a BOT Reset Recovery or Clear Feature operation fails, it is necessary to adjust the control transfer timing and the DATA toggle handling within `USBHost_Control`.

## Regarding the original code
The base is `atc1441/CH559sdccUSBHost`.