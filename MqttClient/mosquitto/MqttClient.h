#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <mosquitto.h>

#include <QObject>
#include <QTimer>

/*!
 * \brief MQTT客户端类，封装Mosquitto C API，用于管理和操作MQTT连接。
 * 实现了与MQTT代理的连接、断开连接、消息发布和订阅等功能，并提供了相应的信号以便与其他组件交互。
 */

class MqttClient : public QObject {
  Q_OBJECT
 public:
  /*!
   * \brief 构造函数
   * \param parent 父对象，用于Qt的对象树管理
   */
  explicit MqttClient(QObject *parent = nullptr);

  /*!
   * \brief 析构函数
   * 负责清理分配的资源，如Mosquitto实例和定时器。
   */
  ~MqttClient();

  /*!
   * \brief 连接到MQTT代理
   * \param host MQTT代理的主机地址
   * \param port MQTT代理的端口号，默认为1883（非加密端口）
   * \param keepalive 保持连接的时间间隔，默认为60秒
   * \param max_retry 最大重试次数，默认为3次
   * \param username 可选的用户名，用于认证
   * \param password 可选的密码，用于认证
   * \return 连接是否成功
   */
  bool connectToBroker(const QString &host, int port = 1883, int keepalive = 60, int max_retry = 3,
                       const QString &username = "", const QString &password = "");

  /*!
   * \brief 从MQTT代理断开连接
   * 清理资源并停止网络循环。
   */
  void disconnectFromBroker();

  /*!
   * \brief 订阅指定主题
   * \param topic 要订阅的主题
   * \param qos 订阅的消息质量等级，默认为0
   */
  void subscribe(const QString &topic, int qos = 0);

  /*!
   * \brief 发布消息到指定主题
   * \param topic 消息要发布到的主题
   * \param payload 消息内容
   * \param qos 发布的消息质量等级，默认为0
   * \param retain 是否将消息设置为保留消息，默认为false
   */
  void publish(const QString &topic, const QByteArray &payload, int qos = 0, bool retain = false);

  /*!
   * \brief 获取客户端ID
   * \return 当前客户端的唯一ID
   */
  QString clientId() const;

  /*!
   * \brief 检查是否已连接到MQTT代理
   * \return 是否已连接
   */
  bool mqttIsConnected();

  /*!
   * \brief 设置MQTT代理的用户名和密码
   * \param username 用户名
   * \param password 密码
   */
  void setCredentials(const QString &username, const QString &password);

  /*!
   * \brief 启用SSL/TLS加密
   * \param caFile CA证书文件路径
   * \param certFile 客户端证书文件路径（可选）
   * \param keyFile 客户端密钥文件路径（可选）
   */
  void enableSSL(const QString &caFile, const QString &certFile = "", const QString &keyFile = "");
  void startHearbeat(int interval);

 signals:
  /*!
   * \brief 连接成功信号
   * 当成功连接到MQTT代理时发出。
   */
  void connected();
  void disconnected();

  /*!
   * \brief 连接失败信号
   * 当连接MQTT代理失败时发出。
   * \param reason 连接失败的原因
   */
  void connectionFailed(const QString &reason);

  /*!
   * \brief 消息接收信号
   * 当从MQTT代理接收到消息时发出。
   * \param topic 消息的主题
   * \param payload 消息的内容
   * \param qos 消息的质量等级
   * \param retain 消息是否为保留消息
   */
  void messageReceived(const QString &topic, const QByteArray &payload, int qos, bool retain);

 private:
  /*!
   * \brief 设置Mosquitto的回调函数
   */
  void setupCallbacks();

  /*!
   * \brief 清理资源
   */
  void cleanup();

  /*!
   * \brief 连接回调函数
   * \param mosq Mosquitto实例指针
   * \param obj 用户数据指针
   * \param rc 连接结果代码
   */
  static void onConnect(struct mosquitto *mosq, void *obj, int rc);

  /*!
   * \brief 断开连接回调函数
   * \param mosq Mosquitto实例指针
   * \param obj 用户数据指针
   * \param rc 断开连接原因代码
   */
  static void onDisconnect(struct mosquitto *mosq, void *obj, int rc);

  /*!
   * \brief 消息接收回调函数
   * \param mosq Mosquitto实例指针
   * \param obj 用户数据指针
   * \param msg 接收到的消息
   */
  static void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

  /*!
   * \brief 日志回调函数
   * \param mosq Mosquitto实例指针
   * \param obj 用户数据指针
   * \param level 日志级别
   * \param str 日志消息
   */
  static void onLog(struct mosquitto *mosq, void *obj, int level, const char *str);

 private slots:
  /*!
   * \brief 重连处理槽函数
   */
  void handleReconnect();

 private:
  struct mosquitto *mosq_ = nullptr;   // Mosquitto实例指针
  QTimer *reconnect_timer_ = nullptr;  // 重连定时器
  QString host_;                       // MQTT代理主机地址
  int port_ = 1883;                    // MQTT代理端口号
  int keepalive_ = 60;                 // 保持连接时间间隔
  int max_retry_ = 3;                  // 最大重试次数
  int retry_count_ = 0;                // 当前重试次数
  bool connected_ = false;             // 是否已连接
  QString client_id_;                  // 客户端ID
  QString username_;                   // 用户名
  QString password_;                   // 密码
};

#endif  // MQTTCLIENT_H
