LIB_DIR	=	lib

all:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) all

clean:
	make -C $(LIB_DIR) clean

fclean:
	make -C $(LIB_DIR) fclean

re:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(LIB_DIR) re

.PHONY:	all clean fclean re
