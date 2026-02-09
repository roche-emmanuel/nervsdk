#include <nvk/base/TaskQueue.h>

namespace nv {

TaskQueue::TaskQueue(U64 maxSize) : _maxSize(maxSize) {}

auto TaskQueue::try_post(Task task) -> bool {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_tasks.size() >= _maxSize) {
        return false;
    }
    _tasks.push(std::move(task));
    return true;
}

void TaskQueue::post(Task task) {
    std::lock_guard<std::mutex> lock(_mutex);
    NVCHK(_tasks.size() < _maxSize, "Task queue of size {} is full.", _maxSize);
    _tasks.push(std::move(task));
}

void TaskQueue::execute_pending(U64 maxTasksPerCall) {
    std::queue<Task> tasksToExecute;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_tasks.size() <= maxTasksPerCall) {
            // Fast path: take all tasks with swap (no copies/moves)
            tasksToExecute.swap(_tasks);
        } else {
            // Slow path: take only maxTasksPerCall tasks
            for (U64 i = 0; i < maxTasksPerCall; ++i) {
                tasksToExecute.push(std::move(_tasks.front()));
                _tasks.pop();
            }
        }
    }

    // Execute without holding lock
    while (!tasksToExecute.empty()) {
        tasksToExecute.front()();
        tasksToExecute.pop();
    }
}

} // namespace nv