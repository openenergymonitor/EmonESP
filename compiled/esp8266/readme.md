# Command line upload:

    esptool.py erase_flash
    esptool.py write_flash 0x000000 src.ino.bin 0x7B000 src.spiffs.bin
