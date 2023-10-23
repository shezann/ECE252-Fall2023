#include "zutil.h"
#include "lab_png.h"

/**
 * @brief     Concatenate PNGs into one PNG vertically
 * 
 * @param[in] pngs The array of PNGs to concatenate
 * @param[in] size The size of the array
 * @return    The concatenated PNG
 * @note     The caller is responsible to call the destory_png method to prevent memory leak.
*/
simple_PNG_p concatenate_pngs(simple_PNG_p* pngs, U64 size);

/**
 * @brief      Calculate the buffer size for the concatenated PNG
 * 
 * @param[in]  png_width  The width of the PNG
 * @param[in]  png_height The height of the PNG
 * @return     The uncompressed buffer size to store the concatenated PNG
*/
U64 calculate_buffer_size(U32 png_width, U32 png_height);