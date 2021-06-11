#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <thread>

#define BAUDRATE B9600
#define SERIAL_PORT "/dev/ttyUSB0"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << argv[0] << " filepath" << std::endl;
    return 1;
  }
  std::string filepath = std::string(argv[1]);
  int packet_size      = 256;

  if (argc >= 3) {
    packet_size = std::stoi(std::string(argv[2]));
  }

  int fd;
  struct termios tio_bk, tio;

  double fps   = 30.0;
  int loop_num = 1000 * 1000;

  // open write only
  fd = open(SERIAL_PORT, O_WRONLY);
  if (fd < 0) {
    perror(SERIAL_PORT);
    std::cerr << "failed open:" << std::strerror(errno) << std::endl;
    return 1;
  }

  int ret;
  // push previous saved setting
  ret = tcgetattr(fd, &tio_bk);
  if (ret == -1) {
    std::cerr << "failed tcgetattr:" << std::strerror(errno) << std::endl;
    return 1;
  }

  memset(&tio, 0, sizeof(tio));
  tio.c_cflag = CREAD | CLOCAL | CS8;
  // tio.c_cflag = CREAD | CLOCAL | CS8 | PARENB; // add parity to output and check input parity
  tio.c_iflag = IGNPAR;  // ignore parity error
  // tio.c_iflag = IXON; // with software flow control of output

  ret = cfsetospeed(&tio, BAUDRATE);
  if (ret == -1) {
    std::cerr << "failed cfsetospeed:" << std::strerror(errno) << std::endl;
    return 1;
  }

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret == -1) {
    std::cerr << "failed tcsetattr:" << std::strerror(errno) << std::endl;
    return 1;
  }

  ret = tcflush(fd, TCOFLUSH);
  if (ret == -1) {
    std::cerr << "failed tcflush:" << std::strerror(errno) << std::endl;
    return 1;
  }

  // std::string data;
  // for (int i = 0; i <= 0xFF; i++) {
  // data += std::string(1, char(i));
  // }

  std::ifstream input_file(filepath);
  std::string data_str((std::istreambuf_iterator<char>(input_file)),
                       std::istreambuf_iterator<char>());
  int data_size    = data_str.length();
  const char* data = data_str.c_str();

  for (int offset = 0; offset < data_size;) {
    int write_size = packet_size;
    if (offset + write_size > data_size) {
      write_size = data_size - offset;
    }
    int ret = write(fd, &data[offset], write_size);
    if (ret == -1) {
      std::cerr << "failed write:" << std::strerror(errno) << std::endl;
      return 1;
    }
    std::this_thread::sleep_for(
        std::chrono::microseconds(int(1.0 / fps * 1000.0 * 1000.0)));
    offset += write_size;
  }

  // for (int i = 0; i < loop_num; i++) {
  // // std::cout << "[" << i << "] send: [" << data << "]" << std::endl;
  // int ret = write(fd, data.c_str(), data.length());
  // if (ret == -1) {
  // std::cerr << "failed write:" << std::strerror(errno) << std::endl;
  // return 1;
  // }
  // std::this_thread::sleep_for(
  // std::chrono::microseconds(int(1.0 / fps * 1000.0 * 1000.0)));
  // }

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
