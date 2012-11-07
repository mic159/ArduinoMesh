#ifndef MESH_BASE_H
#define MESH_BASE_H

#include <stdint.h>
#include "RF24.h"
#include "LinkedList.h"

class MeshBase
{
public:
	MeshBase(uint8_t ce, uint8_t cs);

	struct Peer {
		uint32_t	address;
		uint16_t	time;
		Peer(uint32_t address) : address(address), time(0) {}
	};
	
	struct Message
	{
		uint8_t		protocol_version;
		uint8_t		ttl;
		uint8_t		type;
		bool		split_enabled :  1;
		uint8_t		split_part : 7;
		uint32_t	address_from;
	};

	// -- Message types --
	enum message_type {
		type_peer_discovery,
		type_peer_list,
		type_user,
	};

	void			Begin();
	void			Update();
	void			SendMessage(uint32_t address, uint8_t type, const void* data, uint8_t length);
	uint32_t		GetAddress() const { return address; }
	bool			IsReady() const { return address != 0; }
protected:
	virtual void	OnMessage(const MeshBase::Message* meta, const void* data, uint8_t length) = 0;
	virtual void	OnNewPeer(Peer*) {}
	virtual void	OnLostPeer(Peer*) {}
private:
	uint32_t		address;
	RF24			radio;
	unsigned long	last_broadcast_time;
	unsigned long	last_peer_check_time;

	void			SendPeerDiscovery();
	void			SendMessage(uint32_t address, uint8_t type, const void* data, uint8_t length, bool is_broadcast);
	void			HandlePeerDiscovery(const Message* msg, const void* buff, uint8_t length);
	void			HandleMessage(const Message* msg, const void* data, uint8_t length);
	void			ChooseAddress();
	
	LinkedList<Peer>	peers;
	
	Peer*			GetPeer(uint32_t address);
	
	struct PeerDiscoveryMessage
	{
		uint8_t		protocol_version;
		uint8_t		network_capabilities; // What routing/networking can I do for the network
		uint8_t		application_capabilities; // What type of data do I expose
		uint16_t	num_peers; // Number of direct peers
		uint32_t	uptime; // Seconds since boot
	};

};

#endif
