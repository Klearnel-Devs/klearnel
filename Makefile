# Klearnel Top-level makefile

include include/Makefile

OBJ-KL:=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))
EXECUTABLE:=$(BUILD_DIR)/bin/klearnel

default: info $(EXECUTABLE)


info:
	@echo "This module will be compiled with following CFLAGS: "$(CFLAGS)
	@echo "Created by Klearnel-Devs"

$(EXECUTABLE): subdirs $(OBJ-KL) 
	$(CC) $(CFLAGS) -o $@ $(wildcard $(BUILD_DIR)/*.o)

$(OBJ-KL): $(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

subdirs: build
	@cd quarantine; $(MAKE)
	@cd core; 	$(MAKE)
	@cd logging;	$(MAKE)

build:
	@mkdir -p build
	@mkdir -p build/bin

clean: clean-sub
	@echo "Removing all objects and build dirs..."
	@rm -rf $(BUILD_DIR)
	@echo "The project directory is now clean!"

clean-sub:
	@echo "Removing all symlinks generated..."
	@cd quarantine; $(MAKE) clean
	@cd core;	$(MAKE) clean
	@cd logging;	$(MAKE) clean
	