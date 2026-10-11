/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    random.c

Abstract:

    Runtime Library Pseudo-Random Number Generator.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode and User-mode.

--*/

#include "../inc/rtl.h"

/*
 * Linear Congruential Generator (Park-Miller standard minimal):
 * X_{n+1} = (a * X_n + c) mod m
 * Using a = 2147001325, c = 715136305
 */
ULONG
RtlRandom(
    PULONG Seed
    )
{
    ULONGLONG Temp;
    ULONG NewSeed;

    if (Seed == NULL) {
        return 0;
    }

    if (*Seed == 0) {
        *Seed = 0x1337CAFEUL; 
    }

    Temp = (ULONGLONG)(*Seed) * 2147001325UL + 715136305UL;
    
    NewSeed = (ULONG)(Temp & 0x7FFFFFFFUL);
    *Seed = NewSeed;
    
    return NewSeed;
}
