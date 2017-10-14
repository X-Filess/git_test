
void uart_printf(const char *fmt, ...);

int uart_device_init(unsigned int Baudrate);

int uart_device_receive(unsigned char *Buffer, unsigned int byteToRead);

int uart_device_send(unsigned char *Buffer, unsigned int byteToWrite);

int uart_device_close(void);

