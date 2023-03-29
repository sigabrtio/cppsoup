#pragma once

#include <coroutine>
#include <iterator>
#include <iostream>
#include <mutex>
#include <list>

#include <thesoup/async/types.hpp>

namespace thesoup {

    namespace async {

        class RoundRobinCoroExecutor: public CoroExecutorInterface<RoundRobinCoroExecutor> {
        private:
            std::list<std::coroutine_handle<>> handles {};
            std::atomic<std::size_t> num_open_tasks;
            std::mutex lock;

        public:
            void schedule(std::coroutine_handle<>&& handle) noexcept {
                std::lock_guard<std::mutex> guard {lock};
                handles.push_back(handle);
                num_open_tasks++;
            }

            std::future<void> start() noexcept {
                return std::async(
                        [&]() {
                            while (true) {
                                step();
                            }
                        });
            }

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

            std::size_t size() const noexcept {
                return num_open_tasks;
            }
        };
    }
}
