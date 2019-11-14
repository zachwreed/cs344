# ** Zach Reed
# ** CS-325
# ** Makefile for adventure
# *********************************************

CXX = gcc	#enables gcc as compiler

#target: dependencies
#	action

# Executable output, dependencies, and action ***************************
smallsh: smallsh.o
	$(CXX) smallsh.o -o smallsh

#tells compiler to create object without executable when modified
smallsh.o: smallsh.c
	$(CXX) -c -g smallsh.c

#Removes object files and output ****************************************
clean:
	rm *.o smallsh
