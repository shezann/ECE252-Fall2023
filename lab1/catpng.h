#include "zutil.h"
#include "lab_png.h"

simple_PNG_p concatenate_pngs(simple_PNG_p* pngs, U64 size);

U64 calculate_buffer_size(U32 png_width, U32 arr_size);