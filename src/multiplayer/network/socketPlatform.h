#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace df::mp::net {
#ifdef _WIN32
	using SocketHandle = SOCKET;
	inline constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
	inline constexpr int SOCKET_ERROR_CODE = SOCKET_ERROR;
#else
	using SocketHandle = int;
	inline constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
	inline constexpr int SOCKET_ERROR_CODE = -1;
#endif

	class SocketPlatform {
	  public:
		SocketPlatform();
		~SocketPlatform();

		SocketPlatform(const SocketPlatform&) = delete;
		SocketPlatform& operator=(const SocketPlatform&) = delete;
	};

	struct SocketAddress {
		sockaddr_in addr{};
	};

	SocketHandle createTcpSocket();
	bool isValid(SocketHandle socket) noexcept;

	bool setReuseAddr(SocketHandle socket, bool enabled);
	bool bind(SocketHandle socket, const SocketAddress& address);
	bool listen(SocketHandle socket, int backlog);
	bool connect(SocketHandle socket, const SocketAddress& address);
	SocketHandle accept(SocketHandle socket, SocketAddress& outAddress);

	int send(SocketHandle socket, const void* data, size_t size);
	int recv(SocketHandle socket, void* data, size_t size);

	void shutdownBoth(SocketHandle socket) noexcept;
	void close(SocketHandle socket) noexcept;

	SocketAddress makeIpv4Address(const std::string& ip, uint16_t port);
	uint32_t getIpv4Address(const SocketAddress& address) noexcept;
	std::string ipv4ToString(uint32_t addr);

	int lastError() noexcept;
	bool isInterruptedError(int err) noexcept;
	std::string errorMessage(int err);
} // namespace df::mp::net

