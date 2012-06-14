#ifndef MESH_BASE_H
#define MESH_BASE_H

#include <stdint.h>
#include "RF24.h"

class MeshBase
{
public:
	MeshBase(uint8_t ce, uint8_t cs);

	struct Peer {
		uint32_t	address;
		uint16_t	time;
		Peer*		next;
		Peer*		prev;
		Peer(uint32_t address) : address(address), time(0), next(0), prev(0) {}
	};
	
	void			Begin();
	void			Update();
	void			SendMessage(uint32_t address, const void* data, uint8_t length);
	uint32_t		GetAddress() const { return address; }
	bool			IsReady() const { return address != 0; }
protected:
	virtual void	HandleMessage(uint32_t sender, const void* data, uint8_t length) = 0;
	virtual void	OnNewPeer(Peer*) {}
	virtual void	OnLostPeer(Peer*) {}
private:
	uint32_t		address;
	RF24			radio;
	unsigned long	last_broadcast_time;
	unsigned long	last_peer_check_time;

	void			SendPeerDiscovery();
	void			SendBroadcastMessage(uint32_t address, const void* data, uint8_t length);
	void			HandlePeerDiscovery(void* buff, uint8_t length);
	void			ChooseAddress();
	
	Peer*	first;
	Peer*	last;
	
	Peer*			GetPeer(uint32_t address);
	void			AddPeer(uint32_t address);
	Peer*			RemovePeer(Peer* peer);

};

#endif
