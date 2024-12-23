How to build:

On Client:
`g++ -c ../utils/socketutils.cpp -std=c++17 && g++ main.cpp socketutils.o -I../utils/ -o main -std=c++17`
On Server:
`g++ -c ../utils/socketutils.cpp -std=c++17 && g++ main.cpp socketutils.o -I../utils/ -o server -std=c++17`