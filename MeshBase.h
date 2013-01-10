#ifndef MESH_BASE_H
#define MESH_BASE_H

#include <stdint.h>
#include "RF24.h"
#include "LinkedList.h"

#define PACKED __attribute__ ((packed))

typedef uint32_t address_t;

class MeshBase
{
public:
	MeshBase(uint8_t ce, uint8_t cs);

	struct MessageHeader {
		uint8_t		protocol_version : 4;
		uint8_t		ttl : 4;
		uint8_t		msg_id;
		bool		split_more : 1;
		uint8_t		split_part : 7;
		uint8_t		type;
		address_t	address_from;
	} PACKED;

	struct Message {
		Message(const MessageHeader& a) : header(a), data(NULL), data_used(0), blocks_recieved(0), next(NULL), prev(NULL), age(0) {}
		~Message();
		MessageHeader header;
		void* data;
		uint8_t data_used;
		uint8_t blocks_recieved;
		Message* next;
		Message* prev;
		uint8_t age;
		
		void AddPart(const void* data, uint8_t len, uint8_t part_num, bool more_parts);
		bool IsDone() const;
	};

	// -- Message types --
	enum MessageType {
		type_peer_discovery,
		type_peer_list,
		type_user,
	};

	enum ApplicationCapabilities {
		capability_publish_events = 1 >> 0,
	};

	void			Begin();
	void			Update();
	void			SendMessage(address_t address, uint8_t type, const void* data, uint8_t length);
	void			SendMessage(address_t address, uint8_t type, const void* data, uint8_t length, bool is_broadcast);
	address_t		GetAddress() const { return address; }
	bool			IsReady() const { return address != 0; }
protected:
	struct PeerDiscoveryMessage
	{
		uint8_t		protocol_version;
		uint8_t		network_capabilities; // What routing/networking can I do for the network
		uint8_t		application_capabilities; // What type of data do I expose
		uint16_t	num_peers; // Number of direct peers
		uint32_t	uptime; // Seconds since boot
	} PACKED;

	struct Peer {
		Peer(uint32_t address) : address(address), time(0), application_capabilities(0) {}
		uint32_t	address;
		uint8_t		time;
		uint8_t		application_capabilities;
		void Update(const PeerDiscoveryMessage* msg);
	};

	virtual void	OnMessage(const MessageHeader* meta, const void* data, uint8_t length) = 0;
	virtual void	OnNewPeer(Peer*) {}
	virtual void	OnLostPeer(Peer*) {}
	uint8_t			application_capabilities;
private:
	uint32_t		address;
	RF24			radio;
	unsigned long	last_broadcast_time;
	unsigned long	last_check_time;

	void			SendPeerDiscovery();
	void			HandlePeerDiscovery(const MessageHeader* msg, const void* buff, uint8_t length);
	void			HandlePacket(const byte* data, uint8_t length);
	void			ChooseAddress();

	LinkedList<Peer>	peers;
	LinkedList2<Message>	assembly_list;

	Peer*			GetPeer(uint32_t address);

};

#endif
