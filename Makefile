CXX = g++
CXXFLAGS = -std=c++17 -O3 -s -I. -MMD -MP

SRC = main.cpp Lexer.cpp Parser.cpp TypeChecker.cpp Interpreter.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)

TARGET = pixel.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)
