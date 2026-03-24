#ifndef NV_THREADPOOLDISPATCHER_
#define NV_THREADPOOLDISPATCHER_

#include <nvk/task/JobDispatcher.h>

namespace nv {

class ThreadPoolDispatcher : public JobDispatcher {
  public:
    explicit ThreadPoolDispatcher(
        U32 threadCount = std::thread::hardware_concurrency()) {
        for (U32 i = 0; i < threadCount; ++i)
            _workers.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPoolDispatcher() override {
        {
            std::lock_guard lock(_mutex);
            _stopping = true;
        }
        _cv.notify_all();
        for (auto& t : _workers)
            t.join();
    }

    void post(Job job, bool onMain = false) override {
        // onMain has no real meaning without a main thread pump, so we
        // just run it on the pool.
        {
            std::lock_guard lock(_mutex);
            _queue.push(std::move(job));
        }
        _cv.notify_one();
    }

  private:
    void worker_loop() {
        while (true) {
            Job job;
            {
                std::unique_lock lock(_mutex);
                _cv.wait(lock, [this] { return _stopping || !_queue.empty(); });
                if (_stopping && _queue.empty())
                    return;
                job = std::move(_queue.front());
                _queue.pop();
            }
            job();
        }
    }

    std::vector<std::thread> _workers;
    std::queue<Job> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _stopping = false;
};

} // namespace nv

#endif
