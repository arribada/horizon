#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Constants
#define SYSHAL_UART_NO_ERROR               ( 0)
#define SYSHAL_UART_ERROR_INVALID_SIZE     (-1)
#define SYSHAL_UART_ERROR_INVALID_INSTANCE (-2)
#define SYSHAL_UART_ERROR_BUSY             (-3)
#define SYSHAL_UART_ERROR_TIMEOUT          (-4)
#define SYSHAL_UART_ERROR_DEVICE           (-5)

#define  MAXIMUM_SIZE_WRITE (1024)
#define  MAXIMUM_SIZE_READ (1024)
#define PRINT_READ 1


/* use omega UART1 */
const char *portname = "/dev/ttyUSB0";
int uartFd = -1;

uint8_t b_error_injected = 0;
uint8_t data_inject_error[256];
uint32_t size_error = 0;

static inline uint8_t get_size_command(const uint8_t *ptr)
{
    uint8_t len = 0;
    while (*ptr++ != '\r')len++;
    return len + 1;
}
static int convert_from_baud(uint32_t baudrate)
{
    switch (baudrate)
    {
        case 9600:
            return B9600;
            break;
        case 19200:
            return B19200;
            break;
        case 38400:
            return B38400;
            break;
        case 57600:
            return B57600;
            break;
        case 115200:
            return B115200;
            break;
        case 230400:
            return B230400;
            break;
        default:
            return B9600;
            break;

    }
}
static uint32_t convert_to_baud(int baudrate)
{
    switch (baudrate)
    {
        case B9600:
            return 9600;
            break;
        case B19200:
            return 19200;
            break;
        case B38400:
            return 38400;
            break;
        case B57600:
            return 57600;
            break;
        case B115200:
            return 115200;
            break;
        case B230400:
            return 230400;
            break;
        default:
            return 9600;
            break;

    }
}

int syshal_uart_init(uint32_t instance)
{
    uartFd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (uartFd < 0)
    {
        printf("%s\n", "PROBLEM OPEN PORT" );
        return SYSHAL_UART_ERROR_DEVICE;
    }
    // set speed, 8n1 (no parity)

    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (uartFd, &tty) != 0)
    {
        fprintf (stderr, "error %d from tcgetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }

    cfsetospeed (&tty, B9600);
    cfsetispeed (&tty, B9600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_lflag &= ~ICANON;
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CRTSCTS;

    if (tcsetattr (uartFd, TCSANOW, &tty) != 0)
    {
        fprintf (stderr, "error %d from tcsetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }


    return SYSHAL_UART_NO_ERROR;
}
int syshal_uart_change_baud(uint32_t instance, uint32_t baudrate)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (uartFd, &tty) != 0)
    {
        return SYSHAL_UART_ERROR_DEVICE;
    }

    cfsetospeed (&tty, convert_from_baud(baudrate));
    cfsetispeed (&tty, convert_from_baud(baudrate));


    if (tcsetattr (uartFd, TCSANOW, &tty) != 0)
    {
        fprintf (stderr, "error %d from tcsetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }
    return SYSHAL_UART_NO_ERROR;
}

int syshal_uart_get_baud(uint32_t instance, uint32_t * baudrate)
{

    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (uartFd, &tty) != 0)
    {
        return SYSHAL_UART_ERROR_DEVICE;
    }

    *baudrate = convert_to_baud(cfgetospeed (&tty));
    return SYSHAL_UART_NO_ERROR;
}
int syshal_uart_send(uint32_t instance, uint8_t * data, uint32_t size)
{
    uint32_t current_length = 0;
    int bytes_read = 0;
    int remain_data = size;
#ifdef GTEST
    printf("SEND COMMAND: ");
    for (int i = 0; i < size; ++i)
    {
        printf("%c", (char) data[i] );
    }
    printf("\n");
#endif

    while (remain_data > 0)
    {
        if (remain_data > MAXIMUM_SIZE_WRITE)
        {
            current_length = MAXIMUM_SIZE_WRITE;
        }
        else
        {
            current_length = remain_data;
        }
        bytes_read = write(uartFd, data, current_length);
        if (bytes_read < 0) return SYSHAL_UART_ERROR_DEVICE;
        remain_data -= bytes_read;
    }
    return SYSHAL_UART_NO_ERROR;
}

int syshal_uart_flush(uint32_t instance)
{
    uint8_t buffer;
    uint32_t bytes_read;
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(uartFd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 1;            // 0.5 seconds read timeout
    if (tcsetattr(uartFd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }
    do
    {
        bytes_read = read(uartFd, &buffer, sizeof(buffer) );
    }while(bytes_read != 0);
    return SYSHAL_UART_NO_ERROR;
}
int syshal_uart_read_timeout(uint32_t instance, uint8_t * buffer, uint32_t buf_size, uint32_t read_timeout_us, uint32_t last_char_timeout_us, uint32_t * bytes_received)
{
    int timeout = read_timeout_us / 100000;
    int bytes_read;
    if ((last_char_timeout_us / 100000) > timeout)
        timeout = last_char_timeout_us / 100000;
    if (timeout == 0)
        timeout = 1;
    if (timeout > 200) timeout = 200;
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(uartFd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = timeout;            // 0.5 seconds read timeout
    if (tcsetattr(uartFd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return SYSHAL_UART_ERROR_DEVICE;
    }

    bytes_read = read(uartFd, buffer, buf_size );
#ifdef PRINT_READ
    printf("character read: ");
    for (int i = 0; i < bytes_read; ++i)
    {
        printf("%c", buffer[i]);
    }
    printf("\n");
#endif
    *bytes_received = bytes_read;
    if (bytes_read == 0) return SYSHAL_UART_ERROR_TIMEOUT;
    if (bytes_read < 0 ) printf("%s\n", "ERROR");
    return SYSHAL_UART_NO_ERROR;
}
/* MOCK FUNCTIONS */
int syshal_uart_receive(uint32_t instance, uint8_t * data, uint32_t size)
{
    return 0;
}
uint32_t syshal_uart_available(uint32_t instance)
{
    return 0;
}
void inject_error(uint8_t b_inject_error , uint8_t *data, uint32_t size)
{
    b_error_injected = b_inject_error;
    memcpy(data_inject_error, data, size);
    size_error = size;
}