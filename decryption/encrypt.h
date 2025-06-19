#pragma once

#include "shared.h"

//Encrypter thread function - any other function in the c file is a helper to this function.
//Therefore, we don't want to give access to these functions from outside the c file
void* encrypter(void* arg);