UNAME := $(shell uname)

# OS Specifics
ifeq ($(UNAME), Linux)
CXX		= gcc
LIBS		:= -pthread -lrt
endif
ifeq ($(UNAME), Darwin)
CXX		= clang
LIBS           := -framework IOKit
endif

# Sources
SOURCES		= src/main.c src/t_common.c src/tfile.c src/tfile_client.c src/tfile_server.c src/tfile_shared.c src/tinycthread.c src/t_socket.c src/t_common_linux.c

# Includes
INCLUDES	= -Isrc/include

# Libs

# Global Settings
CXXFLAGS	= -Wall
OBJECTS		= $(SOURCES:%.c=%.o)
EXECUTABLE	= TFile

release: CXXFLAGS += -O2
release: $(SOURCES) $(EXECUTABLE)

debug: CXXFLAGS += -DDEBUG -g3
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.c.o:
	$(CXX) $(INCLUDES) -c -o $@ $< $(CXXFLAGS)

