# ** Zach Reed
# ** CS-325
# ** Makefile for adventure
# *********************************************

CXX = gcc	#enables gcc as compiler

#target: dependencies
#	action

# Executable output, dependencies, and action ***************************
all: reedz.buildrooms reedz.adventure

reedz.buildrooms: reedz.buildrooms.o
	$(CXX) reedz.buildrooms.o -o reedz.buildrooms

#tells compiler to create object without executable when modified
reedz.buildrooms.o: reedz.buildrooms.c
	$(CXX) -c -g reedz.buildrooms.c

reedz.adventure: reedz.adventure.o
	$(CXX) reedz.adventure.o -o reedz.adventure -lpthread

#tells compiler to create object without executable when modified
reedz.adventure.o: reedz.adventure.c
	$(CXX) -c -g reedz.adventure.c

#Removes object files and output ****************************************
clean:
	rm *.o reedz.buildrooms reedz.adventure
