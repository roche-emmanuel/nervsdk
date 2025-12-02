#ifndef DX_COMMON_H_
#define DX_COMMON_H_

#include <nvk_common.h>

#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <comdef.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifndef SAFERELEASE
#define SAFERELEASE(x)                                                         \
    if (x) {                                                                   \
        x->Release();                                                          \
        x = NULL;                                                              \
    }
#endif

using Microsoft::WRL::ComPtr;

namespace nv {

template <typename... Args>
void check_result(HRESULT hr, fmt::format_string<Args...> fmt_str,
                  Args&&... args) {
    if (FAILED(hr)) {
        auto msg = format_msg(fmt_str, std::forward<Args>(args)...);
        _com_error err(hr);
        throw_msg("{} (err={})", msg.c_str(), err.ErrorMessage());
    }
}

inline void check_result(HRESULT hr, const char* fmt_str) {
    if (FAILED(hr)) {
        _com_error err(hr);
        throw_msg("{} (err={})", fmt_str, err.ErrorMessage());
    }
}

auto WStringToString(const std::wstring& wstr) -> std::string;

}; // namespace nv

#define CHECK_RESULT nv::check_result

#endif
