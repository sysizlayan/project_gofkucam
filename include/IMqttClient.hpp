#ifndef IMQTT_CLIENT_HPP_
#define IMQTT_CLIENT_HPP_

#include <memory>
#include "qpcpp.hpp"

namespace GofkuCam
{

class IMqttClient {
public:
    virtual void not_started_entry(QP::QEvt const * const e) = 0;
    virtual void not_started_exit(QP::QEvt const * const e) = 0;

    virtual void start_req(QP::QEvt const * const e) = 0;

    virtual void connected_entry(QP::QEvt const * const e) = 0;
    virtual void connected_exit(QP::QEvt const * const e) = 0;

    virtual void disconnected_entry(QP::QEvt const * const e) = 0;
    virtual void disconnected_exit(QP::QEvt const * const e) = 0;

    virtual void reconnection_timer_timed_out(QP::QEvt const * const e) = 0;

    virtual void cat_feeding_result(QP::QEvt const * const e) = 0;

    virtual void stop_req(QP::QEvt const * const e) = 0;
    virtual bool is_connected() = 0;

    virtual ~IMqttClient() = default;
}; // class IMqttClient

using IMqttClientPtr = std::shared_ptr<IMqttClient>;

} // namespace GofkuCam

#endif
