LIBS	=	libwconr

all:
	@- $(foreach lib, $(LIBS), make -C $(lib) all;)

clean:
	@- $(foreach lib, $(LIBS), make -C $(lib) clean;)

fclean:
	@- $(foreach lib, $(LIBS), make -C $(lib) fclean;)

re:
	@- $(foreach lib, $(LIBS), make -C $(lib) re;)

.PHONY:	all clean fclean re
