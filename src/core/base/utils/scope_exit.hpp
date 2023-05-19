#pragma once

namespace step::utils {

template <typename F>
struct ScopeExitFunctionWrapper
{
    ScopeExitFunctionWrapper(const F& f) : f(f) {}

    ScopeExitFunctionWrapper(const ScopeExitFunctionWrapper&) = delete;
    ScopeExitFunctionWrapper(ScopeExitFunctionWrapper&&) = default;

    ScopeExitFunctionWrapper& operator=(const ScopeExitFunctionWrapper&) = delete;
    ScopeExitFunctionWrapper& operator=(ScopeExitFunctionWrapper&&) = default;

    ~ScopeExitFunctionWrapper() { f(); }
    F f;
};

template <typename F>
static constexpr ScopeExitFunctionWrapper<F> CreateScopeExitFunctionWrapper(const F& f)
{
    return ScopeExitFunctionWrapper<F>(f);
}

}  // namespace step::utils

#define STEP_DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#define STEP_STRING_JOIN2(arg1, arg2)                                                                                  \
    STEP_DO_STRING_JOIN2(arg1, arg2)  // без этого хака конкатенация работать не будет.

// используем variadic macros, т.к. внутри лямбды могут быть запятые.
#define STEP_SCOPE_EXIT(...)                                                                                           \
    auto STEP_STRING_JOIN2(scope_exit_, __LINE__) = step::utils::CreateScopeExitFunctionWrapper(__VA_ARGS__);          \
    (void)STEP_STRING_JOIN2(scope_exit_, __LINE__)