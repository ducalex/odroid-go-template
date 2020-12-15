# Description
Native ESP-IDF 4.0 application template for the ODROID-GO.

The tools folder contains the mkfw.py utility from 
[odroid-go-multi-firmware](https://github.com/ducalex/odroid-go-multi-firmware) 
and patches for esp-idf to improve SD Card compatibility.

# Build
- `idf.py build`
- `./tools/mkfw.py my-project.fw "My Project" tile.raw 0 0 0 my-project build/my-project.bin`
