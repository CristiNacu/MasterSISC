#include <stdio.h>
#include <Windows.h>

#pragma pack(push, 1)

// info https://upload.wikimedia.org/wikipedia/commons/c/c4/BMPfileFormat.png

typedef struct {

    WORD Signature;
    DWORD FileSize;
    DWORD Reserved;
    DWORD FileOffsetToPixelArray;

} BMP_FILE_HEADER, *PBMP_FILE_HEADER;

typedef struct {

    DWORD DibHeaderSize;
    DWORD ImageWidth;
    DWORD ImageHeight;
    WORD Planes;
    WORD BitsPerPixel;

} DIB_HEADER, * PDIB_HEADER;

#pragma pack(pop)

void main() {
    
    LPCSTR fileName = "*.bmp";
    WIN32_FIND_DATAA lpFindFileData;

    HANDLE hFileIterator = FindFirstFileA(fileName, &lpFindFileData);
    if (hFileIterator != NULL) {
        do
        {
            printf("Analyzing file %s:\n", lpFindFileData.cFileName);

            HANDLE openedFile = CreateFileA(
                lpFindFileData.cFileName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                lpFindFileData.dwFileAttributes,
                NULL
            );

            if (openedFile == INVALID_HANDLE_VALUE)
                continue;

            HANDLE mppedFile = CreateFileMappingA(
                openedFile,
                NULL,
                PAGE_READWRITE,
                0,
                0,
                NULL
            );

            if (mppedFile == NULL)
            {
                CloseHandle(openedFile);
                continue;
            }

            BYTE* startFile = (BYTE*)MapViewOfFile(
                mppedFile,
                FILE_MAP_ALL_ACCESS,
                0, 0, 0);

            if (startFile == NULL)
            {
                CloseHandle(mppedFile);
                CloseHandle(openedFile);
                continue;
            }

            // BMP_FILE_HEADER *bmpHeader = (BMP_FILE_HEADER *)startFile;
            PBMP_FILE_HEADER bmpHeader = (PBMP_FILE_HEADER)startFile;

            printf("signature = %x\n", bmpHeader->Signature);

            // 0x4D42 = BMP Signature
            if (bmpHeader->Signature != 0x4D42)
            {
                CloseHandle(mppedFile);
                CloseHandle(openedFile);
                continue;
            }

            printf("signature = %d\n", bmpHeader->FileSize);

            // Alte moduri:
            //                    = (PDIB_HEADER)(bmpHeader + 1)
            PDIB_HEADER dibHeader = (PDIB_HEADER)(startFile + sizeof(BMP_FILE_HEADER));

            printf("file offset to px array = %x", bmpHeader->FileOffsetToPixelArray);

            BYTE* imageDataStart = startFile + bmpHeader->FileOffsetToPixelArray;

            for (int i = 0; i < 300; i+=3)
            {
                imageDataStart[i+0] = 0xFF;
                imageDataStart[i+1] = 0;
                imageDataStart[i+2] = 0;
            }



        } while (FindNextFileA(hFileIterator, &lpFindFileData));
    }
    

}