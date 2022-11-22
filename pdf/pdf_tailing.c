#include <Windows.h>
#pragma const_seg(".pdf")
__declspec(allocate(".pdf"))
#include "pdf_tailing.h"
void refer_tail_data(){
	SetLastError((int)pdf_tail_data);
#pragma comment(linker,"/include:"__FUNCDNAME__)
}
