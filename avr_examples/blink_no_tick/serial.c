#include <stdlib.h>

/* macros */
#define FOSC 16000000L

/* function definitions */
static FILE *
start_serial(unsigned long int speed)
{
	struct serial_buff *buffer = NULL;
	int ubrr = FOSC / 16 / speed - 1;
	/* wait half a second so we don't brick the arduino nano */
	_delay_ms(500);

	/* set baud rate */
	UBRR0H = (char)(ubrr >> 8);
	UBRR0L = (char)ubrr;

	/* set operation mode, 8 bit, no parity, single bit end */
	buffer = (struct serial_buff *)calloc(1, sizeof(struct serial_buff));

	if (buffer == NULL) {
		UCSR0B = (1 << RXEN0) | (1 << TXEN0);
		UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
		serial = fdevopen(send_byte, receive_byte_no_interrupt_with_wait);
	} else {
		UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
		UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
		serial = fdevopen(send_byte, receive_byte_from_buffer_with_wait);
		serial->buf = (char *)buffer;
		sei();
	}
	
	return serial;
}

static int
send_byte(char byte, FILE *f)
{
	/* puts a char with no interrupts (the program wait for
	 * the char to be able to be sent)
	 * this is the default put char function for the serial
	 * file
	 */
	if (f == NULL)
		return EOF;

	if (byte == '\n')
		send_byte('\r', f);

	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = byte;

	return 0;
}

static int
receive_byte_no_interrupt_with_wait(FILE *f)
{
	/* receives byte from usart when interrupts aren't available
	 * this is the default get char function for the serial file
	 * when the buffer couldn't be allocated
	 */
	if (f == NULL)
		return EOF;

	while (!(UCSR0A & (1 << RXC0)));

	return UDR0;
}

static int
receive_byte_from_buffer_with_wait(FILE *f)
{
	/* receives byte from usart when interrupts are available
	 * this function wait for the buffer to receive a new byte
	 * before returning
	 * this is the default get char function for the serial file
	 * when the buffer could be allocated
	 */
	if (f == NULL)
		return EOF;

	struct serial_buff *buffer = (struct serial_buff *)serial->buf;
	char data = 0;

	if (buffer == NULL)
		return receive_byte_no_interrupt_with_wait(f);

	/* for some reason if a nop isn't put here, the program doesn't work */
	/* wait for there to be data to read */
	while (buffer->start == buffer->end)
		_NOP();
	data = buffer->buff[buffer->start];

	buffer->start = (buffer->start + 1) % BUFFER_SIZE;

	return data;
}

static int
receive_byte_from_buffer_no_wait(FILE *f)
{
	/* returns the char to which start points to
	 * if there has been a new char put into the
	 * buffer. this function increments start (eats
	 * the char from the buffer).
	 * this function doesn't wait for a new char to
	 * be available. if a new char isn't available (start == end)
	 * then it returns EOF
	 * if the serial was started without a buffer, then it returns EOF
	 */
	struct serial_buff *buffer = (struct serial_buff *)serial->buf;
	char data = 0;

	if (buffer == NULL)
		return EOF;
	if (buffer->start == buffer->end)
		return EOF;
	
	data = buffer->buff[buffer->start];
	buffer->start = (buffer->start + 1) % BUFFER_SIZE;

	return data;
}

static int
peek_from_buffer_no_wait(FILE *f)
{
	/* returns the char to which start points to if
	 * there has been a new char put in the buffer, but
	 * doesn't increment the start of the buffer (doesn't
	 * eat the char from the buffer)
	 * this function doesn't wait for a new char to be
	 * available. if a new char isn't available (start == end)
	 * then it returns EOF
	 * if the serial was started without a buffer, then it returns EOF
	 */
	struct serial_buff *buffer = (struct serial_buff *)serial->buf;

	if (buffer == NULL)
		return EOF;
	
	return buffer->buff[buffer->start];
}

ISR(USART_RX_vect)
{
	struct serial_buff *buffer = (struct serial_buff *)serial->buf;
	char data = 0;

	if (buffer == NULL)
		return;

	/* receive the message */
	while (!(UCSR0A & (1 << RXC0)));
	data = UDR0;
	
	/* put the message in the buffer */
	buffer->buff[buffer->end] = data;
	buffer->end = (buffer->end + 1) % BUFFER_SIZE;
}
