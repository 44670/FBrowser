#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QProcess>


void MainWindow::configureWebView() {
  // Disable anti-aliasing for better performance
  webView->setRenderHint(QPainter::Antialiasing, false);
  webView->setRenderHint(QPainter::TextAntialiasing, false);
  webView->setRenderHint(QPainter::SmoothPixmapTransform, false);
  // Enable basic JavaScript support
  webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
  webView->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
  webView->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled,
                                    true);
  webView->settings()->setAttribute(
      QWebSettings::OfflineWebApplicationCacheEnabled, true);

  // Enable WebAudio
  webView->settings()->setAttribute(QWebSettings::WebAudioEnabled, true);
  // Don't allow JavaScript to open/close windows
  webView->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                                    false);
  webView->settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows,
                                    false);
  // Allow JavaScript to access clipboard
  webView->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard,
                                    true);
  // Allow universal access from file URLs
  webView->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,
                                    true);
  webView->settings()->setAttribute(
      QWebSettings::LocalContentCanAccessRemoteUrls, true);

  // Show Web Inspector
  webView->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

  // Disallow cache
  
}

MainWindow::MainWindow()
    : QMainWindow(),
      webSocketServer("", QWebSocketServer::SslMode::NonSecureMode, this),
      jsBridge(this) ,
      osBridge(this) {
  loadConfig();
  if (optWebSocketServerPort > 0) {
    QHostAddress address;
    if (optWebSocketServerKey == "") {
      qDebug() << "[ws] WebSocketServer listening at 127.0.0.1 because no key was "
                  "provided.";
      address = QHostAddress::LocalHost;
    } else {
      qDebug() << "[ws] WebSocketServer listening at 0.0.0.0 (AnyIPv4) because a key "
                  "was provided.";
      address = QHostAddress::AnyIPv4;
    }
    // TODO: Wait and retry if port is busy
    if (webSocketServer.listen(address, optWebSocketServerPort)) {
      qDebug() << "WebSocketServer listening on port" << optWebSocketServerPort;
    } else {
      qDebug() << "WebSocketServer failed to listen on port"
               << optWebSocketServerPort;
    }
    connect(&webSocketServer, SIGNAL(newConnection()), this,
            SLOT(onNewWebSocketConnection()));
  }

  // Load proxy settings
  if (optProxyHost != "") {
    qDebug() << "Setting proxy to" << optProxyHost << ":" << optProxyPort;
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(optProxyHost);
    proxy.setPort(optProxyPort);
    QNetworkProxy::setApplicationProxy(proxy);
  }

  // Set window size
  if (optWidth) {
      resize(optWidth, optHeight);
  }

  // Make our window full-screen
  // setWindowState(Qt::WindowFullScreen);

  // Create the web view
  webView = new QWebView(this);
  configureWebView();
  // Add the web view to our window
  setCentralWidget(webView);
  // Handle page load events
  connect(webView, SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));

  // After 1000ms, load the URL
  QTimer::singleShot(1000, this, SLOT(loadStartupUrl()));
}

MainWindow::~MainWindow() {}

void MainWindow::onLoadStarted() { 
    qDebug() << "onLoadStarted"; 
    bool enableJSBridge = false;
    bool enableOSBridge = false;

    enableJSBridge = optEnableJSBridge;
    enableOSBridge = optEnableOSBridge;

    if (enableJSBridge) {
      webView->page()->mainFrame()->addToJavaScriptWindowObject("FB_JSBridge", &jsBridge);
    }
    if (enableOSBridge) {
      webView->page()->mainFrame()->addToJavaScriptWindowObject("FB_OSBridge", &osBridge);
    }                                                          
}

void MainWindow::loadUrl(QString url) {
  if (url.startsWith("./")) {
    url = "file:///" + QDir::currentPath() + QDir::separator() + url.mid(2);
  }
  qDebug() << "Loading URL:" << url;
  webView->load(QUrl(url));
}

void MainWindow::loadStartupUrl() { loadUrl(optStartupUrl); }

void MainWindow::loadConfig() {
  bool haveUrlInCmdLine = false;
  // Check for url in argv
  if (qApp->arguments().size() > 1) {
    optStartupUrl = qApp->arguments()[1];
    haveUrlInCmdLine = true;
  }
  // Load config.json
  QFile configFile("config.json");
  if (!configFile.open(QIODevice::ReadOnly)) {
    qDebug() << "Could not open config.json";
    return;
  }
  QByteArray configData = configFile.readAll();
  configFile.close();
  QJsonDocument configJson = QJsonDocument::fromJson(configData);
  QJsonObject configObject = configJson.object();
  // Load url
  if (configObject.contains("url") && (!haveUrlInCmdLine)) {
    optStartupUrl = configObject["url"].toString();
  }
  // Load websocket port
  if (configObject.contains("wsServerPort")) {
    optWebSocketServerPort = configObject["wsServerPort"].toInt();
  }
  // Load websocket key
  if (configObject.contains("wsServerKey")) {
    optWebSocketServerKey = configObject["wsServerKey"].toString();
  }
  // Load window size
  if (configObject.contains("width")) {
    optWidth = configObject["width"].toInt();
  }
  if (configObject.contains("height")) {
    optHeight = configObject["height"].toInt();
  }
  // Load proxy settings
  if (configObject.contains("proxyHost")) {
    optProxyHost = configObject["proxyHost"].toString();
  }
  if (configObject.contains("proxyPort")) {
    optProxyPort = configObject["proxyPort"].toInt();
  }
  if (configObject.contains("enableJSBridge")) {
    optEnableJSBridge = configObject["enableJSBridge"].toBool();
  }
  if (configObject.contains("enableOSBridge")) {
    optEnableOSBridge = configObject["enableOSBridge"].toBool();
  }
}

void MainWindow::onNewWebSocketConnection() {
  bool isAuthenticated = false;

  qDebug() << "[ws] New WebSocket connection";
  QWebSocket *webSocket = webSocketServer.nextPendingConnection();

  QString host = webSocket->peerAddress().toString();
  qDebug() << "[ws] Connection from " << host;
  QString clientSentKey = webSocket->request().rawHeader("X-FB-Key");
  if (optWebSocketServerKey != "") {
    if (clientSentKey == optWebSocketServerKey) {
      qDebug() << "[ws] Accepted connection from " << host << " by key.";
      isAuthenticated = true;
    }
  } else {
    // Only allow 127.0.0.1 if no key is set
    if (host == "127.0.0.1") {
      qDebug() << "[ws] Accepted connection from " << host << " by default.";
      isAuthenticated = true;
    } else {
      qDebug() << "[ws] Rejected connection from " << host << " by default.";
    }
  }
  if (!isAuthenticated) {
    webSocket->close();
    return;
  }

  connect(webSocket, SIGNAL(textMessageReceived(QString)), this,
          SLOT(onWebSocketTextMessageReceived(QString)));
  connect(webSocket, SIGNAL(disconnected()), webSocket, SLOT(deleteLater()));
}

void MainWindow::onWebSocketTextMessageReceived(QString message) {
  qDebug() << "[ws] Received message: " << message;
  QWebSocket *webSocket = qobject_cast<QWebSocket *>(sender());
  // Parse message as JSON Array
  QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
  QJsonArray jsonArray = jsonDoc.array();
  if (jsonArray.size() == 0) {
    return;
  }
  QString cmd = jsonArray[0].toString();
  QString arg1 = jsonArray[1].toString();
  QJsonArray resp = QJsonArray();
  resp.append("ok");
  if (cmd == "loadUrl") {
    qDebug() << "[ws] Loading URL: " << arg1;
    webView->load(QUrl(arg1));
  } else if (cmd == "clearCache") {
    qDebug() << "[ws] Clearing cache";
    webView->page()->networkAccessManager()->clearAccessCache();
    webView->page()->networkAccessManager()->clearConnectionCache();
  } else if (cmd == "exit") {
    qDebug() << "[ws] Exiting";
    QApplication::quit();
  } else if (cmd == "eval") {
    // Eval JavaScript in current page
    qDebug() << "[ws] Evaluating JavaScript: " << arg1;
    resp.append(
        webView->page()->mainFrame()->evaluateJavaScript(arg1).toString());
  }

  webSocket->sendTextMessage(QJsonDocument(resp).toJson());
}


QString OSBridge::runCmd(QStringList args) {
  QProcess proc;
  QString program = args[0];
  QStringList arguments = args.mid(1);
  proc.start(program, arguments);
  proc.waitForFinished();
  QString output = proc.readAllStandardOutput();
  return output;
}