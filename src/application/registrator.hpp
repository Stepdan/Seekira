#pragma once

namespace step::app {

class Registrator
{
public:
    static Registrator& instance();

    void initialize();
    bool is_initialized() const noexcept { return m_is_init; }

private:
    Registrator() = default;
    ~Registrator() = default;
    Registrator(const Registrator&) = delete;
    Registrator(Registrator&&) = delete;
    Registrator& operator=(const Registrator&) = delete;
    Registrator& operator=(Registrator&&) = delete;

private:
    bool m_is_init{false};
};

}  // namespace step::app