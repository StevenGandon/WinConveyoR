CLI_DIR	=	cli

all: cli

cli:
	make -C $(CLI_DIR) all

clean:
	make -C $(CLI_DIR) clean

fclean:
	rm -rf ./lib/libwconr/libwconr.h
	make -C $(CLI_DIR) fclean

re:
	cp ./include/libwconr.h ./lib/libwconr

	make -C $(CLI_DIR) re

.PHONY:	all clean fclean re lib cli
