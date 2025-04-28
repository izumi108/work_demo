#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

/*!
 *  \brief MQTT 基础通信客户端,封装Mosquitto C API
 *   Mosquitto 库回调 → MosquittoClient::onConnect() → 发出 connectionFailed 信号 → Qt 信号槽系统 →
 *   MainWindow::onConnectionFailed() → 更新UI显示错误
 *   职责: 管理MQTT连接 发布订阅基础操作 事件回调绑定
 */

#include <mosquitto.h>

#include <QObject>
#include <QTimer>

class MqttClient : public QObject {
  Q_OBJECT
 public:
  explicit MqttClient(QObject *parent = nullptr);
  ~MqttClient();

  bool connectToBroker(const QString &host = "localhost", int port = 1883, int keepalive = 60, int max_retry = 3);
  void disconnectFromBroker();
  void subscribe(const QString &topic, int qos = 0);
  void publish(const QString &topic, const QByteArray &payload, int qos = 0, bool retain = false);
  QString clientId() const;
  bool mqttIsConnected();

 signals:
  void connected();
  void disconnected();
  void connectionFailed(const QString &reason);
  void messageReceived(const QString &topic, const QByteArray &payload, int qos, bool retain);

 private:
  void setupCallbacks();
  void cleanup();
  static void onConnect(struct mosquitto *mosq, void *obj, int rc);
  static void onDisconnect(struct mosquitto *mosq, void *obj, int rc);
  static void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
  static void onLog(struct mosquitto *mosq, void *obj, int level, const char *str);

 private slots:
  void handleReconnect();

 private:
  // 成员变量
  struct mosquitto *mosq_ = nullptr;
  QTimer *reconnect_timer_ = nullptr;
  QString host_;
  int port_ = 1883;
  int keepalive_ = 60;
  int max_retry_ = 3;
  int retry_count_ = 0;
  bool connected_ = false;
  QString client_id_;
};

#endif  // MQTTCLIENT_H
