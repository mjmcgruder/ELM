MAKEFLAGS += -r -j

CXXFLAGS := -std=c++11
GLFLAGS  := -O

VULKAN_CXXFLAGS :=
VULKAN_LDFLAGS  :=
VULKAN_LDLIBS   :=  # vulkan, glfw, glm

include local.mk

BIN        := bin
BIN_SHADER := bin_shader

$(shell mkdir -p $(BIN))
$(shell mkdir -p $(BIN_SHADER))
BIN_SHADER_ABSOLUTE := $(shell cd bin_shader && pwd)/

# dependencies

src      := $(wildcard source/*.cpp)
vshaders := $(patsubst %.vert,%.spv,$(wildcard shaders/*.vert))
fshaders := $(patsubst %.frag,%.spv,$(wildcard shaders/*.frag))
cshaders := $(patsubst %.comp,%.spv,$(wildcard shaders/*.comp))

# targets

ELM   := $(BIN)/elm
VSHDR := $(patsubst shaders/%,$(BIN_SHADER)/%,$(vshaders))
FSHDR := $(patsubst shaders/%,$(BIN_SHADER)/%,$(fshaders))
CSHDR := $(patsubst shaders/%,$(BIN_SHADER)/%,$(cshaders))

# ui

.PHONY : elm
elm: $(ELM)

.PHONY : clean
clean:
	rm -f $(ELM) $(VSHDR) $(FSHDR) $(CSHDR)

# targets

$(ELM): $(src) $(VSHDR) $(FSHDR) $(CSHDR)
	$(CXX) $(CXXFLAGS) $(VULKAN_CXXFLAGS) -Isource -DSHADER_DIR=\"$(BIN_SHADER_ABSOLUTE)\" -o$@ source/elm.cpp $(VULKAN_LDFLAGS) $(VULKAN_LDLIBS)

# shaders

$(VSHDR): $(BIN_SHADER)/%.spv: shaders/%.vert shaders/*.glsl
	glslc $(GLFLAGS) -Ishaders -o$@ $<
$(FSHDR): $(BIN_SHADER)/%.spv: shaders/%.frag shaders/*.glsl
	glslc $(GLFLAGS) -Ishaders -o$@ $<
$(CSHDR): $(BIN_SHADER)/%.spv: shaders/%.comp shaders/*.glsl
	glslc $(GLFLAGS) -Ishaders -o$@ $<
