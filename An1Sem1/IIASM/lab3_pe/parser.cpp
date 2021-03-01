#include <stdio.h>
#include <Windows.h>
#include <winnt.h>

#define ABORT(s) {\
		if(fileHandle) \
			CloseHandle(fileHandle);\
		if(hMapping)\
			CloseHandle(hMapping);\
		if(inceputFisier)\
			UnmapViewOfFile(inceputFisier);\
		printf("%s\n",s);\
		return 1;\
	}

int main() {
	const char fileName[] = "wmpnssci.dll";
	HANDLE fileHandle = NULL, hMapping = NULL;
	BYTE * inceputFisier = 0;
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeaders = NULL;
	PIMAGE_SECTION_HEADER pSection = NULL;

	fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == fileHandle) {
		ABORT("Fisier inexistent.");
	}
	hMapping = CreateFileMappingA(fileHandle, 0, PAGE_READONLY, 0, 0, 0);
	if (NULL == hMapping) {
		ABORT("Mapping error");
	}
	inceputFisier = (BYTE*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (0 == inceputFisier)
	{
		ABORT("View error");
	}
	pDosHeader = (PIMAGE_DOS_HEADER)inceputFisier;
	//verificare marime fisier
	if (IMAGE_DOS_SIGNATURE != pDosHeader->e_magic)
	{
		ABORT("Nu e MZ.");
	}
	pNtHeaders = (PIMAGE_NT_HEADERS)(inceputFisier + pDosHeader->e_lfanew);
	if (IMAGE_NT_SIGNATURE != pNtHeaders->Signature)
	{
		ABORT("Missing PE.");
	}
	printf_s("%x\n", pNtHeaders->FileHeader.Machine);
	pSection = (PIMAGE_SECTION_HEADER)(pNtHeaders->FileHeader.SizeOfOptionalHeader + (BYTE*)&(pNtHeaders->OptionalHeader));
	ABORT("All ok.");
	return 0;
}