#ifndef MQTT_CLIENT_IMPL_HPP_
#define MQTT_CLIENT_IMPL_HPP_

#include "GofkuCamCommon.hpp"
#include "IMqttClient.hpp"
#include "LoggerInterface.hpp"
#include "mqtt/async_client.h"
#include "qp.hpp"
#include <memory>
#include <string>

namespace GofkuCam
{


class MqttClientImpl : public IMqttClient
{
 private:
   QP::QActive* m_owner;
   std::string m_broker_url;
   std::uint32_t m_broker_port;
   LoggerInterfacePtr m_logger;

   bool m_is_connected;

   QP::QTimeEvt m_disconnection_timer;
   // The MQTT client
   std::unique_ptr<mqtt::async_client> m_client;

 public:
   explicit MqttClientImpl(QP::QActive* owner, std::string broker_url, std::uint32_t broker_port,
                           LoggerInterfacePtr logger);

   // IMqttClient interface implementation
   void not_started_entry(QP::QEvt const* const e) override;
   void not_started_exit(QP::QEvt const* const e) override;

   void start_req(QP::QEvt const* const e) override;

   void connected_entry(QP::QEvt const* const e) override;
   void connected_exit(QP::QEvt const* const e) override;

   void disconnected_entry(QP::QEvt const* const e) override;
   void disconnected_exit(QP::QEvt const* const e) override;

   void reconnection_timer_timed_out(QP::QEvt const* const e) override;

   void cat_feeding_result(QP::QEvt const* const e) override;

   void stop_req(QP::QEvt const* const e) override;
   bool is_connected() override;

   void connected(const std::string& cause);

   void connection_lost(const std::string& cause);
}; // class MqttClientImpl

} // namespace GofkuCam

#endif // MQTT_CLIENT_IMPL_HPP_