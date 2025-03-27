#pragma once

#include <common/intrusive_ptr.h>

#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

namespace NIpc {

////////////////////////////////////////////////////////////////////////////////

class TComPort
    : public NRefCounted::TRefCountedBase
{
public:
    TComPort(const std::string &portName, uint32_t boudRate);
    ~TComPort();

    void Open();
    void Close();
    std::string ReadLine();
    size_t Read(void* buffer, size_t size);
    void Write(const std::string& data);

    bool IsOpen() const;

private:
#if defined(_WIN32) || defined(_WIN64)
    bool setupPort();
#endif

    std::string PortName_;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE Desc_ = NULL;
#else
    int Desc_ = -1;
#endif

    char buffer[256] = "";

    bool Connected_ = false;
    uint32_t BoudRate_;
};

DECLARE_REFCOUNTED(TComPort);

////////////////////////////////////////////////////////////////////////////////

} // namespace NIpc
