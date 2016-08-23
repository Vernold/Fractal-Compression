#include "Conversion.h"

DomainPool::DomainPool(ImageBlock &image, size_t size): im(image), r_sz(size)
{
    pool = array<DomainList, R_range + 1>();
    
    for (uint i = 0; i < R_range + 1; i++){
        pool[i] = DomainList();
    }
    creat_pool(im.domain_image());
}

void DomainPool::creat_pool(ImageBlock d_im)
{
    uint n = d_im.size() - r_sz;
    
    for (uint i = 0; i < n; i += 2){
        for (uint j = 0; j < n; j += 2){
            Block block = d_im.sub_block(i, j, r_sz);
            uchar Rd = block.get_Rd();
            pool[Rd].push_back(make_pair(block, make_pair(i, j)));
        }
    }
}

DomainList DomainPool::operator[](int i)
{
    int r_dev = i,
    l_dev = i;
    
    while (pool[r_dev].empty() && pool[l_dev].empty() && (r_dev - l_dev) < R_range){
        if (r_dev < R_range) r_dev++;
        if (l_dev > 0) l_dev--;
    }
    if (!pool[r_dev].empty())
        return pool[r_dev];
    else
        return pool[l_dev];
}

ImageBlock ImageBlock::domain_image()
{
    ImageBlock d_im = ImageBlock(size() / 2);
    
    for (uint i = 0; i < size(); i += 2){
        for (uint j = 0; j < size(); j += 2){
            vector<uchar> pxls = vector<uchar>();
            if (i != 0 &&  j != 0){
                pxls.push_back(block(i - 1, j - 1));
            }
            if (i != 0){
                pxls.push_back(block(i - 1, j));
                pxls.push_back(block(i - 1, j + 1));
            }
            if (j != 0){
                pxls.push_back(block(i, j - 1));
                pxls.push_back(block(i + 1, j - 1));
            }
            pxls.push_back(block(i + 1, j));
            pxls.push_back(block(i + 1, j + 1));
            pxls.push_back(block(i, j + 1));
            pxls.push_back(block(i + 1, j + 1));
            
            uint sum = 0;
            for (uchar pxl : pxls) sum += pxl;
            d_im(i / 2, j / 2) = sum / pxls.size();
        }
    }
    return d_im;
}

/* Block Methods */

//DCT
tuple<float, float> Block::get_DCT(int k)
{
    float Ck_ = 0, C_k = 0;
    size_t b_sz = size();
    
    for (uint i = 0; i < b_sz; i++){
        for (uint j = 0; j < b_sz; j++){
            Ck_ += block(i, j) * cos((k * M_PI * (2 * i + 1)) / (2 * b_sz));
            C_k += block(i, j) * cos((k * M_PI * (2 * j + 1)) / (2 * b_sz));
        }
    }
    
    return make_tuple(Ck_, C_k);
}

uchar Block::get_BCI()
{
    float C3_, C_3;
    uchar BCI = 0;
    
    tie(C3_, C_3) = get_DCT(3);
    
    if (abs(C3_) < abs(C_3)) BCI |= 1 << 2;
    if (C3_ < 0) BCI |= 1 << 1;
    if (C_3 < 0) BCI |= 1;
    
    return BCI;
}

uchar Block::get_Rd()
{
    float C1_, C_1;
    
    tie(C1_, C_1) = get_DCT(1);
    if (abs(C1_) >= abs(C_1)){
        return R_range * abs(C_1 / C1_);
    }
    else{
        return R_range * abs(C1_ / C_1);
    }
}

void Block::rotate()
{
    int n = size();
    
    for (int layer = 0; layer < n / 2; ++layer) {
        int first = layer;
        int last = n - 1 - layer;
        for(int i = first; i < last; ++i) {
            int offset = i - first;
            // save top
            int top = block(first, i);
            // left -> top
            block(first, i) = block(last-offset, first);
            // bottom -> left
            block(last-offset, first) = block(last, last - offset);
            // right -> bottom
            block(last, last - offset) = block(i, last);
            // top -> right
            block(i, last) = top;
        }
    }
}

void Block::swap(uchar &x, uchar &y)
{
    x = x ^ y;
    y = x ^ y;
    x = x ^ y;
}

void Block::reflect()
{
    uint n = size();
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n >> 1; j++){
            swap(block(i, j), block(i, n - j - 1));
        }
    }
}

void Block::transform(uint sym)
{
    for (uint i = 0; i <= sym ; i++){
        rotate();
        if (i == 3){
            reflect();
        }
    }
}

void Block::compute_mean()
{
    uint n = size();
    mn = 0;
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            mn += block(i, j);
        }
    }
    mn /= (n * n);
}

void Block::compute_variance()
{
    size_t n = size();
    vr = 0;
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            int summand = (block(i, j) - mn);
            vr += (summand * summand);
        }
    }
    vr /= (n * n);
}


/* Linear Quantization Functions */

uchar LQF(float L, float U, int n_bits, float x)
{
    uchar res = 0;
    
    if (x == 0.0) return 0;
    for (int i = 0; i < n_bits; i++){
        res <<= 1;
        if ( (L + U) / 2 < x){
            res |= 1;
            L = (L + U) / 2;
        }
        else{
            U = (L + U) / 2;
        }
    }
    return res;
}

float LDF(float L, float U, int n_bits, uchar x)
{
    if (x == 0) return 0.0;
    for (int i = n_bits - 1; i >= 0; i--){
        if ( x & 1 << i){
            L = (L + U) / 2;
        }
        else{
            U = (L + U) / 2;
        }
    }
    return (L + U) / 2;
}


/* RGB to ...*/

ImageBlock RGB_to_Y(Image im)
{
    uint n = im.n_rows;
    ImageBlock y_im = ImageBlock(n);
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            uchar r, g, b;
            tie(r, g, b) = im(i, j);
            int y = ((76 * r + 150 * g + 29 * b) + 128) >> 8;
            y_im(i, j) = y;
            
        }
    }
    y_im.compute_sqr_mean();
    return y_im;
}

ImageBlock RGB_to_U(Image im)
{
    uint n = im.n_rows;
    ImageBlock u_im = ImageBlock(n);
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            uchar r, g, b;
            tie(r, g, b) = im(i, j);
            int u =((-43 * r - 84 * g + 127 * b + 128) >> 8) + 128;
            u_im(i, j) = u;
        }
    }
    u_im.compute_sqr_mean();
    return u_im;
}

ImageBlock RGB_to_V(Image im)
{
    uint n = im.n_rows;
    ImageBlock v_im = ImageBlock(n);
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            uchar r, g, b;
            tie(r, g, b) = im(i, j);
            int v = ((127 * r - 106 * g - 21 * b + 128) >> 8) + 128;
            v_im(i, j) = v;
        }
    }
    v_im.compute_sqr_mean();
    return v_im;
}

Image Y_U_V_to_RGB(ImageBlock y_im, ImageBlock u_im, ImageBlock v_im)
{
    Image im = Image(y_im.size(), y_im.size());
    uchar r, g, b;
    
    for (uint i = 0; i < im.n_rows; i++){
        for (uint j = 0; j < im.n_cols; j++){
            r = y_im(i, j) + 1.402 * (v_im(i, j) - 128);
            g = y_im(i, j) - 0.34414 * (u_im(i, j) - 128) - 0.71414 * (v_im(i, j) - 128);
            b = y_im(i, j) + 1.772 * (u_im(i, j) - 128);
            im(i, j) = make_tuple(r, g, b);
        }
    }
    return im;
}

/* io */

Image load_image(const char *path)
{
    BMP in;
    
    if (!in.ReadFromFile(path))
        throw string("Error reading file ") + string(path);
    
    Image res(in.TellHeight(), in.TellWidth());
    
    for (uint i = 0; i < res.n_rows; ++i) {
        for (uint j = 0; j < res.n_cols; ++j) {
            RGBApixel *p = in(j, i);
            res(i, j) = make_tuple(p->Red, p->Green, p->Blue);
        }
    }
    
    return res;
}

void save_image(const Image &im, const char *path)
{
    BMP out;
    out.SetSize(im.n_cols, im.n_rows);
    
    uchar r, g, b;
    RGBApixel p;
    p.Alpha = 255;
    for (uint i = 0; i < im.n_rows; ++i) {
        for (uint j = 0; j < im.n_cols; ++j) {
            tie(r, g, b) = im(i, j);
            p.Red = r; p.Green = g; p.Blue = b;
            out.SetPixel(j, i, p);
        }
    }
    if (!out.WriteToFile(path))
        throw string("Error writing file ") + string(path);
}


/* ImageBlock */

void ImageBlock::compute_sqr_mean()
{
    size_t n = size();
    sqr_mn = 0.0;
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            sqr_mn += block(i, j) * block(i, j);
        }
    }
    sqr_mn /= (n * n);
}

/* for dubugging */
void save_block(Block &im, const char *path)
{
    BMP out;
    out.SetSize(im.size(), im.size());
    
    RGBApixel p;
    p.Alpha = 255;
    for (uint i = 0; i < im.size(); ++i) {
        for (uint j = 0; j < im.size(); ++j) {
            p.Red = im(i, j);
            p.Green = im(i, j);
            p.Blue = im(i, j);
            out.SetPixel(j, i, p);
        }
    }
    if (!out.WriteToFile(path))
        throw string("Error writing file ") + string(path);
}



