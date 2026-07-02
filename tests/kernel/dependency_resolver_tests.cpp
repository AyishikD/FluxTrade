#include <gtest/gtest.h>
#include <kernel/dependency_resolver.hpp>
#include <memory>

using namespace fluxtrade;

class MockModule : public IModule {
private:
    ModuleId id_;
    std::vector<ModuleId> deps_;
    ModuleState state_ = ModuleState::Created;

public:
    MockModule(ModuleId id, std::vector<ModuleId> deps) : id_(id), deps_(std::move(deps)) {}

    [[nodiscard]] ModuleId id() const noexcept override { return id_; }
    [[nodiscard]] std::string_view name() const noexcept override { return "Mock"; }
    [[nodiscard]] std::vector<ModuleId> dependencies() const override { return deps_; }
    [[nodiscard]] ModuleState state() const noexcept override { return state_; }

    expected<void, std::string> initialize() override { return {}; }
    expected<void, std::string> configure() override { return {}; }
    expected<void, std::string> start() override { return {}; }
    expected<void, std::string> pause() override { return {}; }
    expected<void, std::string> resume() override { return {}; }
    expected<void, std::string> stop() override { return {}; }
    expected<void, std::string> shutdown() override { return {}; }
    HealthStatus check_health() override { return HealthStatus::Healthy; }
};

TEST(DependencyResolverTest, SimpleOrdering) {
    // EventBus (no deps), Matching (depends on EventBus)
    auto m1 = std::make_unique<MockModule>(ModuleId::EventBus, std::vector<ModuleId>{});
    auto m2 = std::make_unique<MockModule>(ModuleId::Matching, std::vector<ModuleId>{ModuleId::EventBus});

    std::unordered_map<ModuleId, IModule*> modules;
    modules[ModuleId::EventBus] = m1.get();
    modules[ModuleId::Matching] = m2.get();

    auto result = DependencyResolver::resolve(modules);
    ASSERT_TRUE(result.has_value());
    
    const auto& order = result.value();
    ASSERT_EQ(order.size(), 2);
    EXPECT_EQ(order[0], ModuleId::EventBus);
    EXPECT_EQ(order[1], ModuleId::Matching);
}

TEST(DependencyResolverTest, MultiBranchOrdering) {
    // Config -> Risk
    // EventBus -> Matching -> Router
    auto m_config = std::make_unique<MockModule>(ModuleId::Config, std::vector<ModuleId>{});
    auto m_eb = std::make_unique<MockModule>(ModuleId::EventBus, std::vector<ModuleId>{});
    auto m_risk = std::make_unique<MockModule>(ModuleId::Risk, std::vector<ModuleId>{ModuleId::Config});
    auto m_matching = std::make_unique<MockModule>(ModuleId::Matching, std::vector<ModuleId>{ModuleId::EventBus});
    auto m_router = std::make_unique<MockModule>(ModuleId::Router, std::vector<ModuleId>{ModuleId::Matching, ModuleId::Risk});

    std::unordered_map<ModuleId, IModule*> modules;
    modules[ModuleId::Config] = m_config.get();
    modules[ModuleId::EventBus] = m_eb.get();
    modules[ModuleId::Risk] = m_risk.get();
    modules[ModuleId::Matching] = m_matching.get();
    modules[ModuleId::Router] = m_router.get();

    auto result = DependencyResolver::resolve(modules);
    ASSERT_TRUE(result.has_value());

    const auto& order = result.value();
    ASSERT_EQ(order.size(), 5);

    // Verify Router is last
    EXPECT_EQ(order[4], ModuleId::Router);
}

TEST(DependencyResolverTest, SelfDependencyDetection) {
    auto m1 = std::make_unique<MockModule>(ModuleId::Risk, std::vector<ModuleId>{ModuleId::Risk});
    std::unordered_map<ModuleId, IModule*> modules;
    modules[ModuleId::Risk] = m1.get();

    auto result = DependencyResolver::resolve(modules);
    EXPECT_FALSE(result.has_value());
}

TEST(DependencyResolverTest, CyclicDependencyDetection) {
    // Risk depends on Matching, Matching depends on Risk
    auto m1 = std::make_unique<MockModule>(ModuleId::Risk, std::vector<ModuleId>{ModuleId::Matching});
    auto m2 = std::make_unique<MockModule>(ModuleId::Matching, std::vector<ModuleId>{ModuleId::Risk});

    std::unordered_map<ModuleId, IModule*> modules;
    modules[ModuleId::Risk] = m1.get();
    modules[ModuleId::Matching] = m2.get();

    auto result = DependencyResolver::resolve(modules);
    EXPECT_FALSE(result.has_value());
}

TEST(DependencyResolverTest, MissingDependencyDetection) {
    // Risk depends on Config, but Config is not registered
    auto m1 = std::make_unique<MockModule>(ModuleId::Risk, std::vector<ModuleId>{ModuleId::Config});
    std::unordered_map<ModuleId, IModule*> modules;
    modules[ModuleId::Risk] = m1.get();

    auto result = DependencyResolver::resolve(modules);
    EXPECT_FALSE(result.has_value());
}
