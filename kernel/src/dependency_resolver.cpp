#include <kernel/dependency_resolver.hpp>
#include <unordered_set>
#include <queue>

namespace fluxtrade {

expected<std::vector<ModuleId>, std::string> DependencyResolver::resolve(
    const std::unordered_map<ModuleId, IModule*>& modules
) {
    std::unordered_map<ModuleId, std::vector<ModuleId>> adj;
    std::unordered_map<ModuleId, size_t> in_degree;

    // Initialize degrees
    for (const auto& [id, module] : modules) {
        in_degree[id] = 0;
    }

    // Build adjacency list and compute in-degrees
    for (const auto& [id, module] : modules) {
        if (!module) {
            return unexpected<std::string>("Registered module pointer is null");
        }
        for (ModuleId dep : module->dependencies()) {
            if (dep == id) {
                return unexpected<std::string>("Self-dependency detected for module ID " + std::to_string(static_cast<uint16_t>(id)));
            }
            if (modules.find(dep) == modules.end()) {
                return unexpected<std::string>("Missing dependency: module ID " + std::to_string(static_cast<uint16_t>(id)) + 
                                       " depends on missing module ID " + std::to_string(static_cast<uint16_t>(dep)));
            }
            adj[dep].push_back(id);
            in_degree[id]++;
        }
    }

    // Kahn's algorithm for topological sorting
    std::queue<ModuleId> q;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) {
            q.push(id);
        }
    }

    std::vector<ModuleId> order;
    order.reserve(modules.size());

    while (!q.empty()) {
        ModuleId curr = q.front();
        q.pop();
        order.push_back(curr);

        auto it = adj.find(curr);
        if (it != adj.end()) {
            for (ModuleId neighbor : it->second) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    q.push(neighbor);
                }
            }
        }
    }

    if (order.size() != modules.size()) {
        return unexpected<std::string>("Cyclic dependency detected in module graph");
    }

    return order;
}

} // namespace fluxtrade
