#ifndef NV_JOBDISPATCHER_
#define NV_JOBDISPATCHER_

namespace nv {

class JobDispatcher {
    NV_DECLARE_CUSTOM_INSTANCE(JobDispatcher)

  public:
    virtual ~JobDispatcher() = default;

    using Job = std::function<void()>;

    // Default: execute synchronously — correct for tests and NervSDK standalone
    virtual void post(Job job, bool onMain = false) { job(); }
};

} // namespace nv

#endif
