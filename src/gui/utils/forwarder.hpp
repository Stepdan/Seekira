#pragma once

#include <functional>

#include <QObject>

namespace step::gui::utils {

class Forwarder : public QObject
{
    Q_OBJECT

public:
    using FunctorType = std::function<void()>;

    Forwarder();

    void forward(FunctorType f) const;

signals:
    void execute_functor_signal(FunctorType f) const;

private slots:
    void on_execute_functor_slot(FunctorType f);
};

}  // namespace step::gui::utils