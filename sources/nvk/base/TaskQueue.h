#ifndef _NV_TASKQUEUE_H_
#define _NV_TASKQUEUE_H_

#include <nvk_common.h>

namespace nv {

class TaskQueue {
  public:
    using Task = std::function<void()>;

    explicit TaskQueue(U64 maxSize = 1000);

    auto try_post(Task task) -> bool;

    void post(Task task);

    void execute_pending(U64 maxTasksPerCall = SIZE_MAX);

  private:
    Queue<Task> _tasks;
    mutable std::mutex _mutex;
    U64 _maxSize;
};

} // namespace nv

#endif
