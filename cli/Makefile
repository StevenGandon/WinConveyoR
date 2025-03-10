SRC			= main.py

NAME		= 	wcr
PYINSTALLER	?=	pyinstaller
PYFLAGS		+=	-c -F
ICON		?=	""

ifneq ($(ICON), "")
	ifeq ($(OS), Windows_NT)
		PYFLAGS += -i $(ICON)
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S), Darwin)
			PYFLAGS += -i $(ICON)
		endif
	endif
endif

all:
	$(PYINSTALLER) $(SRC) $(PYFLAGS) -n $(NAME) -y

$(NAME): all

clean:
	find . | grep -E "(/__pycache__$\|\.pyc$\|\.pyo$\)" | xargs rm -rf
	rm -rf $(NAME).spec
	rm -rf ./build

fclean: clean
	rm -rf ./dist

re: fclean all

.PHONY:	all clean fclean re
