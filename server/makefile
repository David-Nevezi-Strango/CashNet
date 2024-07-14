CXX := g++
TARGET := server
LIB_DIR := lib
DB_DIR := db
DB := dbinit
LIBS := $(wildcard $(LIB_DIR)/*.cpp)
OBJS := $(patsubst $(LIB_DIR)/%.cpp, %.o, $(LIBS))
LIBS_HEADER := $(wildcard $(LIB_DIR)/*.h) 
# $(shell find $(LIB_DIR) -name "*.h" | awk "!/(Trash)|(build)/" | sort | uniq)
LIBS_HEADER += $(wildcard ./%.h)

CPPFLAGS := -O3 -std=c++11 -lsqlite3 -lpthread
dependency:
# apt-get install gcc
	apt-get install g++
	apt-get install sqlite3
	apt-get install libsqlite3-dev
	mkdir requests

# ./lib/sqlite3/configure
# cd lib/sqlite3/
# make

all: $(TARGET)

database: $(DB)
$(DB): $(DB_DIR)/$(DB).cpp
	$(CXX) -o $(DB_DIR)/$@ $^ $(CPPFLAGS) 

$(TARGET): $(TARGET).o $(OBJS) $(LIBS_HEADER)
	@echo
	$(CXX) --version
	$(CXX) -o $@ $^ -I . -I $(LIB_DIR) $(CPPFLAGS)


%.o: %.cpp
	@echo
	$(CXX) --version
	$(CXX) -o $@ -c $^ -I . -I $(LIB_DIR) $(CPPFLAGS)

%.o: $(LIB_DIR)/%.cpp
	@echo
	$(CXX) --version
	$(CXX) -o $@ -c $^ -I . -I $(LIB_DIR) $(CPPFLAGS)

.PHONY: clean
clean:
	-rm -f $(TARGET) *.o $(DB_DIR)/$(DB)
