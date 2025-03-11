SRC	=	./src/wcr_state.c
OBJ	=	$(SRC:.c=.o)

NAME	=	libwconr
EXT		?=	so

CC		?=	gcc
CFLAGS	+=	-Wall -Wshadow -Wextra -Wdouble-promotion -Waggregate-return -Wstrict-prototypes \
			-pedantic -Werror -Wformat -I ./ -fPIC -O2
LDFLAGS	+=	-lm

all:	$(NAME)

$(NAME):	$(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared $(OBJ) -o $(NAME).$(EXT)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME).$(EXT)

re:	fclean all

.PHONY:	all clean fclean re
