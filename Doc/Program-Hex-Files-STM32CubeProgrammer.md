# How to Program Hex Files

0. Ensure the board is in dev mode (boot switch in dev mode position).
1. Open STM32CubeProgrammer.
2. Select the Disco board through the "External loaders" tab.
3. ST-Link configuration: set mode to "Hot plug".
4. Connect the board.
5. From the "Erasing & programming" tab, select the `Binary/ai_fsbl.hex` file.
6. Wait for flashing to complete.
7. From the "Erasing & programming" tab, select the `Binary/network_data.hex` file.
8. Wait for flashing to complete.
9. From the "Erasing & programming" tab, select the `Binary/x-cube-n6-ai-h264-usb-uvc.hex` file.
10. Wait for flashing to complete.

![Board Selection](../_htmresc/selectBoard.JPG)
![Flash the Hex file](../_htmresc/flashHex.JPG)

