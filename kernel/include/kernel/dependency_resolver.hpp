#pragma once

#include <kernel/module.hpp>
#include <vector>
#include <unordered_map>
#include <expected>
#include <string>

namespace fluxtrade {

class DependencyResolver {
public:
    // Topologically sorts modules according to their dependency graph.
    // Detects cyclic dependencies, missing dependencies, duplicate registrations, and self-dependencies.
    static expected<std::vector<ModuleId>, std::string> resolve(
        const std::unordered_map<ModuleId, IModule*>& modules
    );
};

} // namespace fluxtrade
