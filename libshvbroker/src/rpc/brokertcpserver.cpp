#include "brokertcpserver.h"
#include "clientconnectiononbroker.h"
#include "../brokerapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/socket.h>
#include <QSslSocket>

#include <QFile>
#include <QDir>
#include <QSslKey>

namespace shv {
namespace broker {
namespace rpc {

static QSslConfiguration load_ssl_configuration()
{
	AppCliOptions *opts = BrokerApp::instance()->cliOptions();
	QString cert_fn = QString::fromStdString(opts->serverSslCertFile());
	if(QDir::isRelativePath(cert_fn))
		cert_fn = QString::fromStdString(opts->configDir()) + '/' + cert_fn;
	QFile cert_file(cert_fn);
	cert_file.open(QIODevice::ReadOnly);
	QSslCertificate ssl_cert(&cert_file, QSsl::Pem);
	cert_file.close();
	shvDebug() << "CERT:" << ssl_cert.toText();

	QString key_fn = QString::fromStdString(opts->serverSslKeyFile());
	if(QDir::isRelativePath(key_fn))
		key_fn = QString::fromStdString(opts->configDir()) + '/' + key_fn;
	QFile key_file(key_fn);
	key_file.open(QIODevice::ReadOnly);
	QSslKey ssl_key(&key_file, QSsl::Rsa, QSsl::Pem);
	key_file.close();

	QSslConfiguration ssl_configuration;
	ssl_configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
	ssl_configuration.setLocalCertificate(ssl_cert);
	ssl_configuration.setPrivateKey(ssl_key);
	//ssl_configuration.setProtocol(QSsl::TlsV1SslV3);
	return ssl_configuration;
}

BrokerTcpServer::BrokerTcpServer(SslMode ssl_mode, QObject *parent)
	: Super(parent), m_sslMode(ssl_mode)
{
	if (m_sslMode == SslMode::SecureMode)
		m_sslConfiguration = load_ssl_configuration();
}

ClientConnectionOnBroker *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ClientConnectionOnBroker *>(Super::connectionById(connection_id));
}

void BrokerTcpServer::incomingConnection(qintptr socket_descriptor)
{
	shvLogFuncFrame() << socket_descriptor;
	if (m_sslMode == SecureMode) {
		QSslSocket *socket = new QSslSocket(this);
		{
			connect(socket, &QSslSocket::connected, [this]() {
				shvDebug() << this << "Socket connected!!!";
			});
			connect(socket, &QSslSocket::disconnected, [this]() {
				shvDebug() << this << "Socket disconnected!!!";
			});
			connect(socket, &QSslSocket::stateChanged, [this](QAbstractSocket::SocketState state) {
				shvDebug() << this << "Socket state changed:" << state;
			});
			connect(socket, &QSslSocket::errorOccurred, this, [socket](QAbstractSocket::SocketError socket_error) {
				shvWarning() << "Socket error:" << socket_error << socket->errorString();
			});
			connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), [](const QList<QSslError> &errors){
				shvWarning() << "SSL Errors:";
				for(const auto &err : errors)
					shvWarning() << err.errorString();
			});
			connect(socket, &QSslSocket::peerVerifyError, [](const QSslError &error) {
				shvWarning() << "SSL peer verify errors:" << error.errorString();
			});

		}
		if (!socket->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		shvDebug() << "startServerEncryption";
		socket->setSslConfiguration(m_sslConfiguration);
		socket->startServerEncryption();
		addPendingConnection(socket);
	} else if (m_sslMode == NonSecureMode) {
		QTcpSocket *sock = new QTcpSocket(this);
		if (!sock->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		addPendingConnection(sock);
	}
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	if (m_sslMode == SecureMode) {
		//shvDebug() << "startServerEncryption";
		//qobject_cast<QSslSocket *>(socket)->startServerEncryption();
		return new ClientConnectionOnBroker(new shv::iotqt::rpc::SslSocket(qobject_cast<QSslSocket *>(socket)), parent);
	}
	else {
		return new ClientConnectionOnBroker(new shv::iotqt::rpc::TcpSocket(socket), parent);
	}
}

}}}
