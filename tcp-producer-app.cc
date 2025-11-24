#include "tcp-producer-app.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpProducerApp");
NS_OBJECT_ENSURE_REGISTERED(TcpProducerApp);

TypeId TcpProducerApp::GetTypeId() {
    static TypeId tid = TypeId("ns3::TcpProducerApp")
        .SetParent<Application>()
        .SetGroupName("Applications")
        .AddConstructor<TcpProducerApp>()
        .AddTraceSource("InterestDelay",
                        "One-way delay of Interest packet (Consumer->Producer)",
                        MakeTraceSourceAccessor(&TcpProducerApp::m_interestDelay),
                        "ns3::Time::TracedValueCallback");
    return tid;
}

TcpProducerApp::TcpProducerApp()
    : m_payloadSize(1024) {
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("TcpProducerApp Constructor");
}

TcpProducerApp::~TcpProducerApp() {
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("TcpProducerApp Destructor");
}

void TcpProducerApp::Setup(uint32_t payloadSize) {
    NS_LOG_FUNCTION(this << payloadSize);
    m_payloadSize = payloadSize;
    NS_LOG_INFO("Producer setup: payloadSize=" << payloadSize);
}

void TcpProducerApp::StartApplication() {
    NS_LOG_FUNCTION(this);

    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    InetSocketAddress local(Ipv4Address::GetAny(), 9000);

    NS_LOG_INFO("Producer " << GetNode()->GetId() 
                << " binding to port 9000");

    if (m_socket->Bind(local) == -1) {
        NS_LOG_ERROR("Failed to bind producer socket!");
        return;
    }

    m_socket->Listen();

    m_socket->SetAcceptCallback(
        MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
        MakeCallback(&TcpProducerApp::HandleAccept, this));

    NS_LOG_INFO("Producer " << GetNode()->GetId()
                << " listening on port 9000");
}

void TcpProducerApp::StopApplication() {
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Producer " << GetNode()->GetId() 
                << " stopping application");

    if (m_socket) {
        m_socket->Close();
    }
}

void TcpProducerApp::HandleAccept(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(this << socket << from);

    InetSocketAddress addr = InetSocketAddress::ConvertFrom(from);
    NS_LOG_INFO("Producer " << GetNode()->GetId()
                << " accepted connection from " << addr.GetIpv4());

    socket->SetRecvCallback(MakeCallback(&TcpProducerApp::HandleRead, this));
}

void TcpProducerApp::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    while ((packet = socket->Recv())) {

        NS_LOG_DEBUG("Producer received " << packet->GetSize()
                      << " bytes at " << Simulator::Now().GetSeconds());

       
        ByteTagIterator i = packet->GetByteTagIterator();
        while (i.HasNext()) {
            ByteTagIterator::Item item = i.Next();
            if (item.GetTypeId() == TimestampTag::GetTypeId()) {

                TimestampTag incomingTag;
                item.GetTag(incomingTag);

                Time t1 = incomingTag.GetTimestamp();
                Time delay = Simulator::Now() - t1;

                NS_LOG_INFO("Interest Delay: " << delay.GetSeconds() << " us");

                m_interestDelay(delay);
                break;
            }
        }

   
        Ptr<Packet> response = Create<Packet>(m_payloadSize);

        TimestampTag outgoingTag;
        outgoingTag.SetTimestamp(Simulator::Now());
        response->AddByteTag(outgoingTag);

        NS_LOG_DEBUG("Producer sending response of size "
                     << m_payloadSize << " bytes");

        socket->Send(response);
    }
}

} 
