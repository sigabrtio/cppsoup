#pragma once

#include <coroutine>
#include <iterator>
#include <iostream>
#include <mutex>
#include <list>

#include <thesoup/async/types.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::async
     *
     * \brief Sub namespace with some async utilities.
     * */
    namespace async {

        /**
         * \brief A simple round robin implementation of the RoundRobinCoroExecutor interface
         *
         * This class is a simple round robin implementation of the RoundRobinCoroExecutor interface. In each step, the
         * executor cycles through the entire queue (list) of tasks and calls resume on them. IF a task is complete, it
         * erase it from the queue.
         *
         * Please note that the queue is locked during the entire single iteration. So the schedule function has to wait
         * if an iteration is in progress. This IS thread safe. NO issues there.
         *
         */
        class RoundRobinCoroExecutor: public CoroExecutorInterface<RoundRobinCoroExecutor> {
        private:
            std::list<std::coroutine_handle<>> handles {};
            std::atomic<std::size_t> num_open_tasks;
            std::atomic<bool> run {true};
            std::mutex lock;

        public:
            /**
             * \brief Schedule a task.
             *
             * This method takes a coroutine handle abd puts it in the task queue. As mentioned before, if a `step` is in
             * progress, the schedule function will wait.
             *
             * @param handle The coroutine handle to schedule.
             */
            void schedule(std::coroutine_handle<>&& handle) noexcept override {
                std::lock_guard<std::mutex> guard {lock};
                handles.push_back(handle);
                num_open_tasks++;
            }

            /**
             * \brief Start the main loop.
             *
             * This method starts the main loo[ in a background thread. The `step` function is called in a while loop,
             * conditioned to run until the `stop` method is called. The method returns a future, so that the caller can
             * continue on their merry ways after starting the scheduler.
             *
             * @return
             */
            std::future<void> start() noexcept {
                return std::async(
                        [&]() {
                            while (run) {
                                step();
                            }
                        });
            }

            /**
             * \brief Send a signal to the executor.
             *
             * This method sends a "stop signal" to the executor. If a `step` happens to be in progress inside the main
             * loop (created by the `start` method), it will finish that stop, and then stop.
             *
             * Please note that this method DOES NOT destroy any resources. The executor object is still valid. One is
             * welcome to restart execution by calling the `start` method again.
             *
             */
            void stop() noexcept {
                run = false;
            }

            /**
             * \brief Step through the task queue once.
             *
             * This method steps through the task queue once. During an iteration, this method locks the queue so that
             * new tasks cannot come in. During the iteration, whatever tasks have finished are destroyed and deleted
             * from the list. The rest have the resume method called on them.
             *
             * This function is what is called inside the thread created by `start` in a loop.
             *
             */
            void step() noexcept {
                std::lock_guard<std::mutex> guard {lock};
                auto it {handles.begin()};
                while(it != handles.end()) {
                    if (it->done()) {
                        it->destroy();
                        it = handles.erase(it);
                        num_open_tasks--;
                    } else {
                        it->resume();
                        it++;
                    }
                }
            }

            /**
             * \brief Returns the size of the task queue.
             *
             * This method returns the number of tasks in the queue. Note that a task may have returned and waiting to be
             * cleaned up on the final suspend. That will still count towards the size.
             *
             * @return
             */
            [[nodiscard]] std::size_t size() const noexcept {
                return num_open_tasks;
            }
        };
    }
}
