#include "MqttClientImpl.hpp"
#include "Evts.hpp"
#include "qp.hpp"
#include <memory>
#include <string>
#include <unistd.h>

namespace GofkuCam
{

MqttClientImpl::MqttClientImpl(QP::QActive* owner, std::string broker_url, std::uint32_t broker_port,
                               LoggerInterfacePtr logger)
   : m_owner(owner)
   , m_broker_url(broker_url)
   , m_broker_port(broker_port)
   , m_logger(logger)
   , m_disconnection_timer{m_owner, RECONNECTION_TIMER_TIMED_OUT_SIG}
   , m_is_connected(false)
{
   std::string serverURI = m_broker_url + ":" + std::to_string(m_broker_port);
   m_logger->info("Creating MQTT client for broker at " + serverURI);
   const int MAX_BUFFERED_MESSAGES = 10;

   std::string clientId = "GofkuCam_Vision_" + std::to_string(getpid());

   // We configure to allow publishing to the client while off-line,
   // and that it's OK to do so before the 1st successful connection.
   auto createOpts = mqtt::create_options_builder()
                        .server_uri(serverURI)
                        .send_while_disconnected(false, false)
                        .max_buffered_messages(MAX_BUFFERED_MESSAGES)
                        .delete_oldest_messages()
                        .client_id(clientId)
                        .finalize();
   m_client = std::make_unique<mqtt::async_client>(createOpts);

   m_client->set_connected_handler(
      std::bind(&MqttClientImpl::connected, this, std::placeholders::_1));

   m_client->set_connection_lost_handler(
       std::bind(&MqttClientImpl::connection_lost, this, std::placeholders::_1));
}

void MqttClientImpl::start_req(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning

   m_owner->subscribe(CONNECTED_TO_BROKER_SIG);
   m_owner->subscribe(DISCONNECTED_FROM_BROKER_SIG);
   m_owner->subscribe(RECONNECTION_TIMER_TIMED_OUT_SIG);
   m_owner->subscribe(CAT_FEEDING_DETERMINED_SIG);

   m_logger->info("Starting MQTT client, connecting to broker at " + m_broker_url + ":" +
                  std::to_string(m_broker_port));

   m_client->connect();
}

void MqttClientImpl::not_started_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_NOT_STARTED state entry");
}

void MqttClientImpl::not_started_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_NOT_STARTED state exit");
}

void MqttClientImpl::connected_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_CONNECTED state entry");
   m_is_connected = true;
}

void MqttClientImpl::connected_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_CONNECTED state exit");
   m_is_connected = false;
}

void MqttClientImpl::disconnected_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_DISCONNECTED state entry");
   m_disconnection_timer.armX(1000, 0);

}

void MqttClientImpl::disconnected_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("MQTT_DISCONNECTED state exit");
}

void MqttClientImpl::reconnection_timer_timed_out(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Reconnection timer timed out, attempting to reconnect to MQTT broker");
   m_client->reconnect();
}

void MqttClientImpl::cat_feeding_result(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("Cat feeding result received, publishing to MQTT broker");
   auto cfd = Q_EVT_CAST(CatFeedingDetermined);
   if(cfd)
   {
      std::string topic = "gofkucam/cat_feeding";
      std::string payload = "";
      if(cfd->m_haku_status->m_is_haku_in_dangerous_zone)
      {
         payload = "dangerous: " + std::to_string(static_cast<int>(cfd->m_haku_status->m_hakus_distance));
         auto msg = mqtt::make_message(topic, payload);
         m_client->publish(msg);
         m_logger->info("Published cat feeding status: " + payload + " to topic: " + topic);
      }
      else
      {
         // Do nothing
      }
   }
}

void MqttClientImpl::stop_req(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->warn("Stopping MQTT client and disconnecting from broker");
}

bool MqttClientImpl::is_connected()
{
   return m_is_connected;
}

void MqttClientImpl::connected(const std::string& cause)
{
   m_logger->info("Connected to MQTT broker. Cause: " + cause);

   ConnectedToBroker* ctb = Q_NEW(ConnectedToBroker, CONNECTED_TO_BROKER_SIG);
   QP::QF::PUBLISH(ctb, m_owner);
}

void MqttClientImpl::connection_lost(const std::string& cause)
{
   m_logger->error("Connection to MQTT broker lost. Cause: " + cause);
   DisconnectedFromBroker* dfb = Q_NEW(DisconnectedFromBroker, DISCONNECTED_FROM_BROKER_SIG);
   QP::QF::PUBLISH(dfb, m_owner);
}

} // namespace GofkuCam
