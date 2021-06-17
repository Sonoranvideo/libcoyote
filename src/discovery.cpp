/*
   Copyright 2021 Sonoran Video Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
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
