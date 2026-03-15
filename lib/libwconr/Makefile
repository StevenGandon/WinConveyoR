SRC	=	./src/wcr_state.c	\
		./src/http_parsing.c	\
		./src/http_headers.c	\
		./src/http_request.c \
		./src/http_response.c \
		./src/download_package.c \
		./src/pars_wizard.c \
		./src/wizard_endian.c \
		./src/wizard_section.c \
		./src/vm.c \
		./src/vm_parse.c \
		./src/vm_decode.c \
		./src/vm_handlers.c

OBJ	=	$(SRC:.c=.o)

NAME	=	libwconr
EXT		?=	so

CC		?=	gcc
CFLAGS	+=	-Wall -Wshadow -Wextra -Wdouble-promotion -Waggregate-return -Wstrict-prototypes \
			-pedantic -Werror -Wformat -Wcast-qual -Wconversion -Winit-self	\
			-I ./ -I /usr/include/libxml2 -fPIC -O2
LDFLAGS	+=	-lm -lxml2

ifeq ($(OS), Windows_NT)
	SRC += ./src/win/http.c
	LDFLAGS += -lws2_32 -lwsock32 -lsecur32 -lcrypt32
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
