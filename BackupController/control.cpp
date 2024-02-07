#include <main.h>



bool SendControlRequest()
{
    const WCHAR FileName[] = L"Sample.txt";
    USHORT FileNameLength = static_cast<USHORT>(wcslen(FileName)) * sizeof(WCHAR);

    HANDLE hDriverPort;
    WCHAR PortName[] = L"\\FilterBackupPort";

    FilterBackupRequest request;

    request.FileNameLength = FileNameLength;
    wcscpy_s(request.FileName, FileNameLength, FileName);


    HRESULT Result = FilterConnectCommunicationPort(PortName, 0, NULL, 0, NULL, &hDriverPort);
    if (FAILED(Result)) {
        std::cout << "[*] failed to connect to driver port" << std::endl;
        return false;
    }

    ULONG replyLength;
    Result = FilterSendMessage(hDriverPort, &request, sizeof(request), NULL, 0, &replyLength);
    if (FAILED(Result)) {
        std::cout << "[*] failed to send request to driver port" << std::endl;
        return false;
    }

    std::cout << "[*] request has been sent to driver successfully!" << std::endl;
    return true;
}