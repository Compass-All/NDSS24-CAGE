include ../_/make.config


OPENCL_INC= -I. -Iinclude -I/home/yunjiedeng/clean/ComputeLibrary/include -I../_
OPENCL_LIB= -larm_compute-static -larm_compute_core-static -lOpenCL
OPENCL_LIN= -L/home/yunjiedeng/clean/ComputeLibrary/build -L/home/yunjiedeng/clean/ComputeLibrary/build/opencl-1.2-stubs

CC = aarch64-linux-android-clang++
CC_FLAGS = -g -O3 -Wall

SRC = lud.cpp common.c ../_/timing.c ../_/goodutil.c

CONFEXE = conf_lud.out

INC = $(OPENCL_INC) -I. -Iinclude -I../_
LIB = $(OPENCL_LIB)
LIN = $(OPENCL_LIN)

all:
	$(CC) $(SRC) $(CC_FLAGS) $(INC) $(LIB) $(LIN) -o $(CONFEXE) -march=armv8-a+crypto -Wno-unused-result  -fopenmp -DTIMING -DCONFIDENTIAL
clean:
	rm -f ./$(CONFEXE)
