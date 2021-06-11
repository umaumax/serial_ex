#include <fcntl.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

bool DumpSerialPortStats(std::string serial_port) {
  int fd;

  fd = open(serial_port.c_str(), O_RDWR);
  if (fd < 0) {
    std::cerr << "failed open:" << std::strerror(errno) << std::endl;
    return false;
  }
  int ret;

  struct serial_icounter_struct icount = {};
  ret                                  = ioctl(fd, TIOCGICOUNT, &icount);
  if (ret != -1) {
    printf(
        "%s: TIOCGICOUNT: ret=%d\n"
        "cts = %d, dsr = %d, rng = %d, dcd = %d\n"
        "rx = %d, tx=%d\n"
        "frame = %d, overrun = %d, parity = %d, brk = %d\n"
        "buf_overrun = %d\n",
        serial_port.c_str(), ret, icount.cts, icount.dsr, icount.rng,
        icount.dcd, icount.rx, icount.tx, icount.frame, icount.overrun,
        icount.parity, icount.brk, icount.buf_overrun);
    printf("reserved = [");
    for (int i = 0; i < 9; i++) {
      if (i > 0) {
        printf(", ");
      }
      printf("%d", icount.reserved[i]);
    }
    printf("]\n");
  } else {
    std::cerr << "failed ioctl(TIOCGICOUNT):" << std::strerror(errno)
              << std::endl;
  }

  ret = close(fd);
  if (ret == -1) {
    std::cerr << "failed close:" << std::strerror(errno) << std::endl;
    return false;
  }
  return true;
}
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << argv[0] << " serial_port" << std::endl;
    std::cerr << "e.g. serial_port='/dev/ttyUSB0'" << std::endl;
    return 1;
  }
  std::string serial_port = std::string(argv[1]);
  bool ret                = DumpSerialPortStats(serial_port);
  if (!ret) {
    return 1;
  }
  return 0;
}
