#include "multiplayer/network/socketPlatform.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/select.h>
#endif

namespace {
	int toSocketLen(size_t size) {
		if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
			throw std::runtime_error("[SocketPlatform] Buffer too large for socket send/recv");
		}
		return static_cast<int>(size);
	}
} // namespace

namespace df::mp::net {
	SocketPlatform::SocketPlatform() {
#ifdef _WIN32
		WSADATA data{};
		int rc = WSAStartup(MAKEWORD(2, 2), &data);
		if (rc != 0) {
			throw std::runtime_error("[SocketPlatform] WSAStartup failed");
		}
#endif
	}

	SocketPlatform::~SocketPlatform() {
#ifdef _WIN32
		WSACleanup();
#endif
	}

	SocketHandle createTcpSocket() {
		return socket(AF_INET, SOCK_STREAM, 0);
	}

	bool isValid(SocketHandle socket) noexcept {
		return socket != INVALID_SOCKET_HANDLE;
	}

	bool setReuseAddr(SocketHandle socket, bool enabled) {
		int opt = enabled ? 1 : 0;
#ifdef _WIN32
		return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) != SOCKET_ERROR_CODE;
#else
		return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != SOCKET_ERROR_CODE;
#endif
	}

	bool bind(SocketHandle socket, const SocketAddress& address) {
		return ::bind(socket, reinterpret_cast<const sockaddr*>(&address.addr), sizeof(address.addr)) != SOCKET_ERROR_CODE;
	}

	bool listen(SocketHandle socket, int backlog) {
		return ::listen(socket, backlog) != SOCKET_ERROR_CODE;
	}

	bool setNonBlocking(SocketHandle socket, bool enabled) {
#ifdef _WIN32
		u_long mode = enabled ? 1 : 0;
		return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
		const int flags = fcntl(socket, F_GETFL, 0);
		if (flags < 0) {
			return false;
		}
		const int next = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		return fcntl(socket, F_SETFL, next) == 0;
#endif
	}

	bool connect(SocketHandle socket, const SocketAddress& address) {
		if (!setNonBlocking(socket, true)) {
			return false;
		}

		const int started = ::connect(socket, reinterpret_cast<const sockaddr*>(&address.addr), sizeof(address.addr));
		if (started == 0) {
			setNonBlocking(socket, false);
			return true;
		}

		const int err = lastError();
#ifdef _WIN32
		const bool inProgress = err == WSAEWOULDBLOCK;
#else
		const bool inProgress = err == EINPROGRESS;
#endif
		if (!inProgress) {
			setNonBlocking(socket, false);
			return false;
		}

		fd_set writeSet;
		FD_ZERO(&writeSet);
		FD_SET(socket, &writeSet);
		timeval wait{};
		wait.tv_sec = 5;
		const int ready = ::select(static_cast<int>(socket) + 1, nullptr, &writeSet, nullptr, &wait);
		int soError = 0;
		if (ready > 0) {
#ifdef _WIN32
			int length = sizeof(soError);
			getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &length);
#else
			socklen_t length = sizeof(soError);
			getsockopt(socket, SOL_SOCKET, SO_ERROR, &soError, &length);
#endif
		} else if (ready == 0) {
#ifdef _WIN32
			soError = WSAETIMEDOUT;
#else
			soError = ETIMEDOUT;
#endif
		} else {
			soError = lastError();
		}

		setNonBlocking(socket, false);
		if (soError != 0) {
#ifdef _WIN32
			WSASetLastError(soError);
#else
			errno = soError;
#endif
			return false;
		}
		return true;
	}

	SocketHandle accept(SocketHandle socket, SocketAddress& outAddress) {
#ifdef _WIN32
		int length = sizeof(outAddress.addr);
#else
		socklen_t length = sizeof(outAddress.addr);
#endif
		return ::accept(socket, reinterpret_cast<sockaddr*>(&outAddress.addr), &length);
	}

	int send(SocketHandle socket, const void* data, size_t size) {
		// A peer that already closed must not raise SIGPIPE, which would end the whole process.
#ifdef MSG_NOSIGNAL
		constexpr int flags = MSG_NOSIGNAL;
#else
		constexpr int flags = 0;
#endif
		return ::send(socket, static_cast<const char*>(data), toSocketLen(size), flags);
	}

	int recv(SocketHandle socket, void* data, size_t size) {
		return ::recv(socket, static_cast<char*>(data), toSocketLen(size), 0);
	}

	void shutdownBoth(SocketHandle socket) noexcept {
#ifdef _WIN32
		::shutdown(socket, SD_BOTH);
#else
		::shutdown(socket, SHUT_RDWR);
#endif
	}

	void close(SocketHandle socket) noexcept {
#ifdef _WIN32
		if (socket != INVALID_SOCKET) {
			closesocket(socket);
		}
#else
		if (socket >= 0) {
			::close(socket);
		}
#endif
	}

	SocketAddress makeIpv4Address(const std::string& ip, uint16_t port) {
		SocketAddress address{};
		address.addr.sin_family = AF_INET;
		address.addr.sin_port = htons(port);

		if (inet_pton(AF_INET, ip.c_str(), &address.addr.sin_addr) <= 0) {
			throw std::runtime_error("[SocketPlatform] Invalid IPv4 address: " + ip);
		}
		return address;
	}

	uint32_t getIpv4Address(const SocketAddress& address) noexcept {
		return address.addr.sin_addr.s_addr;
	}

	std::string ipv4ToString(uint32_t addr) {
		char buffer[INET_ADDRSTRLEN]{};
		inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
		return std::string(buffer);
	}

	int lastError() noexcept {
#ifdef _WIN32
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	bool isInterruptedError(int err) noexcept {
#ifdef _WIN32
		return err == WSAEINTR;
#else
		return err == EINTR;
#endif
	}

	std::string errorMessage(int err) {
#ifdef _WIN32
		return "WSA error " + std::to_string(err);
#else
		return std::string(strerror(err));
#endif
	}
} // namespace df::mp::net



