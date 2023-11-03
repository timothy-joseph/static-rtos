default:
	avr-gcc -Os -DF_CPU=16000000L -mmcu=atmega328p -c -o main.o main.c
	avr-gcc -mmcu=atmega328p -o main.bin main.o -Wall -Wextra
	avr-objcopy -O ihex -R .eeprom main.bin main.hex
	avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 115200 -U flash:w:main.hex
