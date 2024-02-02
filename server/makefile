dependency:
# apt-get install gcc
	apt-get install g++
	apt-get install sqlite3
	apt-get install libsqlite3-dev
	mkdir requests

# ./lib/sqlite3/configure
# cd lib/sqlite3/
# make

server: server.cpp addresses.h
	g++ -o server server.cpp -std=c++11 -Ilib lib/transaction.cpp lib/customer.cpp lib/account.cpp  -lsqlite3 -lpthread
