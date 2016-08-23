#include "Algorithm.h"
#include "Conversion.h"

int main(int argc, char* argv[])
{
    try{
        int mode = 0;
        int block_size = 16;
        int quality = 0;
        char *image_fn = 0;
        char *archive = 0;
        
        for(int i = 1; i < argc; i++){
            if(strcmp(argv[i], "-c") == 0){
                i++;
                if(argv[i]){
                    image_fn = argv[i];
                }
                else{
                    throw "Not enough input arguments";
                }
                i++;
                if(argv[i]){
                    archive = argv[i];
                }
                else{
                    throw "Not enough input arguments";
                }
                mode = 1;
                
            }
            else if(strcmp(argv[i], "-d") == 0){
                i++;
                if(argv[i]){
                    archive = argv[i];
                }
                else{
                    throw "Not enough input arguments";
                }
                i++;
                if(argv[i]){
                    image_fn = argv[i];
                }
                else{
                    throw "Not enough input arguments";
                }
                mode = 2;
            }
            else if(strcmp(argv[i], "-s") == 0){
                i++;
                if(argv[i]){
                    block_size = atoi(argv[i]);
                    if(block_size < 1){
                        throw "Block size should be positive number";
                    }
                }
                else{
                    throw "Not enough input arguments";
                }
            }
            else if(strcmp(argv[i],"-q") == 0){
                i++;
                if(argv[i]){
                    quality = atoi(argv[i]);
                    if((quality < 1)||(quality > 100)){
                        throw "Quality should be in range [1,100]";
                    }
                }
                else{
                    throw "Not enough input arguments";
                }
            }
            else{
                throw "Unknown command line option";
            }
        }
        
        if (mode == 0){
            printf("Work mode wasn't speciefied.\nDoing nothing...\nUsage:\n -c input.bmp output.fr [-s block_size] [-q qualitty]\n -d input.fr output.bmp\n");
            system("pause");
        }
        else if (mode == 1){
            printf("Compression mode\nReading input image\n");
            Image imgd = load_image(image_fn);
            if( (imgd.n_rows % 256) || (imgd.n_rows != imgd.n_cols) ){
                throw "Input image should be 256x256 or 512x512 pixels";
            }
            printf("Running compression proccess\n");
            Compress(imgd, archive, block_size, quality);
            printf("Done\n");
        }
        else if(mode == 2){
            printf("Decompression mode\nRunning decompression proccess\n");
            Image imgd = Decompress(archive);
            
            printf("Writing output image\n");
            save_image(imgd, image_fn);
        }
    }
    catch(char *e){
        printf("Error:");
        printf("%s\n",e);
        system("pause");
    }
    return 0;
}
 
