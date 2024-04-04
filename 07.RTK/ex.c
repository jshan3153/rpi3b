#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <pthread.h>

static int fd = 0;
#define MAX_BUF_SIZE 512
#define DEV_JIGBEE_UART "/dev/ttyMAX"

int init_uart(char * dev, int baud, int * fd)
{
    struct termios newtio;
    * fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if ( * fd < 0) {
        printf("%s> uart dev '%s' open fail [%d]n", __func__, dev, * fd);
        return -1;
    }
    memset( & newtio, 0, sizeof(newtio));
    newtio.c_iflag = IGNPAR; // non-parity 
    newtio.c_oflag = OPOST | ONLCR;;
    newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts 
    switch (baud)
    {
		case 115200:
		  newtio.c_cflag |= B115200;
		  break;
		case 57600:
		  newtio.c_cflag |= B57600;
		  break;
		case 38400:
		  newtio.c_cflag |= B38400;
		  break;
		case 19200:
		  newtio.c_cflag |= B19200;
		  break;
		case 9600:
		  newtio.c_cflag |= B9600;
		  break;
		case 4800:
		  newtio.c_cflag |= B4800;
		  break;
		case 2400:
		  newtio.c_cflag |= B2400;
		  break;
		default:
		  newtio.c_cflag |= B115200;
		  break;
    }

    newtio.c_lflag = 0;
    //newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위 
    //newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기 
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush( * fd, TCIFLUSH);
    tcsetattr( * fd, TCSANOW, & newtio);
    return 0;
}

void test_read_loop(void * arg)
{
    int result;
    char buffer[MAX_BUF_SIZE];
    fd_set reads, temps;

    FD_ZERO( & reads);
    FD_SET(fd, & reads);

    while (1)
    {
        temps = reads;
        result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

        if (result < 0)
        {
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd, & temps))
        {
            memset(buffer, 0, sizeof(buffer));
            if (read(fd, buffer, MAX_BUF_SIZE) == -1)
                continue;
            printf("receive buffer is [%s]rn", buffer);
        }
    }
}
int main()
{
	int result, ret = -1;
    pthread_t p_thread;

    ret = init_uart(DEV_JIGBEE_UART, 115200, & fd);
    printf("init uart [%d], [%d]rn", ret, fd);
}
