# LiquidGlass Hyprland plugin

CXX ?= g++
PKG_CONFIG ?= pkg-config

TARGET := liquidglass.so
BUILD_DIR ?= build
PLUGIN_DIR ?= $(HOME)/.local/share/hyprland/plugins

SOURCES := \
	src/GlassDecoration.cpp \
	src/GlassPassElement.cpp \
	src/GlassRenderer.cpp \
	src/LayerGlassPassElement.cpp \
	src/ShaderManager.cpp \
	src/main.cpp

OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

CPPFLAGS += $(shell $(PKG_CONFIG) --cflags hyprland pixman-1 libdrm)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -fPIC -std=c++23
LDFLAGS += -shared
LDLIBS += $(shell $(PKG_CONFIG) --libs hyprland)

ifneq (,$(findstring g++,$(notdir $(CXX))))
	CXXFLAGS += --no-gnu-unique
endif

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) "$(DESTDIR)$(PLUGIN_DIR)/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(PLUGIN_DIR)/$(TARGET)"

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean install uninstall
