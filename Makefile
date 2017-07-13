
APP  := test
SRCS := si1145.c test.c
OBJS := $(SRCS:.c=.o)

LDFLAGS  := -L../i2c_spa
LDLIBS   := -li2c_spa
CPPFLAGS := -I../i2c_spa/include -D PLATFORM_LINUX

.PHONY: all app clean

all: ${APP}

${APP}: ${OBJS}

clean:
	rm -f ${APP}
	rm -f ${OBJS}

