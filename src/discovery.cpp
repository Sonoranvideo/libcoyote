#include "discovery.h"
#include "native_ws.h"
#include <QtNetwork>

Discovery::DiscoverySession *Discovery::DiscoverySession::Instance;

Discovery::DiscoverySession::DiscoverySession(void)
{
	if (DiscoverySession::Instance)
	{
		throw std::runtime_error{"Multiple instances of singleton class DiscoverySession!"};
	}
	
	this->Instance = this;
	
	this->Thread.reset(QThread::create([this] { this->ThreadFunc(); }));
	this->Thread->start();
}

void Discovery::DiscoverySession::ThreadFunc(void)
{
	this->Sock.reset(new QUdpSocket);
	this->Sock->bind(WS::PortNum, QUdpSocket::ShareAddress);
	
	std::unique_ptr<QTimer> Timer { new QTimer };
	
	QObject::connect(Timer.get(), &QTimer::timeout, [this] { this->ProcessBroadcasts(); });
	
	Timer->start(25);
	
	QEventLoop Loopy;
	
	Loopy.exec();

}

void Discovery::DiscoverySession::ProcessBroadcasts(void)
{
	while (this->Sock->pendingDatagramSize() == sizeof(InternalBCastMsg))
	{
		InternalBCastMsg Msg{};
		
		QHostAddress Origin{};
		
		if (this->Sock->readDatagram((char*)&Msg, sizeof Msg, &Origin) != sizeof(Msg))
		{
			continue; //Wrong size, which somehow got past the first check.
		}
	
		const std::lock_guard<std::mutex> G { this->KnownCoyotesLock };
		
		this->KnownCoyotes.emplace(std::string{Msg.GUID}, Msg.ToPublicStruct(NormalizeIPV4(qs2cs(Origin.toString()))));
	}
}

std::vector<Coyote::LANCoyote> Discovery::DiscoverySession::GetLANCoyotes(void)
{
	const std::lock_guard<std::mutex> G { this->KnownCoyotesLock };
	
	std::vector<Coyote::LANCoyote> RetVal;
	RetVal.reserve(this->KnownCoyotes.size());
	
	for (const auto &Pair : this->KnownCoyotes)
	{
		RetVal.emplace_back(Pair.second);
	}
	
	return RetVal;
}
