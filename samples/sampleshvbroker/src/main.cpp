#include "brokerapp.h"
#include "version.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/network.h>

#include <QHostAddress>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("eyassrvctl");
	QCoreApplication::setApplicationVersion(APP_VERSION);

	BrokerApp::registerLogTopics();

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	int ret = 0;

	AppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if(cli_opts.isParseError()) {
		for(const std::string &err : cli_opts.parseErrors())
			shvError() << err;
		return EXIT_FAILURE;
	}
	if(cli_opts.isAppBreak()) {
		if(cli_opts.isHelp()) {
			cli_opts.printHelp(std::cout);
		}
		return EXIT_SUCCESS;
	}
	for(const std::string &s : cli_opts.unusedArguments()) {
		shvError() << "Undefined argument:" << s;
	}

	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	shv::chainpack::RpcMessage::registerMetaTypes();

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting eyassrvctl ver:" << APP_VERSION
				 << "PID:" << QCoreApplication::applicationPid()
				 << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	//shvInfo() << NecroLog::instantiationInfo();
	shvInfo() << "Primary IPv4 address:" << shv::iotqt::utils::Network::primaryIPv4Address().toString();
	shvInfo() << "Primary public IPv4 address:" << shv::iotqt::utils::Network::primaryPublicIPv4Address().toString();
	shvInfo() << "--------------------------------------------------------------------------------------";

	BrokerApp a(argc, argv, &cli_opts);

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "eyassrvctl bye ...";

	return ret;
}

