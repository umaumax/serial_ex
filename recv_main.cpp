#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

bool recv_serial(std::string serial_port, speed_t baudrate, int read_num = -1) {
  int fd;
  struct termios tio_bk, tio;

  // open read/write for echo back
  fd = open(serial_port.c_str(), O_RDWR);
  if (fd < 0) {
    printf("open error\n");
    return false;
  }

  int ret;

  // push previous saved setting
  ret = tcgetattr(fd, &tio_bk);
  if (ret == -1) {
    std::cerr << "failed tcgetattr:" << std::strerror(errno) << std::endl;
    return false;
  }

  memset(&tio, 0, sizeof(tio));
  tio.c_cflag    = CREAD | CLOCAL | CS8;
  tio.c_iflag    = IGNPAR;
  tio.c_cc[VMIN] = 1;

  ret = cfsetspeed(&tio, baudrate);
  if (ret == -1) {
    std::cerr << "failed cfsetspeed:" << std::strerror(errno) << std::endl;
    return false;
  }

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret == -1) {
    std::cerr << "failed tcsetattr:" << std::strerror(errno) << std::endl;
    return false;
  }

  ret = tcflush(fd, TCIFLUSH);
  if (ret == -1) {
    std::cerr << "failed tcflush:" << std::strerror(errno) << std::endl;
    return false;
  }

  int len;
  char buf[256];
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
      return false;
    }
  }

  // pop previous saved setting
  ret = tcsetattr(fd, TCSANOW, &tio_bk);
  if (ret == -1) {
    std::cerr << "failed tcsetattr:" << std::strerror(errno) << std::endl;
    return false;
  }
  ret = close(fd);
  if (ret == -1) {
    std::cerr << "failed close:" << std::strerror(errno) << std::endl;
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << argv[0] << " serial_port baudrate" << std::endl;
    std::cerr << "e.g. serial_port='/dev/ttyUSB0'" << std::endl;
    std::cerr << "     baudrate=9600" << std::endl;
    std::cerr << "     baudrate=-1 means try all baudrate" << std::endl;
    return 1;
  }
  std::string serial_port = std::string(argv[1]);
  int baudrate_number     = std::stoi(std::string(argv[2]));

  // NOTE: full list: 110, 150, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600
  std::map<int, speed_t> baudrate_map = {
      {4800, B4800},   {9600, B9600},   {9600, B9600},     {19200, B19200},
      {38400, B38400}, {57600, B57600}, {115200, B115200}, {230400, B230400},
  };
  if (baudrate_number != -1) {
    auto it = baudrate_map.find(baudrate_number);
    if (it == baudrate_map.end()) {
      std::cerr << "valid baudrate values: [";
      for (auto& v : baudrate_map) {
        std::cerr << v.first << ", ";
      }
      std::cerr << "]" << std::endl;
      return 1;
    }
    speed_t baudrate = it->second;
    bool ret         = recv_serial(serial_port, baudrate);
    if (!ret) {
      return 1;
    }
  } else {
    // try all baudrate
    int read_num = 8;
    for (auto& v : baudrate_map) {
      int baudrate_number = v.first;
      speed_t baudrate    = v.second;
      std::cout << "baudrate:" << baudrate_number << std::endl;
      bool ret = recv_serial(serial_port, baudrate, read_num);
      if (!ret) {
        return 1;
      }
    }
  }
  return 0;
}
