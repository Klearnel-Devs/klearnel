# core makefile
include ../include/Makefile

OBJ-QR:=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))

default: symlink $(OBJ-QR) unsym
	@echo "Logging compiled successfully"

$(OBJ-QR): $(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

symlink:
	@echo "Generating logging symlinks"
	@ln -s ../$(BUILD_DIR) $(BUILD_DIR)
	@ln -s ../$(INC_DIR) $(INC_DIR)
	@ln -s ../$(LIB_DIR) $(LIB_DIR)

clean: unsym


unsym:
	@echo "logging/ will be cleaned..."
	@rm -f $(INC_DIR)
	@rm -f $(BUILD_DIR)
	@rm -f $(LIB_DIR)
	@echo "logging/ is now cleaned"
	