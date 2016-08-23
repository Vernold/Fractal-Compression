CXX = g++
CXXFLAGS = -O2 -g -Wall -std=c++0x
CXXFLAGS += -Wno-error=unused-command-line-argument -Werror -Wformat-security -Wignored-qualifiers -Winit-self \
		-Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith \
		-Wtype-limits -Wempty-body \
		-Wmissing-field-initializers -Wctor-dtor-privacy \
		-Wnon-virtual-dtor -Wold-style-cast \
		-Woverloaded-virtual -Wsign-promo -Weffc++ -Wextra -pedantic

SRC_DIR = src
INCLUDE_DIR = include

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
DEP_DIR = $(BUILD_DIR)/deps

BRIDGE_MAKE = bridge/Makefile
BRIDGE_INCLUDE_DIR = bridge/include
BRIDGE_LIBRARY_DIR = bridge/lib

BRIDGE_TARGETS = easybmp
LDFLAGS = -leasybmp

CXXFLAGS += -I $(INCLUDE_DIR) -I $(BRIDGE_INCLUDE_DIR)
CXXFLAGS += -L $(BRIDGE_LIBRARY_DIR)

make_path = $(addsuffix $(1), $(basename $(subst $(2), $(3), $(4))))
src_to_obj = $(call make_path,.o, $(SRC_DIR), $(OBJ_DIR), $(1))
src_to_dep = $(call make_path,.d, $(SRC_DIR), $(DEP_DIR), $(1))

CXXFILES := $(wildcard $(SRC_DIR)/*.cpp)

.DEFAULT_GOAL := all

.PHONY: all
all: $(BIN_DIR)/compress

Makefile: ;

ifneq ($(MAKECMDGOALS), clean)
-include bridge.touch
endif

bridge.touch: $(wildcard $(BRIDGE_INCLUDE_DIR)/*) \
		$(wildcard $(BRIDGE_LIBRARY_DIR)/*)
	mkdir -p $(BRIDGE_INCLUDE_DIR) $(BRIDGE_LIBRARY_DIR)
	make -C $(dir $(BRIDGE_MAKE)) -f $(notdir $(BRIDGE_MAKE)) $(BRIDGE_TARGETS)
	mkdir -p $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)
	echo "include deps.mk" > $@

$(BIN_DIR)/compress: $(OBJ_DIR)/main.o $(OBJ_DIR)/Algorithm.o $(OBJ_DIR)/AC.o $(OBJ_DIR)/Conversion.o bridge.touch
	$(CXX) $(CXXFLAGS) $(filter %.o, $^) -o $@ $(LDFLAGS)

$(DEP_DIR)/%.d: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -E -MM -MT $(call src_to_obj, $<) -MT $@ -MF $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $(call src_to_obj, $<) $<

.PHONY: clean
clean:
	make -C $(dir $(BRIDGE_MAKE)) -f $(notdir $(BRIDGE_MAKE)) clean
	rm -rf $(BUILD_DIR) $(BRIDGE_INCLUDE_DIR) $(BRIDGE_LIBRARY_DIR)
	rm -f bridge.touch
