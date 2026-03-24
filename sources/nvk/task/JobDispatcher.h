#ifndef NV_JOBDISPATCHER_
#define NV_JOBDISPATCHER_

namespace nv {

class JobDispatcher {
    NV_DECLARE_CUSTOM_INSTANCE(JobDispatcher)

  public:
    virtual ~JobDispatcher() = default;

    using Job = std::function<void()>;

    // Default: execute synchronously — correct for tests and NervSDK standalone
    virtual void post(Job job) { job(); }

    // For continuations that must run on the main thread
    virtual void post_main(Job job) { job(); }
};

} // namespace nv

#endif
