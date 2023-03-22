#pragma once

// Sets bit n of num to the boolean b
#define SET_BIT(num,n,b) (num = (num & ~(1UL << n)) | (b << n));