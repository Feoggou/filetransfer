#include "NetworkManager.h"
#include "App.h"

//
//NetworkManager::NetworkManager(void)
//{
//}


NetworkManager::~NetworkManager(void)
{
}


void NetworkManager::SendEvent(const Events::NetworkEvent& anEvent)
{
	theApp.OnNetworkEvent(anEvent);
}

void NetworkManager::Manage(NetworkObject* netObj)
{
	m_netObjects.push_back(netObj);
}

void NetworkManager::OnCloseNetwork()
{
	typedef vector<NetworkObject*>::iterator Iter;
	for (Iter I = m_netObjects.begin(); I != m_netObjects.end(); I++)
	{
		(*I)->AbortJob();
	}
}