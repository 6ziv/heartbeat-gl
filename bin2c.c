#include <stdio.h>
int main(int argc,char** argv){
	if(argc!=4)return 1;
	size_t file_size;
	FILE* fbinary = fopen(argv[1],"rb");
	
	fseek(fbinary,0,SEEK_END);
	file_size = ftell(fbinary);
	fseek(fbinary,0,SEEK_SET);
	
	FILE* fheader = fopen(argv[2],"w");
	if(fbinary==NULL || fheader==NULL)return 0;
	
	fprintf(fheader,"#ifdef __cplusplus\n");
	fprintf(fheader,"extern \"C\" {\n");
	fprintf(fheader,"#endif\n");
	
	fprintf(fheader,"static const unsigned char %s[%zd] = {\n",argv[3],file_size);
	if(file_size != 0){
		for(size_t i = 0; i < (file_size - 1) / 8 + 1; i++){
			unsigned char tmp[8];
			size_t rsize = fread(tmp,1,8,fbinary);
			fprintf(fheader,"    ");
			for(size_t j = 0; j < rsize; j++){
				fprintf(fheader,"0x%02x",tmp[j]);
				if(i * 8 + j != file_size - 1){
					fprintf(fheader,", ");
				}
			}
			fprintf(fheader,"\n");
		}
	}
	
	fprintf(fheader,"};\n");
	fprintf(fheader,"#ifdef __cplusplus\n");
	fprintf(fheader,"}\n");
	fprintf(fheader,"#endif\n");
	
}