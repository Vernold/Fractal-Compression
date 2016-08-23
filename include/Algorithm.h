

#ifndef first_part_h
#define first_part_h

#include "Conversion.h"
#include "AC.h"

class FIC
{
    Block R;
    Block D;
    uint min_mse = MaxMSE;
    
    uint h;
    uint x = 0, y = 0;
    uint sym = 0;
    float s = 0.0, o = 0.0;
    
public:
    FIC() {}
    FIC (ulong data) { dequantize(data); }
    
    //Getters
    uint X() { return x; }
    uint Y() { return y; }
    uint symmetry() { return sym; }
    uint H() { return h; }
    
    //Setters
    void set_range (Block b) { R = b; }
    void set_domain (Block b) { D = b; }
    void set_min_mse(ulong mse) { min_mse = mse; }
    void set_h(uint H) { h = H; }
    
    //Computations
    void compute_s();
    void compute_sym();
    ulong compute_error();
    
    ulong quantize();
    void dequantize(ulong data);
    
    void find_block(DomainPool &pool);
    void handle_block();

    bool check_block(uint k, uint l, uint q){ return min_mse <= (l - k * q); }
};

/* Compression */

void fr_encode(DomainPool *, DataBase &, Block &, Block &, Block &, uint, uint);

void Compress(Image, const char *, size_t, uint );

/* Decompression */

void fr_decode(DataBase &, Block &, Block &, uint, Color);

Image Decompress(const char *compressed_file_name);

#endif /* first_part_h */

