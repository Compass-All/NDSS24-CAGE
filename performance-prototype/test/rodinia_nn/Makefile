include ../_/make.config

# [NOTE]: Setup your Arm OpenCL path here
OPENCL_INC= 
OPENCL_LIB= 
OPENCL_LIN=

CC = aarch64-linux-android-clang++

SRC =  nearestNeighbor.cpp clutils.cpp utils.cpp ../_/goodutil.c
CC_FLAGS = -g -O3
CONFEXE = conf_nn.out


INC = $(OPENCL_INC) -I. -Iinclude -I../_
LIB = $(OPENCL_LIB)
LIN = $(OPENCL_LIN)		
		
all:
	$(CC) $(SRC) $(CC_FLAGS) $(INC) $(LIB) $(LIN) -o $(CONFEXE) -march=armv8-a+crypto -Wno-unused-result  -fopenmp -DTIMING -DCONFIDENTIAL

clean:
	rm -f $(CONFEXE)

