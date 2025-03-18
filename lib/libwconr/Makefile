SRC	=	./src/wcr_state.c	\
		./src/http_parsing.c	\
		./src/http_headers.c	\
		./src/http_request.c \
		./src/http_response.c \
		./src/download_package.c

OBJ	=	$(SRC:.c=.o)

NAME	=	libwconr
EXT		?=	so

CC		?=	gcc
CFLAGS	+=	-Wall -Wshadow -Wextra -Wdouble-promotion -Waggregate-return -Wstrict-prototypes \
			-pedantic -Werror -Wformat -Walloc-zero -Wcast-qual -Wconversion -Winit-self	\
			-Wlogical-op -I ./ -fPIC -O2
LDFLAGS	+=	-lm

ifeq ($(OS), Windows_NT)
	SRC += ./src/win/http.c
	LDFLAGS += -lws2_32 -lwsock32
else
	SRC += ./src/posix/http.c
endif

all:	$(NAME)

$(NAME):	$(OBJ)
	$(CC) $(CFLAGS) -shared $(OBJ) -o $(NAME).$(EXT) $(LDFLAGS)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME).$(EXT)

re:	fclean all

.PHONY:	all clean fclean re
