#include <chrono>
#include <iostream>
#include <thread>

#include "infra/stop_token.hpp"
#include "infra/thread_runner.hpp"

int main() {
  using namespace std::chrono_literals;

  dcp::StopSource global_stop; // Requirement for pipeline

  dcp::ThreadRunner runner("test");

  // Start the runner, simulating a stage
  runner.start(global_stop.token(),
               [](const dcp::StopToken& global, const std::atomic_bool& local) {
                 using namespace std::chrono_literals;

                 int i = 0;
                 while (!global.stop_requested() &&
                        !local.load(std::memory_order_relaxed)) {
                   std::cout << "[worker] tick " << i++ << "\n";
                   std::this_thread::sleep_for(200ms);
                 }

                 std::cout << "[worker] exiting\n";
               });

  std::cout << "[main] worker started\n";
  std::this_thread::sleep_for(4000ms);

  std::cout << "[main] requesting global stop\n";
  global_stop.request_stop();

  runner.join();
  std::cout << "[main] joined, exiting\n";
  return 0;
}