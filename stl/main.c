/*
 ** main.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include	"user.h"
#include	"user_initialize.h"
#include	"user_terminate.h"
#include    "user_types.h"
#include    "stl.h"

int 
main(int argc, char **argv)
{
    // initialize the thread library    
    stl_initialize(argc, argv);

    // call the user's function
    user_initialize();
    user();
    user_terminate();
    
    return 0;
}
