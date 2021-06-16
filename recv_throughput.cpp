#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

bool recv_serial(std::string serial_port, int read_num = -1) {
  int fd;

  // open read/write for echo back
  fd = open(serial_port.c_str(), O_RDWR);
  if (fd < 0) {
    printf("open error\n");
    return false;
  }

  int ret;

  ret = tcflush(fd, TCIFLUSH);
  if (ret == -1) {
    std::cerr << "failed tcflush:" << std::strerror(errno) << std::endl;
    return false;
  }

  std::chrono::system_clock::time_point recv_start;

  std::ofstream output_file("out.bin", std::ios::binary);
  uint64_t recv_total_size = 0;
  int len;
  char buf[8192];
  for (int i = 0; read_num == -1 || i < read_num; i++) {
    len = read(fd, buf, sizeof(buf));
    if (len == 0) {
      if (read_num == -1) {
        // waiting...
        std::cout << ".";
      }
      continue;
    } else if (len < 0) {
      std::cerr << "failed read:" << std::strerror(errno) << std::endl;
      return false;
    } else {
      std::cout << "[" << i << "] " << std::endl;
      printf("[len=%d]", len);
      if (recv_start.time_since_epoch().count() == 0) {
        recv_start = std::chrono::system_clock::now();
      } else {
        recv_total_size += len;
        double recv_elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - recv_start)
                .count();
        double throughput =
            static_cast<double>(recv_total_size) / (recv_elapsed / 1000000.0);
        printf(" (%lf B/s) %lu\n", throughput, recv_total_size);
      }
      // for (int i = 0; i < len; i++) {
      // printf("%02X", (unsigned char)(buf[i]));
      // }
      printf("\n");
      // std::cout << std::string(&buf[0], len) << std::endl;
      output_file.write(buf, static_cast<std::streamsize>(len));
      if (output_file.bad()) {
        std::cerr << "error to write file" << std::endl;
      }
      output_file.flush();
    }
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

  bool ret = recv_serial(serial_port);
  if (!ret) {
    return 1;
  }
  return 0;
}
