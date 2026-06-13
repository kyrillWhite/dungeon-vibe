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

CXXFLAGS := -Iinclude -Ilibs

all: release

build:
	g++ -std=c++17 $(CXXFLAGS) -Ilibs/imgui -DGAME_DEBUG src/*.cpp libs/imgui/*.cpp libs/imgui/backends/*.cpp -o $(OUT) $(LIBS)

run: build
	./$(OUT)

release:
	g++ -std=c++17 $(CXXFLAGS) src/*.cpp -o $(OUT) $(LIBS)

release-run: release
	./$(OUT)
