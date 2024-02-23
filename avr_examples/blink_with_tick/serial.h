#define BUFFER_SIZE 10

/* data types */
struct serial_buff {
	/* circular buffer for usart, reading with interrupts */
	char buff[BUFFER_SIZE];
	unsigned int start, end;
};

/* function declarations */
static FILE *start_serial(unsigned long int speed);
static int send_byte(char byte, FILE *f);
static int receive_byte_no_interrupt_with_wait(FILE *f);
static int receive_byte_from_buffer_with_wait(FILE *f);
static int receive_byte_from_buffer_no_wait(FILE *f);
static int peek_from_buffer_no_wait(FILE *f);

/* global variables */
static FILE *serial = NULL;

#include "serial.c"
