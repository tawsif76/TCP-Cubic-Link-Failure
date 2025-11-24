#include "tcp-consumer-app.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpConsumerApp");
NS_OBJECT_ENSURE_REGISTERED(TcpConsumerApp);

TypeId TcpConsumerApp::GetTypeId() {
    static TypeId tid = TypeId("ns3::TcpConsumerApp")
        .SetParent<Application>()
        .SetGroupName("Applications")
        .AddConstructor<TcpConsumerApp>()
        .AddAttribute("PayloadSize", "Size of data packets",
                      IntegerValue(1024),
                      MakeIntegerAccessor(&TcpConsumerApp::m_payloadSize),
                      MakeIntegerChecker<uint32_t>())
        .AddTraceSource("LastDelay",
                        "One-way delay of Data packet",
                        MakeTraceSourceAccessor(&TcpConsumerApp::m_lastDelay),
                        "ns3::Time::TracedValueCallback")
        .AddTraceSource("Tx",
                        "A new packet is created and sent",
                        MakeTraceSourceAccessor(&TcpConsumerApp::m_txTrace),
                        "ns3::Packet::TracedCallback");
    return tid;
}

TcpConsumerApp::TcpConsumerApp()
    : m_socket(0),
      m_payloadSize(1024) {
    NS_LOG_FUNCTION(this);
}

TcpConsumerApp::~TcpConsumerApp() {
    NS_LOG_FUNCTION(this);
    m_socket = 0;
}

void TcpConsumerApp::Setup(Address address, uint32_t totalContents, double q) {
    NS_LOG_FUNCTION(this << address);
    m_peerAddress = address;
}

void TcpConsumerApp::StartApplication() {
    NS_LOG_FUNCTION(this);

    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    }

    if (m_socket->Bind() == -1) {
        NS_LOG_ERROR("Failed to bind socket");
        return;
    }

    // 2. Connect to the Producer
    m_socket->Connect(m_peerAddress);


    m_socket->SetRecvCallback(MakeCallback(&TcpConsumerApp::HandleRead, this));
    

    m_socket->SetSendCallback(MakeCallback(&TcpConsumerApp::SendData, this));

    NS_LOG_INFO("TcpConsumerApp started. Connected to " 
                << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4());
}

void TcpConsumerApp::StopApplication() {
    NS_LOG_FUNCTION(this);
    if (m_socket) {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
}

void TcpConsumerApp::SendData(Ptr<Socket> socket, uint32_t txSpace) {

    while (socket->GetTxAvailable() > m_payloadSize) {
        
        Ptr<Packet> p = Create<Packet>(m_payloadSize);

        TimestampTag tag;
        tag.SetTimestamp(Simulator::Now());
        p->AddByteTag(tag); // avoid using AddPacketTag, as TCP segments packets
        m_txTrace(p);
        socket->Send(p);
    }
}

void TcpConsumerApp::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    while ((packet = socket->Recv())) {
        
        ByteTagIterator it = packet->GetByteTagIterator();
        while (it.HasNext()) {
            ByteTagIterator::Item item = it.Next();
            if (item.GetTypeId() == TimestampTag::GetTypeId()) {
                TimestampTag tag;
                item.GetTag(tag);

                Time sendTime = tag.GetTimestamp();
                Time delay = Simulator::Now() - sendTime;

                if (delay.GetSeconds() >= 0) {
                    m_lastDelay(delay);
                }
                break; 
            }
        }
    }
}

} 