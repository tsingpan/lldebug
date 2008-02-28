/*
 * Copyright (c) 2005-2008  cielacanth <cielacanth AT s60.xrea.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "precomp.h"
#include "net/echostream.h"

#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>

namespace lldebug {
namespace net {

using namespace boost::asio::ip;

echo_thread echo_thread::ms_thread;

struct echo_thread::request {
	shared_ptr<tcp::socket> socket;
	std::vector<char> buffer;
};

echo_thread::echo_thread()
	: m_is_exit_thread(false) {
}

echo_thread::~echo_thread() {
	exit_thread();

	// Finalize the thread.
	if (m_thread != NULL) {
		m_thread->join();
		m_thread.reset();
	}
}

void echo_thread::add_request(shared_ptr<tcp::socket> sock,
							  const std::vector<char> &buffer) {
	shared_ptr<request> req(new request);
	req->socket = sock;
	req->buffer = buffer;
	req->buffer.push_back('\r');
	req->buffer.push_back('\n');

	add_request(req);
}

/// Stop the thread.
void echo_thread::exit_thread() {
	scoped_lock lock(m_mutex);
	
	if (m_current != NULL) {
		m_current->socket->close();
	}
	m_is_exit_thread = true;
	m_cond.notify_all();
}

/// Add a request to the queue.
void echo_thread::add_request(shared_ptr<request> req) {
	scoped_lock lock(m_mutex);

	// Start thread, if any.
	if (m_thread == NULL) {
		shared_ptr<boost::thread> th(new boost::thread(
			boost::bind(&echo_thread::thread_main, this)));
		m_thread = th;
	}

	// Erase the old request, if any.
	while (m_request_queue.size() > 50) {
		m_request_queue.pop();
	}

	m_request_queue.push(req);
	m_cond.notify_all();
}

/// Get a request from the queue.
int echo_thread::get_request() {
	scoped_lock lock(m_mutex);

	m_current.reset();
	if (m_is_exit_thread) {
		return -1;
	}
	
	while (m_request_queue.empty()) {
		if (m_is_exit_thread) {
			return -1;
		}

		m_cond.wait(lock);
	}

	m_current = m_request_queue.front();
	m_request_queue.pop();
	return 0;
}

/// Thread function.
void echo_thread::thread_main() {
	for (;;) {
		if (get_request() != 0) {
			break;
		}

		// Process this request.
		try {
			size_t size = 0;
			while (size < m_current->buffer.size()) {
				size += m_current->socket->write_some(
					boost::asio::buffer(&m_current->buffer[size], m_current->buffer.size() - size));
			}

			size = 0;
			while (size < m_current->buffer.size()) {
				size += m_current->socket->read_some(
					boost::asio::buffer(&m_current->buffer[size], m_current->buffer.size() - size));
			}
		}
		catch (...) {
			/* ignore */
		}
	}
}

} // end of namespace net
} // end of namespace lldebug
