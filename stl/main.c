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

#ifdef typedef_userStackData
userStackData  SD;
#endif
#ifdef typedef_userPersistentData
userPersistentData pd;
#endif

int 
main(int argc, char **argv)
{
    // initialize the thread library    
    stl_initialize(argc, argv);

    // initialize stack data required for some MATLAB functions
#ifdef typedef_userStackData
    #ifdef typedef_userPersistentData
    	SD.pd = &pd;
    #endif
    user_initialize(&SD);
#else
    user_initialize();
#endif

    // call the user's function
    user(); 
    // user(&SD);  use this if user has a stack pointer argument


    user_terminate();
    
    return 0;
}
