#include "MqttClientImpl.hpp"
#include "Config.hpp"
#include "Evts.hpp"
#include "qp.hpp"
namespace GofkuCam
{
MqttClientImpl::MqttClientImpl(QP::QActive * owner, std::string broker_url, std::uint32_t broker_port, LoggerInterfacePtr logger)
   : m_owner(owner)
   , m_broker_url(broker_url)
   , m_broker_port(broker_port)
   , m_logger(logger)
   , m_is_connected(false)
{
}

void MqttClientImpl::start_req(QP::QEvt const * const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Starting MQTT client, connecting to broker at " +  m_broker_url + ":" +std::to_string(m_broker_port));
}

void MqttClientImpl::not_started_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("NOT_STARTED state entry");
}

void MqttClientImpl::not_started_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("NOT_STARTED state exit");
}

void MqttClientImpl::connected_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("CONNECTED state entry");
}

void MqttClientImpl::connected_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("CONNECTED state exit");
}

void MqttClientImpl::disconnected_entry(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("DISCONNECTED state entry");
}

void MqttClientImpl::disconnected_exit(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->trace("DISCONNECTED state exit");
}

void MqttClientImpl::reconnection_timer_timed_out(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Reconnection timer timed out, attempting to reconnect to MQTT broker");
}

void MqttClientImpl::cat_feeding_result(QP::QEvt const* const e)
{
   (void)e; // Suppress unused parameter warning
   m_logger->info("Cat feeding result received, publishing to MQTT broker");
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

} // namespace GofkuCam
