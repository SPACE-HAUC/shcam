CC=gcc
CXX=g++
RM= /bin/rm -vf
PYTHON=python3
ARCH=UNDEFINED
PWD=pwd
CDR=$(shell pwd)

EDCFLAGS:=$(CFLAGS)
EDLDFLAGS:=$(LDFLAGS)
EDDEBUG:=$(DEBUG)

ifeq ($(ARCH),UNDEFINED)
	ARCH=$(shell uname -m)
endif

UNAME_S := $(shell uname -s)

CXXFLAGS:= -I include/ -I drivers/ -I include/imgui -I ./ -Wall -O2 -fpermissive
LIBS = 

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

EDCFLAGS:= -Wall -std=gnu11 $(EDCFLAGS)
EDLDFLAGS:= -lm -lpthread $(EDLDFLAGS)

LIBS += -lm -lpthread

all: EDCFLAGS:= -O3 $(EDCFLAGS)
test_ucam: EDCFLAGS:= -O2 $(EDCFLAGS)

BUILDDRV=drivers/shserial/shserial.o \
drivers/gpiodev/gpiodev.o 

BUILDGUIBASE=drivers/imgui_backends/imgui_impl_glfw.o \
drivers/imgui_backends/imgui_impl_opengl2.o \
src/imgui.o \
src/imgui_demo.o \
src/imgui_draw.o \
src/imgui_widgets.o

BUILDGUI= $(BUILDGUIBASE) \
src/guimain.o

BUILDOBJS=$(BUILDDRV) \
src/ucam3.o

UCAMTARGET=adar_tester.out
GUITARGET=main.out

all: $(GUITARGET)
	@echo Finished building $(GUITARGET) for $(ECHO_MESSAGE)
	sudo ./$(GUITARGET)

test_ucam: $(UCAMTARGET)

$(GUITARGET): $(BUILDOBJS) $(BUILDGUI)
	$(CXX) $(BUILDOBJS) $(BUILDGUI) -o $(GUITARGET) $(CXXFLAGS) $(LIBS)

$(UCAMTARGET): $(BUILDOBJS)
	$(CC) $(BUILDOBJS) $(EDCFLAGS) -Iinclude/ -Idrivers/ -I./ $(LINKOPTIONS) -o $@ \
	$(EDLDFLAGS)

%.o: %.c
	$(CC) $(EDCFLAGS) $(EDDEBUG) -Iinclude/ -Idrivers/ -I./ -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) $(BUILDOBJS)
	$(RM) src/guimain.o
	$(RM) $(UCAMTARGET)
	$(RM) $(GUITARGET)

spotless: clean
	$(RM) $(BUILDGUI)

