#ifndef TCP_PRODUCER_APP_H
#define TCP_PRODUCER_APP_H

#include "ns3/traced-callback.h"
#include "ns3/application.h"
#include "ns3/socket.h"

namespace ns3 {

class TcpProducerApp : public Application {
public:

    static TypeId GetTypeId();
    TcpProducerApp();
    virtual ~TcpProducerApp();

    void Setup(uint32_t payloadSize);

private:
    virtual void StartApplication();
    virtual void StopApplication();

    void HandleAccept(Ptr<Socket> socket, const Address& from);
    void HandleRead(Ptr<Socket> socket);

    uint32_t m_payloadSize; // 1024 bytes
    Ptr<Socket> m_socket;
    TracedCallback<Time> m_interestDelay;
};

} 

#endif
