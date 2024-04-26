# User Level Scheduler

User level task scheduler using coorperative ULE scheduler

## Requirements

* Linux machine with kernel version >= 5.4
* Install g++. Refer [link](https://gasparri.org/2020/07/30/installing-c17-and-c20-on-ubuntu-and-amazon-linux/)
* Install liburing by running following commands outside the project root directory.

```
git clone https://github.com/axboe/liburing.git
make -C ./liburing
```


## To compile

Install g++. Refer [link](https://gasparri.org/2020/07/30/installing-c17-and-c20-on-ubuntu-and-amazon-linux/)

Run command
```
g++ -std=c++2a -pthread main.cc -I../liburing/src/include/ -L../liburing/src/ -luring
```


## To run

Run command
```
./a.out
```
