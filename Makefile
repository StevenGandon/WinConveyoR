LIB_DIR	=	lib
CLI_DIR	=	cli

all:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) all
	make -C $(CLI_DIR) all

clean:
	make -C $(LIB_DIR) clean
	make -C $(CLI_DIR) clean

fclean:
	make -C $(LIB_DIR) fclean
	make -C $(CLI_DIR) fclean

re:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) re
	make -C $(CLI_DIR) re

.PHONY:	all clean fclean re
