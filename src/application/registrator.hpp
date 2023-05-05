#pragma once

namespace step::app {

class Registrator
{
public:
    static Registrator& instance();

private:
    Registrator();
    ~Registrator() = default;
    Registrator(const Registrator&) = delete;
    Registrator(Registrator&&) = delete;
    Registrator& operator=(const Registrator&) = delete;
    Registrator& operator=(Registrator&&) = delete;
};

}  // namespace step::app