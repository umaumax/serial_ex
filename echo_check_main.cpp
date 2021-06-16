#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

bool recv_serial(std::string serial_port, int send_size) {
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

  std::chrono::system_clock::time_point recv_timestamp;
  std::chrono::system_clock::time_point send_timestamp;
  double total_rtt   = 0.0;
  uint64_t total_cnt = 0;

  std::ofstream output_file("out.bin", std::ios::binary);

  int send_len = send_size;
  char buf[send_len + 1];
  for (int i = 0;; i++) {
    uint8_t v = static_cast<uint8_t>(i % 256);
    memset(buf, v, send_len);
    buf[send_len] = 0;

    send_timestamp = std::chrono::system_clock::now();
    int ret        = write(fd, buf, send_len);
    if (ret == -1) {
      std::cerr << "failed write:" << std::strerror(errno) << std::endl;
      return false;
    }

    int total_len = 0;
    while (total_len != send_len) {
      int read_len = read(fd, buf, sizeof(buf));
      if (read_len == 0) {
        // waiting...
        std::cout << ".";
        continue;
      } else if (read_len < 0) {
        std::cerr << "failed read:" << std::strerror(errno) << std::endl;
        return false;
      }
      for (int i = 0; i < read_len; i++) {
        if (buf[i] != v) {
          std::fprintf(stderr,
                       "failed read wrong echo back: got '%x', expected '%x'\n",
                       buf[i], v);
          return false;
        }
        // std::cout << "[" << i << "] " << std::endl;
        // printf("[len=%d]", read_len);
      }
      total_len += read_len;
    }

    recv_timestamp = std::chrono::system_clock::now();
    double rtt     = std::chrono::duration_cast<std::chrono::microseconds>(
                     recv_timestamp - send_timestamp)
                     .count();
    total_rtt += rtt;
    total_cnt++;
    double ave_rtt = total_rtt / static_cast<double>(total_cnt);
    std::printf("[%3d][%4llu] rtt=%16.6lfms, ave=%16.6lfms\n", send_len,
                total_cnt, rtt / 1000.0, ave_rtt / 1000.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
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
  int send_size           = 8;
  if (argc >= 3) {
    send_size = std::stoi(std::string(argv[2]));
  }

  bool ret = recv_serial(serial_port, send_size);
  if (!ret) {
    return 1;
  }
  return 0;
}
