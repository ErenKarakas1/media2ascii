CFLAGS = -Wall -Wextra -I./include -std=c++23 -O3
LDFLAGS = -lavcodec -lavformat -lavutil -lswscale
CXX = g++

LIBS = src/ascii_lib.cpp src/stb_impl.cpp

all: vid2ascii img2ascii

vid2ascii: src/vid2ascii.cpp ${LIBS}
	$(CXX) src/vid2ascii.cpp ${LIBS} $(CFLAGS) ${LDFLAGS} -o vid2ascii

img2ascii: src/img2ascii.cpp ${LIBS}
	$(CXX) src/img2ascii.cpp ${LIBS} $(CFLAGS) -o img2ascii
