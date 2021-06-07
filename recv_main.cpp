#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#define BAUDRATE B9600
#define SERIAL_PORT "/dev/ttyUSB0"

int main(int argc, char *argv[]) {
  int fd;
  struct termios tio_bk, tio;

  // open read/write for echo back
  fd = open(SERIAL_PORT, O_RDWR);
  if (fd < 0) {
    printf("open error\n");
    return -1;
  }

  int ret;

  // push previous saved setting
  ret = tcgetattr(fd, &tio_bk);
  if (ret == -1) {
    std::cerr << "failed tcgetattr:" << std::strerror(errno) << std::endl;
    return 1;
  }

  memset(&tio, 0, sizeof(tio));
  tio.c_cflag    = CREAD | CLOCAL | CS8;
  tio.c_iflag    = IGNPAR;
  tio.c_cc[VMIN] = 1;

  ret = cfsetspeed(&tio, BAUDRATE);
  if (ret == -1) {
    std::cerr << "failed cfsetspeed:" << std::strerror(errno) << std::endl;
    return 1;
  }

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret == -1) {
    std::cerr << "failed tcsetattr:" << std::strerror(errno) << std::endl;
    return 1;
  }

  ret = tcflush(fd, TCIFLUSH);
  if (ret == -1) {
    std::cerr << "failed tcflush:" << std::strerror(errno) << std::endl;
    return 1;
  }

  int len;
  char buf[256];
  for (int i = 0; true; i++) {
    len = read(fd, buf, sizeof(buf));
    if (len == 0) {
      // waiting...
      std::cout << ".";
      continue;
    } else if (len < 0) {
      std::cerr << "failed read:" << std::strerror(errno) << std::endl;
      return 1;
    } else {
      std::cout << "[" << i << "] " << std::endl;
      printf("[len=%d]", len);
      for (int i = 0; i < len; i++) {
        printf("%02X", (unsigned char)(buf[i]));
      }
      printf("\n");
      std::cout << std::string(&buf[0], len) << std::endl;
    }

    // echo back
    int ret = write(fd, buf, len);
    if (ret == -1) {
      std::cerr << "failed write:" << std::strerror(errno) << std::endl;
      return 1;
    }
  }

  // pop previous saved setting
  ret = tcsetattr(fd, TCSANOW, &tio_bk);
  if (ret == -1) {
    std::cerr << "failed tcsetattr:" << std::strerror(errno) << std::endl;
    return 1;
  }
  ret = close(fd);
  if (ret == -1) {
    std::cerr << "failed close:" << std::strerror(errno) << std::endl;
    return 1;
  }
  return 0;
}
