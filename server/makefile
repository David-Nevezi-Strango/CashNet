CXX := g++
TARGET := server
LIB_DIR := lib
LIBS := $(wildcard $(LIB_DIR)/%.cpp)

CPPFLAGS := -03 -std=c++11 -lsqlite3 -lpthread

dependency:
# apt-get install gcc
	apt-get install g++
	apt-get install sqlite3
	apt-get install libsqlite3-dev

# ./lib/sqlite3/configure
# cd lib/sqlite3/
# make

all: $(TARGET)
$(TARGET): $(TARGET).cpp $(LIBS)
	$(CXX) -o $@ $^ -I. -I$(LIB_DIR) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)
