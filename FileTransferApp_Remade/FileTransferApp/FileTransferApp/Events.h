#ifndef EVENTS_H
#define EVENTS_H

namespace Events
{
	class Event
	{
	public:
		Event(void);
		virtual ~Event(void);
	};

	class NetworkEvent: public Event
	{
	public:
		enum EventHappened {Nothing, ConnGracefullyClosed, ConnBroken};

	private:
		EventHappened	m_EventHappened;

	public:
		NetworkEvent(EventHappened eventHappened): m_EventHappened(eventHappened) {}
		EventHappened WhatHappened() const {return m_EventHappened;}
	};

	class DialogEvent: public Event
	{
	};

	class MainDlgEvent: public DialogEvent
	{
	public:
		enum UIEvent {
		//the other computer has closed the connection (e.g. closed the application)
		OtherComputerClosedConn,
		//prepare the UI for a receiving operation (of a file or folder)
		InitReceiveItems,
		//prepare the UI for a receiving operation (of a file or folder)
		InitSendItems
		};

	private:
		UIEvent		m_uiEvent;

	public:
		MainDlgEvent(UIEvent uiEvent): m_uiEvent(uiEvent) {}
		UIEvent WhatHappened() const {return m_uiEvent;}
	};
}

#endif//EVENTS_H