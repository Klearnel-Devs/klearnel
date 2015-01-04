# Quarantine makefile
include ../include/Makefile

OBJ-QR:=$(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))

default: symlink $(OBJ-QR) 
	@echo "Quarantine compiled successfully"

$(OBJ-QR): $(BUILD_DIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

symlink:
	@echo "Generating quarantine symlinks"
	@ln -s ../$(BUILD_DIR) $(BUILD_DIR)
	@ln -s ../$(INC_DIR) $(INC_DIR)
	@ln -s ../$(LIB_DIR) $(LIB_DIR)


clean:
	@echo "Quarantine dir will be cleaned..."
	@rm $(INC_DIR)
	@rm $(BUILD_DIR)
	@rm $(LIB_DIR)
	@echo "Quarantine dir is now cleaned"