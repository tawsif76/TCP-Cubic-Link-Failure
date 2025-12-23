#ifndef TCP_CONSUMER_APP_H
#define TCP_CONSUMER_APP_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class TcpConsumerApp : public Application {
public:
    static TypeId GetTypeId(void);
    TcpConsumerApp();
    virtual ~TcpConsumerApp();

    void Setup(Address address, uint32_t totalContents, double q);

protected:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

private:
    void SendData(Ptr<Socket> socket, uint32_t txSpace);
    void CwndTracer(uint32_t oldVal, uint32_t newVal);
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
    Address m_peerAddress;
    uint32_t m_payloadSize;

    TracedCallback<Time> m_lastDelay;
    TracedCallback<Ptr<const Packet>> m_txTrace;
};

} 

#endif