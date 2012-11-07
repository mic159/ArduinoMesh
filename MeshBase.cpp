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
			
			const MeshBase::Message* msg = (struct MeshBase::Message*)buff;
			uint8_t payload_length = len - sizeof(Message);
			const uint8_t* payload = buff + sizeof(Message);
			
			switch(msg->type) {
				case type_peer_discovery:
					HandlePeerDiscovery(msg, payload, payload_length);
				break;
				default:
					OnMessage(msg, payload, payload_length);
				break;
			}
		} while (!done);
	}
	
	// Update peers
	if (millis() - last_peer_check_time > PEER_CHECK_TIME)
	{
		LinkedList<Peer>::Node* current = peers.first;
		while(current != NULL)
		{
			current->item->time += 1;
			if (current->item->time >= PEER_TIMEOUT)
			{
				Serial.print("Lost Peer: ");
				Serial.println(current->item->address, DEC);
				current = peers.Remove(current);
			} else {
				current = current->next;
			}
		}
		last_peer_check_time = millis();
	}
}

void MeshBase::HandlePeerDiscovery(const MeshBase::Message* msg, const void* buff, uint8_t length)
{
	if (length != sizeof(PeerDiscoveryMessage))
		return;
	const PeerDiscoveryMessage* pd = (struct PeerDiscoveryMessage*)buff;
	//if (msg.protocol_version != 1)
	//	return;

	Peer* peer = GetPeer(msg->address_from);
	if (peer == NULL)
	{
		// Found a new peer
		Serial.print("New Peer: ");
		Serial.println(msg->address_from, DEC);
		Peer* p = new Peer(msg->address_from);
		peers.Add(p);
		OnNewPeer(p);
	} else {
		// Existing peer, reset timer
		peer->time = 0;
	}
}

void MeshBase::SendPeerDiscovery()
{
	last_broadcast_time = millis();
	MeshBase::PeerDiscoveryMessage payload;
	payload.protocol_version = 1;
	payload.network_capabilities = 0;
	payload.application_capabilities = 0;
	payload.num_peers = peers.length;
	payload.uptime = millis() / 1000;
	SendMessage(PEER_DISCOVERY, type_peer_discovery, &payload, sizeof(MeshBase::PeerDiscoveryMessage), true);
}

void MeshBase::SendMessage(uint32_t to, uint8_t type, const void* data, uint8_t length, bool is_broadcast)
{
	uint8_t buff[32];
	Message* msg = (struct Message*)buff;
	msg->protocol_version = 1;
	msg->ttl = 0;
	msg->type = type;
	msg->address_from = address;
	msg->split_enabled = 0;
	msg->split_part = 0;
	radio.stopListening();
	if (is_broadcast)
		radio.openWritingPipe(TO_BROADCAST(to));
	else
		radio.openWritingPipe(TO_ADDRESS(to));
	radio.write(data, length);
	radio.startListening();
}

void MeshBase::SendMessage(uint32_t to, uint8_t type, const void* data, uint8_t length)
{
	SendMessage(to, type_user, data, length, false);
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

MeshBase::Peer* MeshBase::GetPeer(uint32_t a)
{
	LinkedList<Peer>::Node* current = peers.first;
	while(current != NULL)
	{
		if (current->item->address == a)
			return current->item;
		current = current->next;
	}
	// Could not find..
	return NULL;
}

