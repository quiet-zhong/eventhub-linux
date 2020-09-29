#ifndef LIBSOCKET_LIBUNIXSOCKET_H_61CF2FC7034E4AD982DA08144D578572
#define LIBSOCKET_LIBUNIXSOCKET_H_61CF2FC7034E4AD982DA08144D578572

# include <sys/types.h>
# include <sys/socket.h>

# define LIBSOCKET_STREAM 1
# define LIBSOCKET_DGRAM  2

# define LIBSOCKET_READ  1
# define LIBSOCKET_WRITE 2

#ifdef __cplusplus
extern "C" {
#endif

int create_unix_stream_socket(const char* path, int flags);

int create_unix_dgram_socket(const char* bind_path, int flags);

int connect_unix_dgram_socket(int sfd, const char* path);

int destroy_unix_socket(int sfd);

int shutdown_unix_stream_socket(int sfd, int method);

int create_unix_server_socket(const char* path, int socktype, int flags);

int accept_unix_stream_socket(int sfd, int flags);

ssize_t recvfrom_unix_dgram_socket(int sfd, void* buf, size_t size, char* from, size_t from_size, int recvfrom_flags);

ssize_t sendto_unix_dgram_socket(int sfd, const void* buf, size_t size, const char* path, int sendto_flags);

# ifdef __cplusplus
}
# endif

# endif
