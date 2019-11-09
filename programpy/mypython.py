#------------------------------------------------
# -- Author: Zach Reed
# -- Date: 5/20/2019
# -- Description: program py file
# -----------------------------------------------

 # -----------------------------------------
 # -- Citation
 # -- Author: Zach Reed
 # -- Date: 5/8/2019
 # -- Title: reedz.adventure.c
 # -- Description: Python program from Spring 2019 term
 # -----------------------------------------

import random
import string
import sys

nChar = 10
nFile = 3
#---------------------------
# Generate File
# Description:      Generate files with content
# Preconditions:    Files do not exist
# Postconditions:   Files created and populated with content
#---------------------------
def gen_file() :
    for x in range(nFile) :
        fname = "mypyFile" + str(x)
        file = open(fname, "w+")

        content = gen_rand_array() + "\n"
        sys.stdout.write(content)
        file.write(content)
        file.close()

#---------------------------
# Generate Random Numbers
# Description:      Create random integers and their product written to stdout
# Preconditions:    none
# Postconditions:   3 lines written to stdout
# --------------------------
def gen_rand_nums() :
    num1 = random.randint(1, 42)
    num2 = random.randint(1, 42)
    print(num1)
    print(num2)
    print(num1 * num2)

#---------------------------
# Generate Random Array
# Description:      Creates file contents
# Preconditions:    called within gen_file
# Postconditions:   returns string of random lowercase letters
# --------------------------
def gen_rand_array() :
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for x in range(nChar))


gen_file()
gen_rand_nums()
