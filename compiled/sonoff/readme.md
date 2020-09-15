# Command line upload:

    esptool.py erase_flash
    esptool.py -b 921600 write_flash 0x000000 firmware.bin 0x7B000 spiffs.bin
