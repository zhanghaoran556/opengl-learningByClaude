CXX      = clang++
CXXFLAGS = -std=c++17 -Wall \
           $(shell pkg-config --cflags glfw3 glew) \
           -I/opt/homebrew/include \
           -I/opt/homebrew/Cellar/glm/1.0.1/include
LDFLAGS  = $(shell pkg-config --libs glfw3 glew) \
           -framework OpenGL

LESSONS = 01_hello_window 02_hello_triangle 03_shaders 04_textures 05_transformations

.PHONY: all clean $(LESSONS)

all: $(LESSONS)

01_hello_window:
	$(CXX) $(CXXFLAGS) src/01_hello_window/main.cpp -o build/$@ $(LDFLAGS)

02_hello_triangle:
	$(CXX) $(CXXFLAGS) src/02_hello_triangle/main.cpp -o build/$@ $(LDFLAGS)

03_shaders:
	$(CXX) $(CXXFLAGS) src/03_shaders/main.cpp -o build/$@ $(LDFLAGS)

04_textures:
	$(CXX) $(CXXFLAGS) src/04_textures/main.cpp -o build/$@ $(LDFLAGS)

05_transformations:
	$(CXX) $(CXXFLAGS) src/05_transformations/main.cpp -o build/$@ $(LDFLAGS)

clean:
	rm -rf build/

# 自动创建 build 目录
$(shell mkdir -p build)
