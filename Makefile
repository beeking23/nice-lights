IMGUI_DIR = ../imgui

# SRCS = src/main.cpp \
# src/icosahedron.cpp \
# src/icosahedron.hpp \
# src/vertexdesc.hpp \
# src/vertexdesc.cpp \
# src/glhelpers.hpp \
# src/glhelpers.cpp \
# e131/e131.c \
# src/network.cpp \
# src/network.hpp \
# src/serial.cpp

OBJS = main.o icosahedron.o vertexdesc.o glhelpers.o network.o serial.o e131.o
CC = g++
SDL_C_FLAGS = $(shell sdl2-config --cflags)
CXXFLAGS = -std=c++20 -I/home/bunny/gl-libs/glm $(SDL_C_FLAGS) -Wshadow -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I./e131

SRC_IMGUI += $(IMGUI_DIR)/imgui.cpp \
$(IMGUI_DIR)/imgui_demo.cpp \
$(IMGUI_DIR)/imgui_draw.cpp \
$(IMGUI_DIR)/imgui_tables.cpp \
$(IMGUI_DIR)/imgui_widgets.cpp \
$(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
$(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS += $(addsuffix .o, $(basename $(notdir $(SRC_IMGUI))))

%.o:e131/%.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o:src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

nice-lights: $(OBJS)
	g++ $(OBJS) `sdl2-config --libs` -lGL -lGLEW -o nice-lights

clean:
	rm ${OBJS} nice-lights

