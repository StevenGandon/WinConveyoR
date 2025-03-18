LIB_DIR	=	lib
CLI_DIR	=	cli

all: lib cli

lib:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) all

cli:
	make -C $(CLI_DIR) all

clean:
	make -C $(LIB_DIR) clean
	make -C $(CLI_DIR) clean

fclean:
	rm -rf ./lib/libwconr/libwconr.h
	make -C $(LIB_DIR) fclean
	make -C $(CLI_DIR) fclean

re:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) re
	make -C $(CLI_DIR) re

.PHONY:	all clean fclean re
