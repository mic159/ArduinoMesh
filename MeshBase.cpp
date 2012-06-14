#include <Arduino.h>
#include <RF24.h>
#include "MeshBase.h"

// -- Broadcast addresses --
#define PEER_DISCOVERY 1

// -- Helpers --
#define TO_BROADCAST(x) (0xBB00000000LL + x)
#define TO_ADDRESS(x) (0xAA00000000LL + x)

#define PEER_DISCOVERY_TIME 3000
#define PEER_CHECK_TIME 4000
#define PEER_TIMEOUT 2

MeshBase::MeshBase(uint8_t ce, uint8_t cs)
: radio(ce, cs)
, address(0)
, last_broadcast_time(0)
, last_peer_check_time(0)
{}

void MeshBase::Begin()
{
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setRetries(2,1);
	//radio.openReadingPipe(0, TO_ADDRESS(address));
	radio.openReadingPipe(1, TO_BROADCAST(PEER_DISCOVERY));
	radio.setAutoAck(0, true);
	radio.setAutoAck(1, false);
	radio.startListening();
}

void MeshBase::Update()
{
	// Periodic sends
	if (millis() - last_broadcast_time > PEER_DISCOVERY_TIME)
	{
    		if (!IsReady())
			ChooseAddress();
		SendPeerDiscovery();
	}

	// Recieve
	uint8_t pipe_num;
	if (radio.available(&pipe_num))
	{
		bool done = false;
		do {
			uint8_t len = radio.getDynamicPayloadSize();
			uint8_t buff[40];
			done = radio.read(buff, min(len, sizeof(buff)));
			if (pipe_num == 0)
			{
				HandleMessage(0, buff, len);
			} else if (pipe_num == 1) {
				HandlePeerDiscovery(buff, len);
			}
		} while (!done);
	}
	
	// Update peers
	if (millis() - last_peer_check_time > PEER_CHECK_TIME)
	{
		Peer* current = first;
		while(current != NULL)
		{
			current->time += 1;
			if (current->time >= PEER_TIMEOUT)
			{
				current = RemovePeer(current);
			} else {
				current = current->next;
			}
		}
		last_peer_check_time = millis();
	}
}

void MeshBase::HandlePeerDiscovery(void* buff, uint8_t length)
{
	if (length != sizeof(uint32_t))
		return;
	uint32_t from = *(uint32_t*)buff;
	// Dont know why, but this keeps happening?
	if (from == 0)
		return;

	Peer* peer = GetPeer(from);
	if (peer == NULL)
	{
		// Found a new peer
		AddPeer(from);
	} else {
		// Existing peer, reset timer
		peer->time = 0;
	}
}

void MeshBase::SendPeerDiscovery()
{
	last_broadcast_time = millis();
	SendBroadcastMessage(PEER_DISCOVERY, &address, sizeof(address));
}

void MeshBase::SendBroadcastMessage(uint32_t to, const void* data, uint8_t length)
{
	radio.stopListening();
	radio.openWritingPipe(TO_BROADCAST(to));
	radio.write(data, length);
	radio.startListening();
}

void MeshBase::SendMessage(uint32_t to, const void* data, uint8_t length)
{
	radio.stopListening();
	radio.openWritingPipe(TO_ADDRESS(to));
	radio.write(data, length);
	radio.startListening();
}

void MeshBase::ChooseAddress()
{
	do {
		address = random(0xFFFF);
	} while(GetPeer(address) != NULL);

	radio.openReadingPipe(0, TO_ADDRESS(address));
	Serial.print("Chose address: ");
	Serial.println(address, DEC);
}

void MeshBase::AddPeer(uint32_t a)
{
	Serial.print("New Peer: ");
	Serial.println(a, DEC);
	Peer* n = new Peer(a);
	if (last == NULL)
	{
		// Empty list.
		first = n;
		last = n;
	} else {
		// Attach onto end
		last->next = n;
		n->prev = last;
		last = n;
	}
	OnNewPeer(n);
}

MeshBase::Peer* MeshBase::GetPeer(uint32_t a)
{
	Peer* current = first;
	while(current != NULL)
	{
		if (current->address == a)
			return current;
		current = current->next;
	}
	// Could not find..
	return NULL;
}

MeshBase::Peer* MeshBase::RemovePeer(MeshBase::Peer* p)
{
	Serial.print("Lost Peer: ");
	Serial.println(p->address, DEC);
	OnLostPeer(p);
	Peer* next = p->next;
	if (first == p)
		first = p->next;
	if (last == p)
		last = p->prev;
	if (p->prev)
		p->prev->next = p->next;
	if (p->next)
		p->next->prev = p->prev;
	delete p;
	return next;
}

