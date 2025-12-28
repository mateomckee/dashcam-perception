#include <chrono>
#include <iostream>
#include <thread>

#include "core/config_loader.hpp"
#include "core/frame.hpp"

#include "infra/bounded_queue.hpp"


int main(int argc, char** argv) {
  const std::string cfg_path = (argc > 1) ? argv[1] : "configs/dev.yaml";

  try {
    dcp::AppConfig cfg = dcp::LoadConfigFromYamlFile(cfg_path);

    dcp::BoundedQueue<dcp::Frame> bq(3, dcp::DropPolicy::DropOldest);

    //

    dcp::Frame f1, f2, f3, f4;
    f1.sequence_id = 1;
    f2.sequence_id = 2;
    f3.sequence_id = 3;
    f4.sequence_id = 4;


    bq.try_push(f1);
    bq.try_push(f2);
    bq.try_push(f3);
    bq.try_push(f4);

    dcp::Frame out;

    // Pop and print

    bq.try_pop(out);
    std::cout << out.sequence_id << std::endl;

    bq.try_pop(out);
    std::cout << out.sequence_id << std::endl;
    
    bq.try_pop(out);
    std::cout << out.sequence_id << std::endl;

    std::cout << bq.drops_total() << std::endl;


  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
