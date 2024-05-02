#include <iostream>
#include <hwloc.h>

int main() {
    // Initialize hwloc topology object
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);

    // Get the current CPU set
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);

    hwloc_obj_t current_core = hwloc_get_obj_covering_cpuset(topology, cpuset);
    hwloc_obj_t nearest_cores[4]; // Adjust the array size as needed
    int num_nearest_cores = hwloc_get_closest_objs(topology, current_core, nearest_cores, 4);

    int depth = 7;
    auto prev = current_core;
    hwloc_obj_t next_core = hwloc_get_next_obj_covering_cpuset_by_depth(topology, cpuset, depth, prev);

    // Print the nearest cores
    std::cout << current_core->memory_first_child->depth << next_core << hwloc_topology_get_depth(topology) << "Nearest cores to current core:" << std::endl;
    for (int i = 0; i < num_nearest_cores; ++i) {
        std::cout << "Core " << nearest_cores[i]->os_index << std::endl;
    }

    // Cleanup
    hwloc_bitmap_free(cpuset);
    hwloc_topology_destroy(topology);

    return 0;
}
