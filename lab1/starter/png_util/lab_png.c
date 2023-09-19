#include "lab_png.h"

int is_png(U8 *buf, size_t n) {
    if (n < 8) 
        return 0;

    return ((U64 *) buf)[0] == PNG_HEADER;
}

int get_png_height(struct data_IHDR *buf) {

}

int get_png_width(struct data_IHDR *buf) {

}

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {

}

int main() {
    U8* buf = malloc(10*sizeof(U8));
    buf[0] = 137;
    buf[1] = 80;
    buf[2] = 78;
    buf[3] = 71;
    buf[4] = 13;
    buf[5] = 10;
    buf[6] = 26;
    buf[7] = 10;

    U64 a = PNG_HEADER;

    printf("The value of buf: %ld\n", ((U64 *) buf)[0]);
    printf("The value of header: %ld\n", a);
    
    printf("is png: %d\n", is_png(buf, 10));

    free(buf);
    return 0;
}