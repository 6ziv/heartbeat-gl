#include<stdio.h>
char buffer[65536];
int main(int argc,char** argv){
	if(argc!=4)return 0;
	FILE* fin1 = fopen(argv[1],"rb");
	FILE* fin2 = fopen(argv[2],"rb");
	FILE* fout = fopen(argv[3],"wb+");
	if(fin1==NULL || fin2==NULL || fout==NULL)return 1;
	size_t read_size = 0;
	do{
		read_size = fread(buffer,1,65536,fin1);
		fwrite(buffer,1,read_size,fout);
	}while(read_size);
	do{
		read_size = fread(buffer,1,65536,fin2);
		fwrite(buffer,1,read_size,fout);
	}while(read_size);
	fclose(fout);
	fclose(fin2);
	fclose(fin1);
	return 0;
}