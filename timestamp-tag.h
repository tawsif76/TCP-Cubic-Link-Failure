#ifndef TIMESTAMP_TAG_H
#define TIMESTAMP_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class TimestampTag : public Tag {
public:
  static TypeId GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::TimestampTag")
      .SetParent<Tag> ()
      .AddConstructor<TimestampTag> ();
    return tid;
  }

  virtual TypeId GetInstanceTypeId (void) const {
    return GetTypeId ();
  }

  virtual uint32_t GetSerializedSize (void) const {
    return sizeof (int64_t);
  }

void Serialize(TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds();
  i.Write((const uint8_t*)&t, 8);
}

void Deserialize(TagBuffer i)
{
  int64_t t;
  i.Read((uint8_t*)&t, 8);
  m_timestamp = NanoSeconds(t);
}

  virtual void Print (std::ostream &os) const {
    os << "t=" << m_timestamp;
  }

  // Setters and Getters
  void SetTimestamp (Time t) { m_timestamp = t; }
  Time GetTimestamp (void) const { return m_timestamp; }

private:
  Time m_timestamp;
};

} // namespace ns3

#endif