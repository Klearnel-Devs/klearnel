# Klearnel Top-level makefile

include include/Makefile

OBJ-KL:=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))
EXECUTABLE:=$(BUILD_DIR)/bin/klearnel

default: info $(EXECUTABLE)
	@mkdir -p build/out
	@echo "Creating compressed ZIP archive..."
	@zip build/out/klearnel-binaries.zip build/bin/*
	@echo "ZIP archive created successfully"
	@echo "Creating compressed TAR BZ2 archive..."
	@tar cvfj build/out/klearnel-binaries.tar.bz2 build/bin
	@echo "TAR BZ2 archive created successfully"

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
	@cp LICENSE build/bin
	@cp README.md build/bin


clean: clean-sub
	@echo "Removing all objects and build dirs..."
	@rm -rf $(BUILD_DIR)
	@echo "The project directory is now clean!"

clean-sub:
	@echo "Removing all symlinks generated..."
	@cd quarantine; $(MAKE) clean
	@cd core;	$(MAKE) clean
	@cd logging;	$(MAKE) clean
	