# boost_asio_TCP_server_thread-pool
/**
 * The main function of the TCP server application.
 *
 * This function initializes and runs a TCP server that listens for incoming client connections on port 12345.
 * It creates an IO context to manage asynchronous operations and an acceptor to handle client connections.
 * The server launches a pool of worker threads, each running the IO context, to handle client connections concurrently.
 *
 * The main loop of the server waits for incoming client connections and launches a separate thread for each connection
 * using the `handleClient` function. The server gracefully shuts down upon receiving a shutdown request by closing
 * the acceptor, stopping the IO context, and notifying worker threads to finish their work.
 *
 * @note The server prints a message upon startup, indicating the port number and the number of worker threads.
 *       It catches exceptions and prints error messages if any occur during execution.
 *
 * @return 0 on successful execution.
 */
