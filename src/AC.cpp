#include <stdlib.h>
#include "AC.h"

void AcEncoder::output_bit(int bit)
{
    buffer >>= 1;
    if (bit)
        buffer |= 0x80;
    bits_to_go -= 1;
    total_bits += 1;
    if (bits_to_go==0)  {
        if (fp)
            putc (buffer, fp);
        bits_to_go = 8;
    }
}

void AcEncoder::bit_plus_follow(int bit)
{
    output_bit (bit);
    while (fbits > 0)  {
        output_bit (!bit);
        fbits -= 1;
    }
}

int AcDecoder::input_bit()
{
    if (bits_to_go == 0)  {
        buffer = getc(fp);
        if (buffer == EOF)  {
            garbage_bits += 1;
            if (garbage_bits > Code_value_bits - 2)
                cerr << "AD: bad input file" << endl;
        }
        bits_to_go = 8;
    }
    
    int tmp = buffer & 1;
    buffer >>= 1;
    bits_to_go -= 1;
    
    return tmp;
}

void AcModel::update_model(int sym)
{
    if (cfreq[0] == MaxFreq)  {
        int cum = 0;
        cfreq[nsym] = 0;
        for (int i = nsym - 1; i >= 0; i--)  {
            freq[i] = (freq[i] + 1) / 2;
            cum += freq[i];
            cfreq[i] = cum;
        }
    }
    
    freq[sym] += 1;
    for (int i = sym; i >= 0; i--)
        cfreq[i] += 1;
}

AcEncoder::AcEncoder(FILE *fd)
{
    fp = fd;
    bits_to_go = 8;
    
    low = 0;
    high = Top_value;
    fbits = 0;
    buffer = 0;
    
    total_bits = 0;
}

AcEncoder::~AcEncoder()
{
    fbits += 1;
    if (low < First_qtr)
        bit_plus_follow (0);
    else
        bit_plus_follow (1);
    if (fp)
        putc (buffer >> bits_to_go, fp);
    
    if (fp)
        fclose (fp);
}

AcDecoder::AcDecoder(FILE *fd)
{
    fp = fd;
    bits_to_go = 0;
    garbage_bits = 0;
    
    value = 0;
    for (int i = 1; i <= Code_value_bits; i++)  {
        value = 2 * value + input_bit();
    }
    low = 0;
    high = Top_value;
}

AcDecoder::~AcDecoder()
{
    fclose (fp);
}

AcModel::AcModel(int n, int *ifreq)
{
    nsym = n;
    freq = new int[nsym];
    cfreq = new int[nsym + 1];
    
    if (ifreq)  {
        cfreq[nsym] = 0;
        for (int i = nsym - 1; i >= 0; i--)  {
            freq[i] = ifreq[i];
            cfreq[i] = cfreq[i + 1] + freq[i];
        }
        if (cfreq[0] > MaxFreq)
            cerr << "AC: model max frequency exceeded" << endl;
    }  else  {
        for (int i = 0; i < nsym; i++) {
            freq[i] = 1;
            cfreq[i] = nsym - i;
        }
        cfreq[nsym] = 0;
    }
}

AcModel::~AcModel()
{
    nsym = 0;
    delete freq;
    freq = NULL;
    delete cfreq;
    cfreq = NULL;
}

long AcEncoder::ac_encoder_bits()
{
    return total_bits;
}

void AcEncoder::ac_encode_symbol(AcModel &acm, int sym)
{
    long range = static_cast<long>(high - low) + 1;
    high = low + (range * acm.cfreq[sym]) / acm.cfreq[0] - 1;
    low = low + (range * acm.cfreq[sym + 1]) / acm.cfreq[0];
    
    for (;;)  {
        if (high < Half)  {
            bit_plus_follow (0);
        }  else if (low >= Half) {
            bit_plus_follow (1);
            low -= Half;
            high -= Half;
        }  else if (low >= First_qtr && high < Third_qtr) {
            fbits += 1;
            low -= First_qtr;
            high -= First_qtr;
        }  else
            break;
        low = 2 * low;
        high = 2 * high + 1;
    }
    
    acm.update_model(sym);
}

int AcDecoder::ac_decode_symbol (AcModel &acm)
{
    int sym;
    long range = static_cast<long>(high - low) + 1;
    int cum = ((static_cast<long>(value - low) + 1) * acm.cfreq[0] - 1) / range;
    
    for (sym = 0; acm.cfreq[sym + 1] > cum; sym++);
    
    high = low + (range * acm.cfreq[sym]) / acm.cfreq[0] - 1;
    low = low +  (range * acm.cfreq[sym + 1]) / acm.cfreq[0];
    
    for (;;)  {
        if (high < Half)  {
            
        }  else if (low >= Half)  {
            value -= Half;
            low -= Half;
            high -= Half;
        }  else if (low >= First_qtr && high < Third_qtr)  {
            value -= First_qtr;
            low -= First_qtr;
            high -= First_qtr;
        }  else
            break;
        low = 2 * low;
        high = 2 * high + 1;
        value = 2 * value + input_bit();
    }
    
    acm.update_model(sym);
    
    return sym;
}

/* DataBase Methods */

void DataBase::put_data(ulong data, Color c){
    switch (c){
        case Color::Y:
            y.push_back(data);
            return;
        case Color::U:
            u.push_back(data);
            return;
        case Color::V:
            v.push_back(data);
            return;
    }
}

void DataBase::next(Color c)
{
    switch(c){
        case Color::Y:
            y_i++;
            y_i %= y.size();
            return;
        case Color::U:
            u_i++;
            u_i %= u.size();
            return;
        case Color::V:
            v_i++;
            v_i %= v.size();
            return;
    }
}

ulong DataBase::get_data(Color c)
{
    switch(c){
        case Color::Y:
            return y[y_i];
        case Color::U:
            return u[u_i];
        case Color::V:
            return v[v_i];
    }
}

void DataBase::read_sizes(){
    fread(&im_sz, sizeof(im_sz), 1, fd);
    fread(&r_sz, sizeof(r_sz), 1, fd);
}

void DataBase::write_sizes(){
    fwrite(&im_sz, sizeof(im_sz), 1, fd);
    fwrite(&r_sz, sizeof(r_sz), 1, fd);
}

/* _to_stream */

void DataBase::y_to_stream(){
    for (ulong data : y){
        if (data & 1 << 27){
            stream.push(data >> 24);
            stream.push(data << 8 >> 24);
            stream.push(data << 16 >> 24);
        }
        else stream.push(0);
        stream.push(data);
    }
}
void DataBase::u_to_stream(){
    for (ulong data : u){
        stream.push(data);
    }
}
void DataBase::v_to_stream(){
    for (ulong data : v){
        stream.push(data);
    }
}

/* stream_to_ */

void DataBase::stream_to_y(){
    while (!stream.empty())
    {
        ulong data = stream.front();
        stream.pop();
        
        if (data & 1 << 3){
            data <<= 24;
            data |=  stream.front() << 16;
            stream.pop();
            data |=  stream.front() << 8;
            stream.pop();
            data |=  stream.front();
            stream.pop();
        }
        else{
            data = stream.front();
            stream.pop();
        }
        y.push_back(data);
    }
}

void DataBase::stream_to_u(){
    while (!stream.empty()){
        ulong data = 0;

        data |= stream.front();
        stream.pop();
        u.push_back(data);
    }
}

void DataBase::stream_to_v(){
    while (!stream.empty()){
        ulong data = 0;
        
        data |= stream.front();
        stream.pop();
        
        v.push_back(data);
    }
}

/* ac_encode */

void DataBase::encode_stream(AcEncoder &ace, AcModel &acm)
{
    while (!stream.empty()){
        int sym = stream.front();
        ace.ac_encode_symbol(acm, sym);
        stream.pop();
    }
    ace.ac_encode_symbol(acm, EOF_symbol);
}

void DataBase::decode_stream(AcDecoder &acd, AcModel &acm)
{
    int sym = 0;
    int counter=0;
    while ((sym = acd.ac_decode_symbol(acm)) != EOF_symbol){
        stream.push(sym);
        counter++;
    }
}

void DataBase::ac_encode()
{
    write_sizes();
    
    AcEncoder ace = AcEncoder(fd);
    AcModel acm = AcModel(NSYM, NULL);
    
    y_to_stream();
    encode_stream(ace, acm);
    
    u_to_stream();
    encode_stream(ace, acm);
    
    v_to_stream();
    encode_stream(ace, acm);
}

void DataBase::ac_decode()
{
    read_sizes();
    
    AcDecoder acd = AcDecoder(fd);
    AcModel acm = AcModel(NSYM, NULL);
    
    decode_stream(acd, acm);
    stream_to_y();
    
    decode_stream(acd, acm);
    stream_to_u();
    
    decode_stream(acd, acm);
    stream_to_v();
}