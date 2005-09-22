
#include <stdio.h>
#include "stdint.hxx"

int main() 
{
   uint16_t sui16, ui16 = 0x0123;
   uint32_t sui32, ui32 = 0x01234567;
   uint64_t sui64, ui64 = 0x0123456789ABCDEFLL;

   sui16 = ui16; sgEndianSwap(&sui16);
   sui32 = ui32; sgEndianSwap(&sui32);
   sui64 = ui64; sgEndianSwap(&sui64);

   printf("\nUI16: %x (normal)\t\t %x (swapped)\n", ui16, sui16 );
   printf("UI32: %x (normal)\t\t %x (swapped)\n", ui32, sui32 );
   printf("UI64: %llx (normal)\t %llx (swapped)\n\n", ui64, sui64 );

   return 0;
}
