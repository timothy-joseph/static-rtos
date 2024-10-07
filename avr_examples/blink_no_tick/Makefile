default:
	avr-gcc -Os -DF_CPU=16000000L -mmcu=atmega328p -Wall -Wextra -Wpedantic -std=c99 -I../../static_rtos/include/ ../../static_rtos/port/avr_port.c -DSTATIC_RTOS_AVR_TARGET -c -o build/port.o
	avr-gcc -Os -DF_CPU=16000000L -mmcu=atmega328p -Wall -Wextra -Wpedantic -std=c99 -I../../static_rtos/include/ ../../static_rtos/kernel/scheduler.c -DSTATIC_RTOS_AVR_TARGET -c -o build/scheduler.o
	avr-gcc -Os -DF_CPU=16000000L -mmcu=atmega328p -Wall -Wextra -Wpedantic -std=c99 -I../../static_rtos/include/ main.c -DSTATIC_RTOS_AVR_TARGET -c -o main.o 
	avr-gcc -mmcu=atmega328p -o main.bin build/scheduler.o build/port.o main.o -Wall -Wextra
	avr-objcopy -O ihex -R .eeprom main.bin main.hex
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 115200 -U flash:w:main.hex
