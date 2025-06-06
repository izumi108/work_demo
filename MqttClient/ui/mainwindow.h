#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QThread>

#include "MqttClient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 signals:
  void requestConnect(const QString& host, int port, int keepalive, int max_retry, const QString& username,
                      const QString& password);
  void requestPublish(const QString& topic, const QByteArray& payload, int qos, bool retain);

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();
  void publishMessage();

 private slots:
  void onConnected();
  void onMessage(const QString& topic, const QByteArray& payload, int qos, bool retain);
  void onConnectionFailed(const QString& reason);
  void onButtonClicked();
  void onButton1Clicked();

 private:
  Ui::MainWindow* ui;
  MqttClient* mqtt_client_;
  QThread* mqtt_thread_;

  QSet<QString> received_retained_messages_;
};
#endif  // MAINWINDOW_H
