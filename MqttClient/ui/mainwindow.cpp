#include "mainwindow.h"

#include <QMessageBox>
#include <QTimer>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), received_retained_messages_() {
  ui->setupUi(this);
  mqtt_client_ = new MqttClient(this);

  // 连接信号
  connect(mqtt_client_, &MqttClient::connected, this, &MainWindow::onConnected);
  connect(mqtt_client_, &MqttClient::messageReceived, this, &MainWindow::onMessage);
  connect(mqtt_client_, &MqttClient::connectionFailed, this, &MainWindow::onConnectionFailed);

  // 连接参数配置
  mqtt_client_->connectToBroker("broker.hivemq.com", 1883, 60, 5);
}

MainWindow::~MainWindow() {
  if (mqtt_client_) {
    delete mqtt_client_;
    mqtt_client_ = nullptr;
  }
  delete ui;
}

void MainWindow::publishMessage() {
  // 正确代码
  QString msg = "[From:" + mqtt_client_->clientId() + "]";
  mqtt_client_->publish("myapp/status", msg.toUtf8());  // 使用 toUtf8()
}

void MainWindow::onConnected() {  // 订阅示例主题
  mqtt_client_->subscribe("myapp/status", 1);

  // 发布初始消息
  mqtt_client_->publish("myapp/status", QByteArray("Client connected"), 1, true);
}

void MainWindow::onMessage(const QString &topic, const QByteArray &payload, int qos, bool retain) {
  // 情况1:过滤己方消息
  QString message = QString::fromUtf8(payload).trimmed();
  QString selfPrefix = "[ClientID:" + mqtt_client_->clientId() + "]";
  if (payload.startsWith(selfPrefix.toUtf8())) {
    qDebug() << "完整过滤自身消息:" << message;
    return;
  }

  // 情况2: 处理未标识来源的消息
  if (!message.contains("[ClientID:")) {
    qWarning() << "收到未标识来源的消息，建议隔离处理：" << message;
    return;
  }

  // 处理保留消息去重
  if (retain) {
    // 如果已记录过该主题的保留消息
    if (received_retained_messages_.contains(topic)) {
      qDebug() << "Ignored duplicate retained message on topic:" << topic;
      return;
    }
    // 记录新主题的保留消息
    received_retained_messages_.insert(topic);
  }

  // 处理QoS0消息的特殊情况
  if (qos == 0 && message.contains(mqtt_client_->clientId())) {
    qDebug() << "过滤QoS0自身消息：" << message;
    return;
  }

  // 使用明确的占位符顺序
  // QString display_msg = QString("[QoS%1][%2] Topic: %3 | Message: %4")
  //                           .arg(qos)                          // %1 → QoS等级
  //                           .arg(retain ? "R" : " ")           // %2 → 保留标志（R/空格）
  //                           .arg(topic)                        // %3 → 主题
  //                           .arg(QString::fromUtf8(payload));  // %4 → 消息内容
  QString retain_flag = retain ? "R" : " ";
  QString display_msg = QString("[QoS%1][%2] Topic: %3 | Message: %4")
                            .arg(qos)
                            .arg(retain_flag)
                            .arg(topic)
                            .arg(QString::fromUtf8(payload));
  qDebug() << display_msg;
}

void MainWindow::onConnectionFailed(const QString &reason) {
  // 显示错误对话框
  QMessageBox::critical(this, tr("连接错误"), tr("无法连接到MQTT服务器：\n%1").arg(reason));
}
