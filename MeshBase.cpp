#include <Arduino.h>
#include <RF24.h>
#include "MeshBase.h"

#define MAX_PACKET_SIZE 32
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - sizeof(MeshBase::MessageHeader))

// -- Broadcast addresses --
#define PEER_DISCOVERY 1

// -- Helpers --
#define TO_BROADCAST(x) (0xBB00000000LL + x)
#define TO_ADDRESS(x) (0xAA00000000LL + x)

#define PEER_DISCOVERY_TIME 4000
#define CHECK_TIME 4000
#define PEER_TIMEOUT 3
#define ASSEMBLY_TIMEOUT 2

MeshBase::MeshBase(uint8_t ce, uint8_t cs)
: radio(ce, cs)
, address(0)
, last_broadcast_time(0)
, last_check_time(0)
, application_capabilities(0)
{}

void MeshBase::Begin()
{
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setRetries(4,2);
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
	if (radio.available())
	{
		bool done = false;
		do {
			uint8_t len = radio.getDynamicPayloadSize();
			byte buff[MAX_PAYLOAD_SIZE];
			done = radio.read(buff, len);
			HandlePacket(buff, len);
		} while (!done);
	}
	
	// Do periodic checks
	if (millis() - last_check_time > CHECK_TIME)
	{
		// Check for expired peers
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
		}

		// Check for expired packets
		{
			Message* current = assembly_list.first;
			while(current != NULL)
			{
				current->age += 1;
				if (current->age >= ASSEMBLY_TIMEOUT)
				{
					Serial.print("Dropped partial message. address=");
					Serial.print(current->header.address_from, DEC);
					Serial.print(" msg_id=");
					Serial.print(current->header.msg_id);
					Serial.print(" blocks_recieved=");
					Serial.println(current->blocks_recieved);
					current = assembly_list.Remove(current);
				} else {
					current = current->next;
				}
			}
		}
		last_check_time = millis();
	}
}

bool FindStream(const MeshBase::Message* current, const MeshBase::MessageHeader* find)
{
	if (current->header.address_from != find->address_from)
		return false;
	if (current->header.msg_id != find->msg_id)
		return false;
	return true;
}

void MeshBase::Message::AddPart(const void* payload, uint8_t len, uint8_t part_num, bool more_parts)
{
	uint8_t start_pos = part_num * MAX_PAYLOAD_SIZE;
	uint8_t end_pos = len + (part_num * MAX_PAYLOAD_SIZE);
	if (data == NULL)
		data = malloc(end_pos);
	if (end_pos > data_used)
		data = realloc(data, end_pos);
	memcpy(&static_cast<byte*>(data)[start_pos], payload, len);
	if (end_pos > data_used)
		data_used = end_pos;
	blocks_recieved += 1;
	if (!more_parts) {
		header.split_more = false;
		header.split_part = part_num;
	}
	age = 0;
}

bool MeshBase::Message::IsDone() const
{
	// We set the split_more to false if we recieved the last packet
	// in the stream, and split_part to total number of blocks in the stream.
	// So if split_more is false, and we have the right number of blocks_recieved
	// we are good to go.
	if (!header.split_more && blocks_recieved > header.split_part) {
		return true;
	}
	return false;
}

MeshBase::Message::~Message() {
	free(data);
}

void MeshBase::HandlePacket(const byte* data, uint8_t len)
{
	if (len < sizeof(MessageHeader))
		return;
	const MeshBase::MessageHeader* header = (struct MeshBase::MessageHeader*)data;
	uint8_t payload_length = len - sizeof(MessageHeader);
	const byte* payload = data + sizeof(MessageHeader);
	if (header->protocol_version != 1)
		return;

	Message* s = assembly_list.Find<const MessageHeader*>(header, &FindStream);
	if (s == NULL) {
		s = new Message(*header);
		assembly_list.Add(s);
	}
	s->AddPart(payload, payload_length, header->split_part, header->split_more);
	if (s->IsDone()) {
		switch(header->type) {
			case type_peer_discovery:
				HandlePeerDiscovery(&(s->header), s->data, s->data_used);
			break;
			default:
				OnMessage(&(s->header), s->data, s->data_used);
			break;
		}
		assembly_list.Remove(s);
	}
}

void MeshBase::HandlePeerDiscovery(const MeshBase::MessageHeader* msg, const void* buff, uint8_t length)
{
	if (length != sizeof(PeerDiscoveryMessage))
		return;
	const PeerDiscoveryMessage* pd = (struct PeerDiscoveryMessage*)buff;
	if (pd->protocol_version != 1)
		return;

	Peer* peer = GetPeer(msg->address_from);
	if (peer == NULL)
	{
		// Found a new peer
		Serial.print("New Peer. Address=");
		Serial.print(msg->address_from, DEC);
		Serial.print(" uptime=");
		Serial.print(pd->uptime, DEC);
		Serial.print(" num_peers=");
		Serial.println(pd->num_peers, DEC);
		Peer* p = new Peer(msg->address_from);
		p->Update(pd);
		peers.Add(p);
		OnNewPeer(p);
	} else {
		// Existing peer, reset timer
		peer->Update(pd);
	}
}

void MeshBase::SendPeerDiscovery()
{
	last_broadcast_time = millis();
	MeshBase::PeerDiscoveryMessage payload;
	payload.protocol_version = 1;
	payload.network_capabilities = 0;
	payload.application_capabilities = application_capabilities;
	payload.num_peers = peers.length;
	payload.uptime = millis() / 1000;
	SendMessage(PEER_DISCOVERY, type_peer_discovery, &payload, sizeof(payload), true);
}

void MeshBase::SendMessage(uint32_t to, uint8_t type, const void* data, uint8_t length, bool is_broadcast)
{
	static uint8_t current_msg_id = 0;
	byte buff[MAX_PACKET_SIZE];
	MessageHeader* msg = (struct MessageHeader*)buff;
	msg->protocol_version = 1;
	msg->ttl = 0;
	msg->type = type;
	msg->address_from = address;
	msg->msg_id = current_msg_id++;

	uint8_t num_pkts = (length / MAX_PAYLOAD_SIZE) + 1;
	for (uint8_t num = 0; num < num_pkts; ++num)
	{
		uint8_t remaining_length = length - (num * MAX_PAYLOAD_SIZE);
		msg->split_part = num;
		msg->split_more = remaining_length > MAX_PAYLOAD_SIZE;
		memcpy(buff + sizeof(MessageHeader), (const byte*)data + (num * MAX_PAYLOAD_SIZE), min(remaining_length, MAX_PAYLOAD_SIZE));
		uint8_t wire_size = min(remaining_length + sizeof(MessageHeader), MAX_PACKET_SIZE);

		radio.stopListening();
		bool result = true;
		if (is_broadcast)
			radio.openWritingPipe(TO_BROADCAST(to));
		else
			radio.openWritingPipe(TO_ADDRESS(to));
		if (is_broadcast) {
			//radio.startWrite(buff, wire_size);
			result = radio.write(buff, wire_size);
		} else {
			result = radio.write(buff, wire_size);
			if (result == false) {
				// Issue transmitting packet, retry?
				radio.startListening();
				delay(100);
				radio.stopListening();
				result = radio.write(buff, wire_size);
			}
			Serial.print(" T Sending pkt split_part=");
			Serial.print(msg->split_part);
			Serial.print(" id=");
			Serial.print(msg->msg_id);
			Serial.print(" result=");
			Serial.println(result);
		}
		radio.startListening();
	}
}

void MeshBase::SendMessage(uint32_t to, uint8_t type, const void* data, uint8_t length)
{
	SendMessage(to, type, data, length, false);
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

void MeshBase::Peer::Update(const PeerDiscoveryMessage* msg)
{
	application_capabilities = msg->application_capabilities;
	time = 0;
}

