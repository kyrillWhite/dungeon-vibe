ifeq ($(OS),Windows_NT)
	UNAME_CHECK := $(shell uname -s 2>EMPTY)
	OUT := game.exe
	FLAGS := -mwindows
	ifeq ($(findstring MINGW,$(UNAME_CHECK)),MINGW)
		LIBS := -lglew32 -lglfw3 -lopengl32
	else
		LIBS := -lglew32 -lglfw3 -lopengl32 -lgdi32
	endif
	MKDIR := powershell -Command "New-Item -ItemType Directory -Force"
else
	OUT := game
	FLAGS := 
	LIBS := -lGLEW -lglfw -lGL
	MKDIR := mkdir -p
endif

CXX := g++
CXX_FLAGS_BASE := -std=c++17 -DGLEW_STATIC -Iinclude -Ilibs

IMGUI_DIR := libs/imgui
IMGUI_INC := -I$(IMGUI_DIR)

MY_SRC := $(wildcard src/*.cpp)
MY_OBJ_DEBUG := $(patsubst src/%.cpp, obj/debug/%.o, $(MY_SRC))
MY_OBJ_RELEASE := $(patsubst src/%.cpp, obj/release/%.o, $(MY_SRC))

IMGUI_SRC := $(wildcard $(IMGUI_DIR)/*.cpp) \
             $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp \
             $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp

IMGUI_OBJ := $(patsubst $(IMGUI_DIR)/%.cpp, obj/debug/%.o, \
             $(patsubst $(IMGUI_DIR)/backends/%.cpp, obj/debug/%.o, $(IMGUI_SRC)))

.PHONY: all debug release run

all: debug

debug: CXXFLAGS := $(CXX_FLAGS_BASE) $(IMGUI_INC) -DGAME_DEBUG -g
debug: $(OUT)

$(OUT): $(MY_OBJ_DEBUG) $(IMGUI_OBJ)
	$(CXX) $(CXXFLAGS) $(MY_OBJ_DEBUG) $(IMGUI_OBJ) -o $(OUT) $(LIBS) $(FLAGS)

release: CXXFLAGS := $(CXX_FLAGS_BASE) -O3
release: $(MY_OBJ_RELEASE)
	$(CXX) $(CXXFLAGS) $(MY_OBJ_RELEASE) -o $(OUT) $(LIBS) $(FLAGS)

obj/debug/%.o: src/%.cpp | obj/debug
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/debug/%.o: $(IMGUI_DIR)/%.cpp | obj/debug
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/debug/%.o: $(IMGUI_DIR)/backends/%.cpp | obj/debug
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/release/%.o: src/%.cpp | obj/release
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/debug:
	@$(MKDIR) obj/debug

obj/release:
	@$(MKDIR) obj/release

run: debug
	./$(OUT)
