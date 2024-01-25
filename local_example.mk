CXX := clang++


WARN_FLAGS := -Wall -Wextra -Wshadow \
              -Wno-unused-parameter -Wno-unused-function -Wno-unused-result -Wno-cast-function-type


CXXFLAGS += -O0 -g3 $(WARN_FLAGS)
GLFLAGS  +=# -g  # debug info for shader source profiling


VULKAN_CXXFLAGS +=
VULKAN_LDFLAGS  +=
VULKAN_LDLIBS   += -lglfw -lvulkan
