TARGET = mintext.exe
SOURCES = mintextmain.cpp mintextcore.cpp
OBJECTS = $(SOURCES:.cpp=.o)
CXX = g++
CXXFLAGS = -Wall -std=c++11 -I.
LDFLAGS = -lws2_32 -lkernel32 -lgdi32 -luser32 -mconsole
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
clean:
	del $(TARGET) $(OBJECTS)