# Multi-Client Chat Server 
### With Epoll and Pthreads

----

## 📝 Overview:
This project is a multi-client chat server built in C that demonstrates advanced Linux system programming concepts. It uses POSIX sockets for networking, epoll for efficient non-blocking I/O, and pthreads for handling multiple clients concurrently. The server supports custom client naming, message broadcasting, logging of events, and periodic monitoring of active client connections. Signal handling is implemented for graceful shutdown.

----

## ✨ Features:
* Multi-client support using epoll for scalable I/O multiplexing.
* Each client connection is handled by a dedicated thread (pthreads).
* Clients send their username upon connection, and messages are broadcast with the sender’s name as a prefix.
* Logging of server events, messages, and errors to a log file (chat.log).
* Periodic monitoring of active client count.
* Graceful shutdown using SIGINT and SIGTERM signal handlers.

----

## ⚙️ Technologies:
* C (ANSI C)
* POSIX Sockets and epoll
* Pthreads for multithreading
* Linux System Programming
* Makefile for build automation

----

## 🚀 Usage:
#### Build the project:
```sh
Run “make” in the project directory.
```
#### Start the server:
```sh
Execute “./server” in a terminal.
```
#### Start one or more clients:
```sh
Execute “./client” in separate terminal windows.
```
* On client startup, the user is prompted for a username. After entering the username, clients can send messages which will be broadcast to all connected clients with the sender’s username prefixed.

#### To stop the server
* press Ctrl+C. The server handles this signal to gracefully close sockets and clean up resources.

----

## 📂 Files:
* server.c – Contains the main server code (socket setup, epoll management, client handling with threads, logging, and signal handling).
* client.c – Contains the client code to connect to the server, send a username, and exchange messages.
* logger.c/h – Module for logging server events to “chat.log”.
* monitor.c/h – Module for monitoring active client connections.
* Makefile – Build configuration file for compiling and linking the project.
* README.md – This file.
----

## 👤 Author:
* https://github.com/DevanshuS83/
