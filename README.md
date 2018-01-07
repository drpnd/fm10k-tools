# fm10k-tools

This project aims to develop a utility set for Intel FM10K-based network
interface cards (NICs).

## Note
The FM10K Linux driver, fm10k.ko, maps its BAR4 region to /dev/uioX.  This tool
mmap() and read()/write() from/to this device to control the internal switching
functionalities.

