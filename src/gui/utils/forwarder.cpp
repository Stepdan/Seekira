#include <cassert>

#include <QtCore>

#include "forwarder.hpp"

namespace step::gui::utils {

Forwarder::Forwarder()
{
    qRegisterMetaType<FunctorType>("FunctorType");

    bool result = connect(this, SIGNAL(execute_functor_signal(FunctorType)), this,
                          SLOT(on_execute_functor_slot(FunctorType)), Qt::QueuedConnection);
    if (!result)
        assert(!"Failed to connect signal and slot");
}

void Forwarder::forward(Forwarder::FunctorType f) const { emit execute_functor_signal(f); }

void Forwarder::on_execute_functor_slot(Forwarder::FunctorType f) { f(); }

}  // namespace step::gui::utils