/***************************************
 ** Author: Zach Reed
 ** Description: Smallsh
 ** Date: 11/20/2019
 ****************************************/

# How to Compile 
	There are two ways to compile the program:
	1.) 	Included in the folder is a makefile. 
		Simply run "$ make" in the command line and the executable will be ready for testing.

	2.)	In the command line run "$ gcc smallsh.c -o smallsh -lm" to create the executable.

	The program utilizes the math library, so the "-lm" flag is required for compilation.
