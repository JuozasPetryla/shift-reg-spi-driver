# SPI driver for shift register SN74HC595N

## Compiling:

`make ARCH={architecture} KDIR={/path/to/linux/kernel} CROSS_COMPILE={toolchain_prefix}`

- Example:
`make ARCH=aarch64 KDIR=../linux CROSS_COMPILE=aarch64-rpi3-linux-musl-`

## Device tree

The .dtso file is left for reference, what actual device tree overlay changes have been applied, in order for the driver to be discovered

