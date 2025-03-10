SRC			= main.py

NAME		= 	wcr
PYINSTALLER	?=	pyinstaller
PYFLAGS		+=	-c -F

all:
	$(PYINSTALLER) $(SRC) $(PYFLAGS) -n $(NAME)

$(NAME): all

clean:
	find . | grep -E "(/__pycache__$\|\.pyc$\|\.pyo$\)" | xargs rm -rf
	rm -rf $(NAME).spec
	rm -rf ./build

fclean: clean
	rm -rf ./dist

re: fclean all

.PHONY:	all clean fclean re
