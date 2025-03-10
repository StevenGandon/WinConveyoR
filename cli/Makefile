SRC			= main.py

NAME		= 	wcr
PYINSTALLER	?=	pyinstaller
PYFLAGS		+=	-c -F

all:
	$(PYINSTALLER) $(SRC) $(PYFLAGS) -n $(NAME)

$(NAME): all

clean:

fclean: clean

re: fclean all

.PHONY:	all clean fclean re
