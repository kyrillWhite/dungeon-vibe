UNAME := $(shell uname -s)

ifeq ($(findstring MINGW,$(UNAME)),MINGW)
OUT := openag_game.exe
LIBS := -lglew32 -lglfw3 -lopengl32
else
OUT := openag_game
LIBS := -lGLEW -lglfw -lGL
endif

all: build

run: build
	./$(OUT)

build:
	g++ main.cpp -o $(OUT) $(LIBS)
