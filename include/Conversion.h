#pragma once

#include "matrix.h"
#include "EasyBMP.h"

#include <tuple>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <array>
#include <list>
#include <queue>

#define MaxMSE 255 * 255 * 256
#define R_range 15
#define TH  50
#define HP 50
#define SD 40
#define MaxSize 16
#define MinHeight 1
#define IF 0.8
#define Ra 0.03

#define FLAG 0xFF

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef Matrix<std::tuple<uchar, uchar, uchar>> Image;
typedef Matrix<uchar> MonoChrome;

using std::tuple;
using std::make_tuple;
using std::tie;
using std::pair;
using std::endl;
using std::cout;
using std::vector;
using std::abs;
using std::array;
using std::make_pair;
using std::string;
using std::queue;
using std::cerr;

/* Color */

enum Color {Y, U, V};

/* Block */
class Block
{
protected:
    MonoChrome block;
    
    float mn;
    ulong vr;
    
public:
    Block(MonoChrome b): block(b)
    {
        this->compute_mean();
        this->compute_variance();
    }
    Block(size_t sz = 1): mn(0), vr(0)
        { block = MonoChrome(sz, sz); }
    
    bool is_homogeneous() { return vr <= HP; }
    
    /* DCT */
    tuple<float, float> get_DCT(int);
    uchar get_BCI();
    uchar get_Rd();
    
    /* Clock Conversions */
    void rotate();
    void reflect();
    void transform(uint sym);
    
    /* Getters */
    size_t size() { return block.n_rows; }
    float mean() { return mn; }
    float var() { return vr; }
    
    uchar &operator() (uint x, uint y)
        { return block(x, y); }
    MonoChrome submatrix(uint x, uint y, uint rows, uint cols) const
        { return block.submatrix(x, y, rows, cols); }
    Block deep_copy() const
        { return Block(block.deep_copy()); }
    const Block sub_block(uint x, uint y, size_t sz) const
        { return Block(block.submatrix(x, y, sz, sz)); }
    
private:
    /* Computations */
    void swap(uchar &, uchar &);
    void compute_mean();
    void compute_variance();
};

/* ImageBlock */

class ImageBlock: public Block
{
    float sqr_mn;
    
public:
    ImageBlock(size_t size): Block(size), sqr_mn(0.0){}

    float sqr_mean() { return sqr_mn; }
    void compute_sqr_mean();
    ImageBlock domain_image();
};

/* DomainPool */

typedef pair<uint, uint> Point;
typedef vector<pair<Block, Point>> DomainList;

class DomainPool
{
    ImageBlock im = ImageBlock(1);
    size_t r_sz;
    
    array<DomainList, R_range + 1> pool;
    
public:
    DomainPool(){}
    DomainPool(ImageBlock &im, size_t r_sz);
    
    DomainList operator[](int i);
    
    ImageBlock image() { return im; }
private:
    void creat_pool(ImageBlock );
};

// Linear Quantization Functions

uchar LQF(float, float, int, float ); // Direct F

float LDF(float, float, int, uchar );// Inverse F

void swap(uchar &, uchar &);

// RGB to ...

ImageBlock RGB_to_Y(Image );

ImageBlock RGB_to_U(Image );

ImageBlock RGB_to_V(Image );

Image Y_U_V_to_RGB(ImageBlock, ImageBlock, ImageBlock );

// io

Image load_image(const char *);

void save_image(const Image &, const char *);

void save_block(Block &, const char *);

/* Conversion_h */

