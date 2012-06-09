#include <iostream>
#include <windows.h>


void errhandler(char* msg)
{
    printf("%s\n",msg);
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD    cClrBits;

    // Retrieve the bitmap color format, width, and height.
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp))
        printf("GetObject\n");

    // Convert the color format to a count of bits.
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else cClrBits = 32;

    // Allocate memory for the BITMAPINFO structure. (This structure
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD
    // data structures.)

     if (cClrBits != 24)
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR,
                    sizeof(BITMAPINFOHEADER) +
                    sizeof(RGBQUAD) * (1<< cClrBits));

     // There is no RGBQUAD array for the 24-bit-per-pixel format.

     else
         pbmi = (PBITMAPINFO) LocalAlloc(LPTR,
                    sizeof(BITMAPINFOHEADER));

    // Initialize the fields in the BITMAPINFO structure.

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24)
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits);

    // If the bitmap is not compressed, set the BI_RGB flag.
    pbmi->bmiHeader.biCompression = BI_RGB;

    // Compute the number of bytes in the array of color
    // indices and store the result in biSizeImage.
    // For Windows NT, the width must be DWORD aligned unless
    // the bitmap is RLE compressed. This example shows this.
    // For Windows 95/98/Me, the width must be WORD aligned unless the
    // bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                  * pbmi->bmiHeader.biHeight;
    // Set biClrImportant to 0, indicating that all of the
    // device colors are important.
     pbmi->bmiHeader.biClrImportant = 0;
     return pbmi;
 }
//The following example code defines a function that initializes the remaining structures, retrieves the array of palette indices, opens the file, copies the data, and closes the file.

void CreateBMPFile(LPTSTR pszFile, PBITMAPINFO pbi,
                  HBITMAP hBMP, HDC hDC)
 {
     HANDLE hf;                 // file handle
    BITMAPFILEHEADER hdr;       // bitmap file-header
    PBITMAPINFOHEADER pbih;     // bitmap info-header
    LPBYTE lpBits;              // memory pointer
    DWORD dwTotal;              // total count of bytes
    DWORD cb;                   // incremental count of bytes
    BYTE *hp;                   // byte pointer
    DWORD dwTmp;

    pbih = (PBITMAPINFOHEADER) pbi;
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    if (!lpBits)
         printf("GlobalAlloc\n");

    // Retrieve the color table (RGBQUAD array) and the bits
    // (array of palette indices) from the DIB.
    if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi,
        DIB_RGB_COLORS))
    {
        printf("GetDIBits\n");
    }

    // Create the .BMP file.
    hf = CreateFile(pszFile,
                   GENERIC_READ | GENERIC_WRITE,
                   (DWORD) 0,
                    NULL,
                   CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   (HANDLE) NULL);
    if (hf == INVALID_HANDLE_VALUE)
        errhandler("CreateFile");
    hdr.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M"
    // Compute the size of the entire file.
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) +
                 pbih->biSize + pbih->biClrUsed
                 * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    // Compute the offset to the array of color indices.
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +
                    pbih->biSize + pbih->biClrUsed
                    * sizeof (RGBQUAD);

    // Copy the BITMAPFILEHEADER into the .BMP file.
    if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER),
        (LPDWORD) &dwTmp,  NULL))
    {
       errhandler("WriteFile");
    }

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
    if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER)
                  + pbih->biClrUsed * sizeof (RGBQUAD),
                  (LPDWORD) &dwTmp, ( NULL)))
        errhandler("WriteFile");

    // Copy the array of color indices into the .BMP file.
    dwTotal = cb = pbih->biSizeImage;
    hp = lpBits;
    if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL))
           errhandler("WriteFile");

    // Close the .BMP file.
     if (!CloseHandle(hf))
           errhandler("CloseHandle");

    // Free memory.
    GlobalFree((HGLOBAL)lpBits);
}

int main()
{
// Create a normal DC and a memory DC for the entire screen. The
// normal DC provides a "snapshot" of the screen contents. The
// memory DC keeps a copy of this "snapshot" in the associated
// bitmap.
HDC hdcScreen, hdcCompatible;
HBITMAP hbmScreen;
SYSTEMTIME time;
char* filename;
char* filepath;
char* regSubKeyBase = "Software\\PJBSoftware\\ScreenShooter";

bool isRegVersion = false;

filepath = (char*)malloc(100 * sizeof(char));
if (!filepath) return 1;


HKEY progKey;
//Get the screenshot location from the registry, else stuff 'em in "My Documents"
if (RegOpenKeyEx(HKEY_CURRENT_USER,regSubKeyBase,0,KEY_QUERY_VALUE,&progKey) == ERROR_SUCCESS)
{
    DWORD buffsize = 100;
    DWORD type;
    if (RegQueryValueEx(progKey,"ScreenshotPath",NULL,&type,(LPBYTE)filepath,&buffsize) != ERROR_SUCCESS) return 1;
 }
else
{
    char* username = (char*) malloc(25 * sizeof(char));
    if (! username) return 1;
    DWORD buffsize = 25;
    BOOL st = GetUserName(username,&buffsize);
    if (!st) return 1;


    sprintf(filepath,"C:\\Documents and Settings\\%s\\My Documents",username);
    free (username);
}

// Get the time
GetLocalTime((LPSYSTEMTIME)&time);

hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
hdcCompatible = CreateCompatibleDC(hdcScreen);

// Create a compatible bitmap for hdcScreen.

hbmScreen = CreateCompatibleBitmap(hdcScreen,
                     GetDeviceCaps(hdcScreen, HORZRES),
                     GetDeviceCaps(hdcScreen, VERTRES));

if (hbmScreen == 0)
    printf("Could not create bitmap\n");

// Select the bitmaps into the compatible DC.

if (!SelectObject(hdcCompatible, hbmScreen))
    printf("Could not select bitmap\n");



         //Copy color data for the entire display into a
         //bitmap that is selected into a compatible DC.

        if (!BitBlt(hdcCompatible,
               0,0,
               GetDeviceCaps(hdcScreen, HORZRES), GetDeviceCaps(hdcScreen, VERTRES),
               hdcScreen,
               0,0,
               SRCCOPY))

         printf("Screen to Compat Blt Failed\n");

    filename = (char*)malloc(150 * sizeof(char));
    if (!filename) return 1;

    sprintf(filename,"%s\\screenshot-%04u-%02u-%02u-%02u%02u%02u.bmp",filepath,time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
    free(filepath);


    CreateBMPFile(filename,CreateBitmapInfoStruct(hbmScreen),hbmScreen,hdcCompatible);

    free(filename);

 	return 0;


}
