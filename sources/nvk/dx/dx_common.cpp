#ifdef _WIN32

#include <comdef.h>
#include <nvk/dx/dx_common.h>

namespace nv {

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                        size_needed, NULL, NULL);
    return strTo;
}

auto get_hresult_error_message(HRESULT hr) -> std::string {
    _com_error err(hr);
    return err.ErrorMessage();
}

} // namespace nv

#endif
