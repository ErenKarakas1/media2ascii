CFLAGS = -Wall -Wextra -I./include -std=c++20 -O3
LDFLAGS = -lavcodec -lavformat -lavutil -lswscale
CXX = g++

common = src/ascii_lib.cpp src/stb_impl.cpp

all: vid2ascii img2ascii

vid2ascii: src/vid2ascii.cpp ${common}
	$(CXX) src/vid2ascii.cpp ${common} $(CFLAGS) ${LDFLAGS} -o vid2ascii

img2ascii: src/img2ascii.cpp ${common}
	$(CXX) src/img2ascii.cpp ${common} $(CFLAGS) -o img2ascii
