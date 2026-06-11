#ifdef _WIN32

#include <comdef.h>
#include <nvk/dx/dx_common.h>

namespace nv {

auto get_hresult_error_message(HRESULT hr) -> std::string {
    _com_error err(hr);
    return err.ErrorMessage();
}

} // namespace nv

#endif
