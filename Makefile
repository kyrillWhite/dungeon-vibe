ifeq ($(OS),Windows_NT)
	UNAME_CHECK := $(shell uname -s 2>EMPTY)
	OUT := game.exe
	ifeq ($(findstring MINGW,$(UNAME_CHECK)),MINGW)
		LIBS := -lglew32 -lglfw3 -lopengl32
	else
		LIBS := -lglew32 -lglfw3 -lopengl32 -lgdi32
	endif
else
	OUT := game
	LIBS := -lGLEW -lglfw -lGL
endif

CXXFLAGS := -DGLEW_STATIC -Iinclude -Ilibs
IMGUI_LIB := libs/imgui
IMGUI_SRC := $(IMGUI_LIB)/*.cpp $(IMGUI_LIB)/backends/imgui_impl_opengl3.cpp $(IMGUI_LIB)/backends/imgui_impl_glfw.cpp

all: release

build:
	g++ -std=c++17 $(CXXFLAGS) -I$(IMGUI_LIB) -DGAME_DEBUG src/*.cpp $(IMGUI_SRC) -o $(OUT) $(LIBS)

run: build
	./$(OUT)

release:
	g++ -std=c++17 $(CXXFLAGS) src/*.cpp -o $(OUT) $(LIBS)

release-run: release
	./$(OUT)
