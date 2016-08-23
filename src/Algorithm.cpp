
#include "Algorithm.h"

/* FIC Methods */

void FIC::compute_s()
{
    size_t n = R.size();
    float R_D = 0, D_D = 0;
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            R_D += (R(i, j) - R.mean()) * (D(i, j) - D.mean());
            D_D += (D(i, j) - D.mean()) * (D(i, j) - D.mean());
        }
    }
    s = R_D / D_D;
}

void FIC::compute_sym()
{
    Matrix<uchar> table =
    {   {0, 6, 4, 2, 5, 1, 3, 7},
        {6, 0, 2, 4, 1, 5, 7, 3},
        {4, 2, 0, 6, 3, 7, 5, 1},
        {2, 4, 6, 0, 7, 3, 1, 5},
        {5, 1, 3, 7, 0, 6, 4, 2},
        {1, 5, 7, 3, 6, 0, 2, 4},
        {3, 7, 5, 1, 4, 2, 0, 6},
        {7, 3, 1, 5, 2, 4, 6, 0} };
    uchar BCI_r = R.get_BCI(),
          BCI_d = D.get_BCI();
    
    sym = table(BCI_r, BCI_d);
    D.transform(sym);
}

ulong FIC::compute_error()
{
    size_t n = R.size();
    long long mse = 0;
    float summand = 0;
    float S = LDF(-1, 1, 2, LQF(-1, 1, 2, s));
    
    for (uint i = 0; i < n; i++){
        for (uint j = 0; j < n; j++){
            summand = S * (D(i, j) - D.mean()) - (R(i, j) - R.mean());
            mse += summand * summand;
        }
    }
    return mse / (n * n);
}

void FIC::find_block(DomainPool &pool)
{
    if (R.is_homogeneous()) return;
    
    DomainList d_list = pool[R.get_Rd()];
    
    for (auto it = d_list.begin(); it != d_list.end(); it++){
        
        if (abs(R.var() - it->first.var()) >= SD){
            continue;
        }
        set_domain(it->first.deep_copy());
        Point p = it->second;
        
        compute_sym();
        compute_s();
        
        long mse = compute_error();
        
        if (mse < TH){
            min_mse = mse;
            x = p.first / 2;
            y = p.second / 2;
            return;
        }
        if (mse < min_mse){
            min_mse = mse;
            x = p.first / 2;
            y = p.second / 2;
        }
    }
}

void FIC::handle_block()
{
    size_t r_sz = R.size();
    
    D.transform(sym);
    for (uint i = 0; i < r_sz; i++){
        for (uint j = 0; j < r_sz; j++){
            int value = s * (D(i, j) - D.mean()) + o;
            if (value < 0) value = 0;
            if (value > 255) value = 255;
            R(i, j) = value;
        }
    }
}

ulong FIC::quantize()
{
    ulong res = 0;
    
    if (s != 0.0) res |= 1 << 27;
    res |= x << 20; // 7 bits
    res |= y << 13; // 7 bits
    res |= sym << 10; // 3 bits
    res |= LQF(-1, 1, 2, s) << 8; // 2 bits
    res |=  h << 6; // 2 bits
    res |= LQF(0, 256, 6, R.mean());  // 6 bits
    
    return res;
}

void FIC::dequantize(ulong data)
{
    x = (data & 127 << 20) >> 20;
    y = (data & 127 << 13) >> 13;
    sym = (data & 7 << 10) >> 10;
    s = LDF(-1, 1, 2, (data & 3 << 8) >> 8);
    h = ((data & 3 << 6) >> 6);
    o = LDF(0, 256, 6, data & 63);
}

/* Compress */

void fr_encode(DomainPool *pool, DataBase &data, Block &y, Block &u, Block &v, uint h, uint q = 0)
{
    size_t r_sz = 1 << (h + 1);
    size_t b_sz = y.size();
    uint MinH = 1, k, l;
    
    if (q == 0) { k = 0; l = 50; }
    else if (q <= 35) { MinH = 2; k = 100; l = 3600;}
    else if (q > 75) { k = 5; l = 500; }
    else { k = 4; l = 400;}
    
    for (uint i = 0; i < b_sz; i += r_sz){
        for (uint j = 0; j < b_sz; j += r_sz){
            Block y_R = y.sub_block(i, j, r_sz),
                  u_R = u.sub_block(i, j, r_sz),
                  v_R = v.sub_block(i, j, r_sz);
            FIC y_fic, u_fic, v_fic;
            
            y_fic.set_h(h);
            u_fic.set_h(h);
            v_fic.set_h(h);
            
            y_fic.set_range(y_R);
            u_fic.set_range(u_R);
            v_fic.set_range(v_R);
            
            y_fic.find_block(pool[h]);
            
            if (y_fic.check_block(k, l, q)){
                data.put_data (y_fic.quantize(), Color::Y);
                data.put_data (u_fic.quantize(), Color::U);
                data.put_data (v_fic.quantize(), Color::V);
            }
            else if (h == MinH || q == 0){
                data.put_data (y_fic.quantize(), Color::Y);
                data.put_data (u_fic.quantize(), Color::U);
                data.put_data (v_fic.quantize(), Color::V);
            }
            else{
                fr_encode(pool, data, y_R, u_R, v_R, h - 1, q);
            }
        }
    }
}

void Compress(Image im, const char *compressed_file_name, size_t b_sz, uint q = 0)
{
    if (q != 0) b_sz = MaxSize;
    if (q > 75) b_sz = MaxSize >> 1;
    
    DataBase data = DataBase(compressed_file_name, "wb");
    uint h = log2(b_sz) - 1;
    data.set_sizes(im.n_rows, b_sz);
    
    ImageBlock y_im = RGB_to_Y(im);
    ImageBlock u_im = RGB_to_U(im);
    ImageBlock v_im = RGB_to_V(im);
    
    DomainPool *pool = new DomainPool[4];
    
    for (int i = 0; i < 4; i++){
        pool[i] = DomainPool(y_im, 1 << (i + 1));
    }
    fr_encode(pool, data, y_im, u_im, v_im, h, q);
    
    data.ac_encode();
}

/* Decompress */

void fr_decode(DataBase &data, Block &block, Block &d_im, uint h, Color c)
{
    size_t b_sz = block.size();
    size_t r_sz = 1 << (h + 1);
    
    for (uint i = 0; i < b_sz; i += r_sz){
        for (uint j = 0; j < b_sz; j += r_sz){
            FIC fic = FIC (data.get_data(c));
            Block R = block.sub_block(i, j, r_sz);
            
            if (fic.H() == h){
                uint d_x, d_y;
                
                if (c == Color::Y){
                    d_x = fic.X() * 2;
                    d_y = fic.Y() * 2;
                }
                else{
                    d_x = i;
                    d_y = j;
                }
                Block D = d_im.sub_block (d_x, d_y, r_sz);
                fic.set_range(R);
                fic.set_domain(D.deep_copy());
                fic.handle_block();
                data.next(c);
            }
            else fr_decode(data, R, d_im, h - 1, c);
        }
    }
}


Image Decompress(const char *compressed_file_name)
{
    DataBase data = DataBase(compressed_file_name, "rb");
    data.ac_decode();
    uint b_sz = data.range_size();
    uint h = log2(b_sz) - 1;
    
    ImageBlock y_im = ImageBlock(data.image_size());
    ImageBlock u_im = ImageBlock(data.image_size());
    ImageBlock v_im = ImageBlock(data.image_size());
    ImageBlock d_im = ImageBlock(data.image_size());
    
    for (int i = 0; i < 14; i++){
        fr_decode(data, y_im, d_im, h, Color::Y);
        d_im = y_im.domain_image();
    }
    cout << "Y - component is decompressed" << endl;
    fr_decode(data, u_im, y_im, h, Color::U);
    cout << "U - component is decompressed" << endl;
    fr_decode(data, v_im, y_im, h, Color::V);
    cout << "V - component is decompressed" << endl;
    
    return Y_U_V_to_RGB(y_im, u_im, v_im);
}


