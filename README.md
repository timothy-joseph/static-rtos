# static-rtos

This project is a example of a rtos which uses statically allocated memory for
all of its structures.

## Features

1. Priority scheduler with timeslicing for threads with the same priority
2. Binary mutexes (TODO)
3. Somewhat portable
4. Automatically generated documentation with doxygen

## Supported architectures

1. AVR - fully supported
2. Linux - not fully supported
	The linux port is missing thread arguments and the timer

## Usage

You need to download the `static_rtos` directory to your project's directory and
specify in your compile flags to use `static_rtos/include` to lookup for
includes in there. You should also use as a compile flag -D`YOUR_MCU_PORT`.
Where `YOUR_MCU_PORT` is either `STATIC_RTOS_AVR_TARGET` or (TODO).\
You must also compile the following files:
`static_rtos/src/port/your_mcu_port.c`, `static_rtos/src/scheduler.c`
and link them along side your project's c files.

## Porting (TODO)

## License

See LICENSE.txt for details

