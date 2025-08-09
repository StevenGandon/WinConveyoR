CLI_DIR =	cli
LIB_DIR =	lib/libwconr
GUI_DIR =	gui

all: lib gui cli

lib:
	cmake -S $(LIB_DIR) -B $(LIB_DIR)/build
	cmake --build $(LIB_DIR)/build --config Release

gui:
	npm --prefix $(GUI_DIR) run build

cli:
	make -C $(CLI_DIR) all

clean:
	test -d $(LIB_DIR)/build && cmake --build $(LIB_DIR)/build --config Release --target clean || true
	make -C $(CLI_DIR) clean
	rm -rf $(GUI_DIR)/dist $(GUI_DIR)/dist-electron

fclean: clean
	rm -rf $(LIB_DIR)/build
	make -C $(CLI_DIR) fclean
	rm -rf $(GUI_DIR)/dist $(GUI_DIR)/dist-electron

re: fclean all

.PHONY: all clean fclean re lib gui cli