SRC			= main.py

NAME		= 	wcr
PYINSTALLER	?=	pyinstaller
PYFLAGS		+=	-c -F --add-data "../assets/cli:./assets/cli"
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

ifeq ($(OS), Windows_NT)
	PYFLAGS += --add-binary "build/libwconr.dll:."
	LIBRARY = ../lib/libwconr/build/Release/libwconr.dll
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Darwin)
		PYFLAGS += --add-binary "build/libwconr.dylib:."
		LIBRARY = ../lib/libwconr/build/libwconr.dylib
	else
		PYFLAGS += --add-binary "build/libwconr.so:."
		LIBRARY = ../lib/libwconr/build/libwconr.so
	endif
endif


all:
	mkdir -p ./build
	cp $(LIBRARY) ./build/
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
