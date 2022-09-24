/*

  Hello world application using ubxlib

*/

#include "ubxlib.h"

void main()
{
    uPortInit();
    int i = 0;
    while (1) {
        uPortLog("Hello #%3d\n", ++i);
        uPortTaskBlock(5000);
    }
}
