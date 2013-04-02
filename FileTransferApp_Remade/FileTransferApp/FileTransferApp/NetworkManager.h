#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "General.h"
#include "NetworkObject.h"
#include "Events.h"
#include <vector>

class App;

//this class is a singletone and deals with the network connection and other general network stuff.
class NetworkManager: public NetworkObject
{
	//PRIVATE DATA
	vector<NetworkObject*>	m_netObjects;

	//PRIVATE FUNCTIONS
	NetworkManager(void): NetworkObject(0) {}
	~NetworkManager(void);

public:
	//sends the event to the NetworkManager. The NetworkManager will know what to do with it.
	void SendEvent(const Events::NetworkEvent& anEvent);

private:
	//adds the netObj to the manage list.
	//NOTW that NetworkObject is expected to have been allocated on the stack: NetworkManager will never "delete" netObj!
	void Manage(NetworkObject* netObj);
	void OnCloseNetwork();

	friend class App;
	friend class NetworkObject;
};

#endif//NETWORKMANAGER_H