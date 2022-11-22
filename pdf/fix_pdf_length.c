#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include <winnt.h>
int main(int argc, char** argv) {
	if (argc != 2)return 0;
	FILE* exe = fopen(argv[1], "rb+");

	IMAGE_DOS_HEADER dos_header;
	if (1 != fread(&dos_header, sizeof(IMAGE_DOS_HEADER), 1, exe) || dos_header.e_magic != IMAGE_DOS_SIGNATURE)return 1;
	
	fseek(exe, dos_header.e_lfanew, SEEK_SET);
	DWORD pe_magic;
	if (1 != fread(&pe_magic, sizeof(DWORD), 1, exe) || pe_magic != IMAGE_NT_SIGNATURE)return 1;
	
	IMAGE_FILE_HEADER coff_file_hdr;
	if (1 != fread(&coff_file_hdr, sizeof(IMAGE_FILE_HEADER), 1, exe))return 1;
	
	//Find ".pdf" section
	fseek(exe, coff_file_hdr.SizeOfOptionalHeader, SEEK_CUR);
	const char* target_section_name = ".pdf";
	INT64 ptr = -1;
	for (WORD i = 0; i < coff_file_hdr.NumberOfSections; i++) {
		IMAGE_SECTION_HEADER section_hdr;
		fread(&section_hdr, sizeof(IMAGE_SECTION_HEADER), 1, exe);
		if (strncmp((PCCH)section_hdr.Name, target_section_name, strlen(target_section_name) + 1)) {
			continue;
		}
		ptr = section_hdr.PointerToRawData;
		printf("Found .pdf section at %I64d\n", ptr);
		break;
	}
	if (ptr < 0)return 1;

	INT64 pattern_pos = -1;
	const char* search_pattern = "/Length ExeLenByte";
	fseek(exe, sizeof(IMAGE_DOS_HEADER), SEEK_SET);
	int matched = 0;
	size_t offset1 = 8;
	size_t offset2 = 29;
	for (size_t i = sizeof(IMAGE_DOS_HEADER); i + strlen(search_pattern) <= dos_header.e_lfanew; i++) {
		int t = fgetc(exe);
		if (t != search_pattern[matched])matched = 0;
		if (t == search_pattern[matched])matched++;
		if (matched == strlen(search_pattern)) {
			pattern_pos = i + 1 - strlen(search_pattern);
			printf("Found pattern at %I64d\n", pattern_pos);
			break;
		}
	}
	if (pattern_pos < 0)return 1;

	INT64 payload_begin = pattern_pos + offset2;
	if (payload_begin > ptr)return 1;
	INT64 payload_len = ptr - payload_begin;
	char tmp[11];
	printf("Change obj length to %I64d\n", payload_len);
	sprintf(tmp, "%I64d", payload_len);
	for (int i = 0; i < 10; i++) {
		if (tmp[i] == 0) { tmp[i] = ' '; tmp[i + 1] = '\0'; }
	}

	fseek(exe, pattern_pos + offset1, SEEK_SET);
	fwrite(tmp, 1, 10, exe);
	
	fflush(exe);
	fclose(exe);
	return 0;
}