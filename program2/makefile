# ** Zach Reed							      
# ** CS-325							      	 
# ** Makefile for adventure				   
# *********************************************

CXX = gcc	#enables gcc as compiler

#target: dependencies
#	action

# Executable output, dependencies, and action *************************** 
reedz.buildrooms: reedz.buildrooms.o
	$(CXX) reedz.buildrooms.o -o reedz.buildrooms

#tells compiler to create object without executable when modified 
reedz.buildrooms.o: reedz.buildrooms.c
	$(CXX) -c reedz.buildrooms.c 

#Removes object files and output ****************************************
clean:
	rm *.o reedz.buildrooms
