# SPI driver for shift register SN74HC595N

## Compiling:

`make ARCH={architecture} KDIR={/path/to/linux/kernel} CROSS_COMPILE={toolchain_prefix}`

- Example:
`make ARCH=aarch64 KDIR=../linux CROSS_COMPILE=aarch64-rpi3-linux-musl-`

## Device tree

The .dtso file is left for reference, what actual device tree overlay changes have been applied, in order for the driver to be discovered

## Usage

`echo` a string pattern of up to 16 bits (4 states of LEDs)

Examples:

`echo "1100 0011" > /dev/my_shift_register`

This will blink the LEDs like such: ++-- -> --++ periodicaly

`echo "1100 0011 1010 0101" > /dev/my_shift_register`

This will blink the LEDs like such: ++-- -> --++ -> +-+- -> -+-+ periodicaly
