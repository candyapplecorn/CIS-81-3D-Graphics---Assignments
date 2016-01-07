#OBJ_NAME specifies the name of our exectuable 
OBJ_NAME = 81_Assignment_1

#OBJS specifies which files to compile as part of the project 
OBJS := $(wildcard *.cpp)

#CC specifies which compiler we're using 
CC = g++ 

#COMPILER_FLAGS specifies the additional compilation options we're using 
# -w suppresses all warnings 
COMPILER_FLAGS =-std=c++11

#LINKER_FLAGS specifies the libraries we're linking against 
LINKER_FLAGS =-lGL -lGLU -lglut

#This is an additional directory to search for header files
HEADER_FILES =-I/usr/include/GL -I$(shell pwd)

#This is the target that compiles our executable 
all : $(OBJS) 
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) $(HEADER_FILES) -o $(OBJ_NAME) 
