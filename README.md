# serial programs

## how to build
``` bash
g++ -std=c++11 send_main.cpp -pthread -o send_main
g++ -std=c++11 recv_main.cpp -pthread -o recv_main
g++ -std=c++11 serial_port_icount.cpp -o serial_port_icount
```
