CXX := g++
TARGET := server
LIB_DIR := lib
LIBS := $(wildcard $(LIB_DIR)/%.cpp)
LIBS_HEADER := $(wildcard $(LIB_DIR)/%.h)

CPPFLAGS := -O3 -std=c++11 -lsqlite3 -lpthread

dependency:
# apt-get install gcc
	apt-get install g++
	apt-get install sqlite3
	apt-get install libsqlite3-dev

# ./lib/sqlite3/configure
# cd lib/sqlite3/
# make

all: $(TARGET)
$(TARGET): $(TARGET).cpp $(LIBS) $(LIBS_HEADER)
	$(CXX) -o $@ $^ -I. -I$(LIB_DIR) $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)
