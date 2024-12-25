Dependencies:
1. libzip
2. cpr

How to run:
```
g++ -std=c++17 -o main -I/usr/local/include -L/usr/local/lib/ -lcpr -lzip -rpath /usr/local/lib  main.cpp && ./main
```