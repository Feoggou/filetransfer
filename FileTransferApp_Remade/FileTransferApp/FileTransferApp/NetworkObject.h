#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

class NetworkManger;

//used only for the abort job: all network objects will need this data member and this function/command.
class NetworkObject
{
	bool		m_bAbortJob;
public:
	NetworkObject(NetworkManager* pManager):m_bAbortJob(false) {}
	~NetworkObject(void);

	void AbortJob() {m_bAbortJob = true;}
	bool AbortingJob() {return m_bAbortJob;}
};

#endif//NETWORKOBJECT_H