#include "MqttClient.h"

#include <mosquitto.h>

#include <QDebug>
#include <QRandomGenerator>
#include <QThread>

static struct MosquittoLibInitializer {
  MosquittoLibInitializer() { mosquitto_lib_init(); }
  ~MosquittoLibInitializer() { mosquitto_lib_cleanup(); }
} mosquittoLibInitializer;

MqttClient::MqttClient(QObject *parent) : QObject(parent) {
  // 创建Mosquitto实例（设置clean session为true）
  // 生成唯一ID (示例：使用Qt的随机数)
  client_id_ = QString("CLIENTID_%1_END").arg(QRandomGenerator::global()->generate());
  mosq_ = mosquitto_new(client_id_.toUtf8().constData(), true, this);
  if (!mosq_) {
    qFatal("Failed to create Mosquitto instance: insufficient memory");
  }

  // 设置回调函数
  setupCallbacks();

  // 配置自动重连定时器
  reconnect_timer_ = new QTimer(this);
  reconnect_timer_->setInterval(5000);  // 5秒重连间隔
  reconnect_timer_->setSingleShot(true);
  connect(reconnect_timer_, &QTimer::timeout, this, &MqttClient::handleReconnect);
}

MqttClient::~MqttClient() {
  disconnect();
  cleanup();
}

bool MqttClient::connectToBroker(const QString &host, int port, int keepalive, int max_retry) {
  // 参数有效性检查
  if (host.isEmpty() || port <= 0 || keepalive <= 0) {
    qWarning() << "Invalid connection parameters";
    return false;
  }

  // 保存连接参数用于重连
  host_ = host;
  port_ = port;
  keepalive_ = keepalive;
  max_retry_ = qMax(1, max_retry);
  retry_count_ = 0;

  // 发起连接
  int rc = mosquitto_connect(mosq_, host.toUtf8().constData(), port, keepalive);
  if (rc != MOSQ_ERR_SUCCESS) {
    qCritical() << "Initial connection failed:" << mosquitto_strerror(rc);
    return false;
  }

  // 启动网络循环线程（非阻塞）
  rc = mosquitto_loop_start(mosq_);
  if (rc != MOSQ_ERR_SUCCESS) {
    qCritical() << "Failed to start network loop:" << mosquitto_strerror(rc);
    return false;
  }

  return true;
}

void MqttClient::disconnectFromBroker() {
  if (connected_) {
    mosquitto_disconnect(mosq_);
    reconnect_timer_->stop();
    connected_ = false;
  }
}

void MqttClient::subscribe(const QString &topic, int qos) {
  if (!connected_) {
    qWarning() << "Cannot subscribe when disconnected";
    return;
  }

  // 使用UTF8编码处理中文主题
  int rc = mosquitto_subscribe(mosq_, nullptr, topic.toUtf8().constData(), qos);
  if (rc != MOSQ_ERR_SUCCESS) {
    qWarning() << "Subscribe failed:" << mosquitto_strerror(rc);
    emit connectionFailed(mosquitto_strerror(rc));
  }
}

void MqttClient::publish(const QString &topic, const QByteArray &payload, int qos, bool retain) {
  if (!connected_) {
    qWarning() << "Cannot publish when disconnected";
    return;
  }

  /*
  // 创建MQTTv5属性（需要Mosquitto 2.0+版本）
  mosquitto_property *props = nullptr;
  // 设置NO_LOCAL属性（需代理支持MQTTv5）让代理不要将消息回传给发布者自己
  mosquitto_property_add_boolean(&props, MQTT_PROP_NO_LOCAL, 1);
  int rc = mosquitto_publish_v5(mosq_,
                                nullptr,  // 自动生成message id
                                topic.toUtf8().constData(), payload.size(), payload.constData(), qos, retain, props);
  // 清理属性资源
  mosquitto_property_free_all(&props);
  if (rc != MOSQ_ERR_SUCCESS) {
    qWarning() << "Publish failed:" << mosquitto_strerror(rc);
  }
*/

  QByteArray fullPayload = "[ClientID:" + client_id_.toUtf8() + "]" + payload;
  int rc = mosquitto_publish(mosq_,
                             nullptr,  // 自动生成 message id
                             topic.toUtf8().constData(), fullPayload.size(), fullPayload.constData(), qos, retain);
  if (rc != MOSQ_ERR_SUCCESS) {
    qWarning() << "Publish failed:" << mosquitto_strerror(rc);
  }
}

QString MqttClient::clientId() const { return client_id_; }

bool MqttClient::mqttIsConnected() { return connected_; }

void MqttClient::setupCallbacks() {
  // 绑定C库回调到静态成员函数
  mosquitto_connect_callback_set(mosq_, &MqttClient::onConnect);
  mosquitto_disconnect_callback_set(mosq_, &MqttClient::onDisconnect);
  mosquitto_message_callback_set(mosq_, &MqttClient::onMessage);
  mosquitto_log_callback_set(mosq_, &MqttClient::onLog);
}

void MqttClient::cleanup() {
  if (mosq_) {
    // 停止网络循环并销毁实例
    mosquitto_loop_stop(mosq_, true);  // 强制停止
    mosquitto_destroy(mosq_);
    mosq_ = nullptr;
  }
}

void MqttClient::onConnect(mosquitto *mosq, void *obj, int rc) {
  MqttClient *client = static_cast<MqttClient *>(obj);
  client->retry_count_ = 0;  // 重置重试计数器

  if (rc == MOSQ_ERR_SUCCESS) {
    client->connected_ = true;
    client->reconnect_timer_->stop();
    emit client->connected();
    qInfo() << "Connected to broker at" << client->host_ << ":" << client->port_;
  } else {
    client->connected_ = false;
    QString errMsg = mosquitto_strerror(rc);
    qCritical() << "Connection failed:" << errMsg;
    emit client->connectionFailed(errMsg);
    client->reconnect_timer_->start();
  }
}

void MqttClient::onDisconnect(mosquitto *mosq, void *obj, int rc) {
  MqttClient *client = static_cast<MqttClient *>(obj);
  client->connected_ = false;

  if (rc == MOSQ_ERR_SUCCESS) {
    qInfo() << "Gracefully disconnected";
    emit client->disconnected();
  } else {
    qWarning() << "Unexpected disconnection:" << mosquitto_strerror(rc);
    emit client->connectionFailed(mosquitto_strerror(rc));
    client->reconnect_timer_->start();
  }
}

void MqttClient::onMessage(mosquitto *mosq, void *obj, const mosquitto_message *msg) {
  MqttClient *client = static_cast<MqttClient *>(obj);

  // 构造消息参数（注意线程安全）
  QString topic = QString::fromUtf8(msg->topic);
  QByteArray payload(static_cast<char *>(msg->payload), msg->payloadlen);

  // 通过信号传递到主线程
  emit client->messageReceived(topic, payload, msg->qos, msg->retain);
}

void MqttClient::onLog(mosquitto *mosq, void *obj, int level, const char *str) {
  Q_UNUSED(obj)
  // 根据日志级别输出不同信息
  switch (level) {
    case MOSQ_LOG_DEBUG:
      qDebug() << "[MQTT Debug]" << str;
      break;
    case MOSQ_LOG_INFO:
      qInfo() << "[MQTT Info]" << str;
      break;
    case MOSQ_LOG_NOTICE:
      qInfo() << "[MQTT Notice]" << str;
      break;
    case MOSQ_LOG_WARNING:
      qWarning() << "[MQTT Warning]" << str;
      break;
    case MOSQ_LOG_ERR:
      qCritical() << "[MQTT Error]" << str;
      break;
    default:
      qDebug() << "[MQTT Unknown]" << str;
  }
}

void MqttClient::handleReconnect() {
  if (++retry_count_ > max_retry_) {
    qCritical() << "Max reconnect attempts reached";
    emit connectionFailed(tr("Max retry attempts (%1) exceeded").arg(max_retry_));
    return;
  }

  qInfo() << "Attempting reconnect (" << retry_count_ << "/" << max_retry_ << ")";
  connectToBroker(host_, port_, keepalive_, max_retry_);
}
