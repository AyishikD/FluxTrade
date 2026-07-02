#include <gtest/gtest.h>
#include <kernel/kernel.hpp>
#include <memory>
#include <thread>

using namespace fluxtrade;

class TestLifecycleModule : public IModule {
private:
    ModuleId id_;
    std::vector<ModuleId> deps_;
    ModuleState state_ = ModuleState::Created;
    std::vector<std::string>& execution_log_;

public:
    TestLifecycleModule(ModuleId id, std::vector<ModuleId> deps, std::vector<std::string>& log) 
        : id_(id), deps_(std::move(deps)), execution_log_(log) {}

    [[nodiscard]] ModuleId id() const noexcept override { return id_; }
    [[nodiscard]] std::string_view name() const noexcept override {
        switch (id_) {
            case ModuleId::Config: return "Config";
            case ModuleId::EventBus: return "EventBus";
            case ModuleId::Matching: return "Matching";
            default: return "Mock";
        }
    }
    [[nodiscard]] std::vector<ModuleId> dependencies() const override { return deps_; }
    [[nodiscard]] ModuleState state() const noexcept override { return state_; }

    expected<void, std::string> initialize() override {
        state_ = ModuleState::Initializing;
        execution_log_.push_back(std::string(name()) + "_init");
        return {};
    }
    expected<void, std::string> configure() override {
        state_ = ModuleState::Configured;
        execution_log_.push_back(std::string(name()) + "_config");
        return {};
    }
    expected<void, std::string> start() override {
        state_ = ModuleState::Running;
        execution_log_.push_back(std::string(name()) + "_start");
        return {};
    }
    expected<void, std::string> pause() override { return {}; }
    expected<void, std::string> resume() override { return {}; }
    expected<void, std::string> stop() override {
        state_ = ModuleState::Stopped;
        execution_log_.push_back(std::string(name()) + "_stop");
        return {};
    }
    expected<void, std::string> shutdown() override {
        state_ = ModuleState::Shutdown;
        execution_log_.push_back(std::string(name()) + "_shutdown");
        return {};
    }
    HealthStatus check_health() override { return HealthStatus::Healthy; }
};

TEST(KernelTest, FullLifecycleOrchestration) {
    Kernel kernel;
    std::vector<std::string> log;

    // Config -> Matching (depends on Config)
    auto res1 = kernel.register_module<TestLifecycleModule>(ModuleId::Config, std::vector<ModuleId>{}, log);
    auto res2 = kernel.register_module<TestLifecycleModule>(ModuleId::Matching, std::vector<ModuleId>{ModuleId::Config}, log);

    ASSERT_TRUE(res1.has_value());
    ASSERT_TRUE(res2.has_value());

    // 1. Initialize
    auto init_res = kernel.initialize();
    ASSERT_TRUE(init_res.has_value());
    ASSERT_EQ(log.size(), 2);
    EXPECT_EQ(log[0], "Config_init");
    EXPECT_EQ(log[1], "Matching_init");

    // 2. Configure
    auto config_res = kernel.configure();
    ASSERT_TRUE(config_res.has_value());
    ASSERT_EQ(log.size(), 4);
    EXPECT_EQ(log[2], "Config_config");
    EXPECT_EQ(log[3], "Matching_config");

    // 3. Start
    auto start_res = kernel.start();
    ASSERT_TRUE(start_res.has_value());
    ASSERT_EQ(log.size(), 6);
    EXPECT_EQ(log[4], "Config_start");
    EXPECT_EQ(log[5], "Matching_start");

    // 4. Stop (Reverse order)
    auto stop_res = kernel.stop();
    ASSERT_TRUE(stop_res.has_value());
    ASSERT_EQ(log.size(), 8);
    EXPECT_EQ(log[6], "Matching_stop");
    EXPECT_EQ(log[7], "Config_stop");

    // 5. Shutdown (Reverse order)
    auto shut_res = kernel.shutdown();
    ASSERT_TRUE(shut_res.has_value());
    ASSERT_EQ(log.size(), 10);
    EXPECT_EQ(log[8], "Matching_shutdown");
    EXPECT_EQ(log[9], "Config_shutdown");
}

TEST(KernelTest, SignalHandlingWait) {
    Kernel kernel;
    std::vector<std::string> log;

    kernel.register_module<TestLifecycleModule>(ModuleId::Config, std::vector<ModuleId>{}, log);
    kernel.initialize();
    kernel.configure();
    kernel.start();

    // Trigger signal in background thread to exit wait loop
    std::thread sig_thread([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Kernel::handle_signal(SIGINT);
    });

    kernel.wait();
    sig_thread.join();

    EXPECT_TRUE(true); // Exited successfully
}
