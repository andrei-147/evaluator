# 1. Variables
CXX      := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Wpedantic
TARGET   := my_program
SRC      := main.cpp

# 2. Default Target
all: $(TARGET)

# 3. Build Rule
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# 4. Clean Rule
clean:
	rm -f $(TARGET)

# 5. Run Rule (Optional convenience)
run: all
	./$(TARGET)
