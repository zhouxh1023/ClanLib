/*
**  ClanLib SDK
**  Copyright (c) 1997-2015 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/

#include "Core/precomp.h"
#include "API/Core/System/event.h"
#include "API/Core/System/event_provider.h"
#include "API/Core/System/exception.h"
#include "event_impl.h"
#ifdef WIN32
#include "Win32/event_provider_win32.h"
#else
#include "Unix/event_provider_socketpair.h"
#include <errno.h>
#include <stdlib.h>
#endif

#ifndef WIN32
#include <sys/time.h>
#endif

namespace clan
{

/////////////////////////////////////////////////////////////////////////////
// Event Construction:

#ifdef WIN32
Event::Event(bool manual_reset, bool initial_state)
: impl(std::make_shared<Event_Impl>(new EventProvider_Win32(manual_reset, initial_state)))
{
}
#else
Event::Event(bool manual_reset, bool initial_state)
: impl(std::make_shared<Event_Impl>(new EventProvider_Socketpair(manual_reset, initial_state)))
{
}
#endif

Event::Event(EventProvider *event_provider)
: impl(std::make_shared<Event_Impl>(event_provider))
{
}

Event::~Event()
{
}

/////////////////////////////////////////////////////////////////////////////
// Event Attributes:

EventProvider *Event::get_event_provider() const
{
	return impl->provider;
}

/////////////////////////////////////////////////////////////////////////////
// Event Operations:

bool Event::wait(int timeout)
{
	Event *event = this;
	return wait(1, &event, timeout) == 0;
}

int Event::wait(int count, Event const * const * events, int timeout)
{
#ifdef WIN32
	DWORD timeout_win32 = (timeout == -1) ? INFINITE : timeout;

	int index_events;
	int num_events = 0;
	for (index_events = 0; index_events < count; index_events++)
	{
		bool flagged = events[index_events]->impl->provider->check_before_wait();
		if (flagged)
			return index_events;
		num_events += events[index_events]->impl->provider->get_num_event_handles();
	}

	if (num_events == 0)
	{
		WaitForMultipleObjects(0, 0, FALSE, timeout_win32);
		return -1;
	}
	else if (num_events == 1 && count == 1)
	{
		HANDLE handle = events[0]->impl->provider->get_event_handle(0);
		while (true)
		{
			DWORD result = WaitForSingleObject(handle, timeout_win32);
			if (result == WAIT_TIMEOUT)
				break;

			bool flagged = events[0]->impl->provider->check_after_wait(0);
			if (flagged)
				return 0;
		}
		return -1;
	}
	else
	{
		std::vector<HANDLE> handles;
		for (index_events = 0; index_events < count; index_events++)
		{
			int num_handles = events[index_events]->impl->provider->get_num_event_handles();
			for (int i=0; i<num_handles; i++)
				handles.push_back(events[index_events]->impl->provider->get_event_handle(i));
		}
		while (true)
		{
			DWORD result = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout_win32);
			DWORD index = 0;
			if (result == WAIT_TIMEOUT)
				break;
			else if (result == WAIT_FAILED)
				throw Exception("WaitForMultipleObjects failed");
			else if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + num_events)
				index = result - WAIT_OBJECT_0;
			else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + num_events)
				index = result - WAIT_ABANDONED_0;
			else
				continue;

			for (index_events = 0; index_events < count; index_events++)
			{
				int num_handles = events[index_events]->impl->provider->get_num_event_handles();
				if (index < (DWORD) num_handles)
				{
					bool flagged = events[index_events]->impl->provider->check_after_wait((int) index);
					if (flagged)
						return index_events;
					break;
				}
				index -= (DWORD) num_handles;
			}
		}

		return -1;
	}
#else
	int index_events;
	for (index_events = 0; index_events < count; index_events++)
	{
		EventProvider *provider = events[index_events]->impl->provider;
		if (provider == nullptr)
			throw Exception("Event's EventProvider is a null pointer!");
		bool flagged = provider->check_before_wait();
		if (flagged)
			return index_events;
	}

	// Placing the timeval struct here allows linux systems to more
	// correctly resume a select if it was awaken by a complex event.
	// On non-linux unixes (those that do not update timeval), the
	// timeout precision will be more inaccurate for manual reset
	// events with multiple listeners.
	//   -- mbn 4 nov 2004
	timeval tv;
	if (timeout == -1)
	{
		// Just set the timeval to zero.  Not really needed since we don't pass the field to select in this case.
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
	else
	{
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
	}

	while (true)
	{
		bool reads = false;
		bool writes = false;
		bool exceptions = false;
		int highest_fd = -1;
		fd_set rfds, wfds, efds;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		for (index_events = 0; index_events < count; index_events++)
		{
			EventProvider *provider = events[index_events]->impl->provider;
			if (provider == nullptr)
				throw Exception("Event's EventProvider is a null pointer!");
			int num_handles = provider->get_num_event_handles();
			for (int i=0; i<num_handles; i++)
			{
				int handle = provider->get_event_handle(i);
				switch (provider->get_event_type(i))
				{
				case EventProvider::type_fd_read:
					FD_SET(handle, &rfds);
					if (handle > highest_fd)
						highest_fd = handle;
					reads = true;
					break;
				case EventProvider::type_fd_write:
					FD_SET(handle, &wfds);
					if (handle > highest_fd)
						highest_fd = handle;
					writes = true;
					break;
				case EventProvider::type_fd_exception:
					FD_SET(handle, &efds);
					if (handle > highest_fd)
						highest_fd = handle;
					exceptions = true;
					break;
				}
			}
		}

		int result;
		do
		{
			result = select(highest_fd+1,
					reads ? &rfds : nullptr,
					writes ? &wfds : nullptr,
					exceptions ? &efds : nullptr,
					(timeout == -1) ? nullptr : &tv);
		} while (result == -1 && errno == EINTR); // The syscall was interrupted.  Try again.
		
		if (result == -1) // Error occoured
		{
			throw Exception(std::string("Event wait failed! Unix Error: ") + strerror(errno));
		}
		else if (result == 0) // Timed out
		{
			return -1;
		}
		else // Got a message
		{
			// find the flagged sockets
			for (index_events = 0; index_events < count; index_events++)
			{
				EventProvider *provider = events[index_events]->impl->provider;
				if (provider == nullptr)
					throw Exception("Event's EventProvider is a null pointer!");
				int num_handles = provider->get_num_event_handles();
				for (int i=0; i<num_handles; i++)
				{
					int handle = provider->get_event_handle(i);
					bool flagged = false;
					switch (provider->get_event_type(i))
					{
					case EventProvider::type_fd_read:
						if (FD_ISSET(handle, &rfds))
							flagged = true;
						break;
					case EventProvider::type_fd_write:
						if (FD_ISSET(handle, &wfds))
							flagged = true;
						break;
					case EventProvider::type_fd_exception:
						if (FD_ISSET(handle, &efds))
							flagged = true;
						break;
					}

					if (flagged)
					{
						flagged = provider->check_after_wait(i);
						if (flagged)
							return index_events;
					}
				}
			}
		}
	}

	return -1;
#endif
}

int Event::wait(const std::vector<Event *> &events, int timeout)
{
	if (events.size() > 0)
		return wait((int) events.size(), &events[0], timeout);
	else
		return wait(0, nullptr, timeout);
}

int Event::wait(const std::vector<Event> &events, int timeout)
{
	std::vector<Event>::size_type i, size;
	size = events.size();
	std::vector<Event *> event_ptrs;
	event_ptrs.reserve(size);
	for (i = 0; i < size; i++)
		event_ptrs.push_back((Event *) &events[i]);
	return wait(event_ptrs, timeout);
}

int Event::wait(Event &event1, int timeout)
{
	return event1.wait(timeout) ? 0 : -1;
}

int Event::wait(Event &event1, Event &event2, int timeout)
{
	Event *events[2] = { &event1, &event2 };
	return wait(2, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, int timeout)
{
	Event *events[3] = { &event1, &event2, &event3 };
	return wait(3, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, Event &event4, int timeout)
{
	Event *events[4] = { &event1, &event2, &event3, &event4 };
	return wait(4, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, Event &event4, Event &event5, int timeout)
{
	Event *events[5] = { &event1, &event2, &event3, &event4, &event5 };
	return wait(5, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, Event &event4, Event &event5, Event &event6, int timeout)
{
	Event *events[6] = { &event1, &event2, &event3, &event4, &event5, &event6 };
	return wait(6, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, Event &event4, Event &event5, Event &event6, Event &event7, int timeout)
{
	Event *events[7] = { &event1, &event2, &event3, &event4, &event5, &event6, &event7 };
	return wait(7, events, timeout);
}

int Event::wait(Event &event1, Event &event2, Event &event3, Event &event4, Event &event5, Event &event6, Event &event7, Event &event8, int timeout)
{
	Event *events[8] = { &event1, &event2, &event3, &event4, &event5, &event6, &event7, &event8 };
	return wait(8, events, timeout);
}

void Event::set()
{
	bool result = impl->provider->set();
	if (result == false)
		throw Exception("Unable to set event to signalled state!");
}

void Event::reset()
{
	bool result = impl->provider->reset();
	if (result == false)
		throw Exception("Unable to reset event!");
}

/////////////////////////////////////////////////////////////////////////////
// Event Implementation:

}
