#include <iostream>
#include <thread>
#include <vector>
#include <nanodbc_exec/nanodbc.h>
#include <nanodbc_exec/mssql_functions.hpp>
#include <boost/asio.hpp>
#include <condition_variable>
#include <map>
using boost::asio::ip::tcp;
const std::string cls = "[2J[H[A";
const std::string Beep2 = "";
const std::string Beep3 = "";

std::atomic<bool> shutdownRequested(false);

/**
 * Monitors the shutdownRequested flag and gracefully shuts down the server when requested.
 *
 * @param ioContext The Boost.Asio IO context managing asynchronous operations.
 * @param acceptor The TCP acceptor listening for incoming connections.
 *
 * This function runs in a separate thread and periodically checks the shutdownRequested flag.
 * When the flag becomes true, it notifies all threads to finish their work and initiates a graceful
 * shutdown by closing the acceptor and stopping the IO context. This ensures that existing connections
 * are allowed to finish processing before terminating the server.
 */
void shutdownCheck(boost::asio::io_context& ioContext, tcp::acceptor& acceptor)
{
    while (!shutdownRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "Server stopping..client requested" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Close acceptor and stop ioContext
    std::cout << "acceptor.close();" << std::endl;
    acceptor.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "ioContext.stop();" << std::endl;
    ioContext.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return;
}

/**
 * Processes the received message and sends a response back to the client.
 *
 * @param clientSocket A reference to the TCP socket connected to the client.
 * @param bufferMsg A string reference to store the buffered message.
 * @param buffer An array of characters used to receive data from the client.
 * @param bytesRead The number of bytes read from the client.
 * @param clienVars A reference to a map storing various client-related information.
 *
 * This function handles the processing of the message received from the client. It takes the received message,
 * buffers it if it is not an empty line, and performs specific actions based on the content of the message.
 * The function checks for the Enter key, F1, F2, and F3 key sequences, and triggers corresponding actions.
 * It uses the provided client variables (clienVars) to maintain the client's state and manage communication.
 *
 * If the received message is an empty line (Enter key pressed), the function checks if the client's scanned badge
 * (`clienVars["reqScan_Badge"]`) is empty. If empty, it queries the database to match the scanned badge with
 * information in the database. If a match is found, it updates client variables with the user's information.
 * If no match is found, an error message is added to the buffer.
 *
 * The function also handles different menu states (`clienVars["currentMenu"]`) and performs actions accordingly.
 * It sends custom messages back to the client using the provided socket. Additionally, it triggers actions based
 * on specific key sequences, such as F1, F2, and F3.
 * !-- uses  thread-local storage contributes to thread safety in this context.
 * @note The function assumes the existence of a database connection string (`connStr`),
 *       the `mssql_select_vector` function, and the `cls` constant for clearing the screen.
 */
void processAndSend(tcp::socket &clientSocket, std::string &bufferMsg, const std::array<char, 1024> &buffer, std::size_t bytesRead, std::unordered_map<std::string, std::string> &clienVars)
{
    // Process the received message
    std::string receivedMessage(buffer.data(), bytesRead);

    // Buffer the received message
    if (receivedMessage != "\r\n")
        bufferMsg += receivedMessage;

    // Detect Enter key (New Line)
     // Handle actions when Enter key is pressed
    if (receivedMessage == "\r\n")
    {
        /*
        Request server stop
        In real world it is an RESTART
        Server will be an Windows Service ......
        Or an C# :
        
        using System;
        using System.Diagnostics;

        namespace run_once_WMS_batch_Csh
        {
            class Program
            {
                public static void Main(string[] args)
                {
                    Console.WriteLine("Starting...in ");
                    string execPath = AppDomain.CurrentDomain.BaseDirectory;
                    Console.WriteLine(execPath);

                        System.Threading.Thread.Sleep(5*1000);
                        Process pro_ini = new Process();
                        pro_ini.StartInfo.FileName = "WMS_batch.exe";
                        pro_ini.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
                        pro_ini.StartInfo.WorkingDirectory = execPath;
                        pro_ini.Start();
                        pro_ini.WaitForExit();


                    while (true) // condition
                    {

                        Process[] pname = Process.GetProcessesByName("WMS_batch");

                        if (pname.Length == 0)
                        {
                            Console.WriteLine("Start in ~10 s");

                            System.Threading.Thread.Sleep(10*1000);//10 s
                            Process pro = new Process();
                            pro.StartInfo.FileName = "WMS_batch.exe";
                            pro.StartInfo.WindowStyle = ProcessWindowStyle.Normal;
                            pro_ini.StartInfo.WorkingDirectory = execPath;
                            pro.Start();
                            pro.WaitForExit();
                        }
                        else
                        {
                            System.Threading.Thread.Sleep(10*1000); //10 s
                        }

                    }
                }
            }
        }

        */
        if(bufferMsg == "STOPALL"){
        shutdownRequested = true;
        clientSocket.close();
        return;

        }

       

        if (clienVars["reqScan_Badge"] == "")
        {
            // Check the database for the scanned badge
            auto userInfo = mssql_select_vector("SELECT  * FROM WHOISWHO WHERE SHORT_ID='" + bufferMsg + "'",false);

            if (userInfo.size() == 1)
            {
                // Update client variables with user information
                clienVars["reqScan_Badge"] = userInfo[0][1];
                clienVars["uName"] = userInfo[0][0];
                clienVars["currentMenu"] = "HOME";
            }
            else
            {
                // Add an error message to the buffer
                bufferMsg += cls + "Wrong Badge retry:\r\n";
            }
        }

        // Handle actions based on currentMenu state
        if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "HOME")
        {
            bufferMsg = cls + clienVars["uName"] + "\r\n1-Rec\r\n2-Imp\r\n";
        }

        if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "Recetion0" && receivedMessage.length() > 0)
        {
            // Handle actions for Recetion0 menu state
            clienVars["doc"] = bufferMsg;

            bufferMsg = cls + "Recetion\r\n:" + clienVars["doc"] + "\r\nScanML\r\n";
            clienVars["currentMenu"] = "Recetion1";
            receivedMessage = "";
        }

        if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "Recetion1" && receivedMessage.length() > 0)
        {
            // Handle actions for Recetion1 menu state
            std::string tbufferMsg = bufferMsg;
            bufferMsg = cls + "Recetion\r\n:" + clienVars["doc"] + "\r\n???\r\n";
            bufferMsg += tbufferMsg + "to CHECK";
        }

        if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "Imports" && receivedMessage.length() > 0)
        {
            clienVars["doc"] = bufferMsg;

            bufferMsg = cls + "ImportDoc:" + clienVars["doc"] + "\r\nScanSerial\r\n";
            clienVars["currentMenu"] = "Imports1";
            receivedMessage = "";
        }

        if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "Imports1" && receivedMessage.length() > 0)
        {
            std::string tbufferMsg = bufferMsg;
            bufferMsg = cls + "ImportDoc:" + clienVars["doc"] + "\r\n__\r\n";
            bufferMsg += tbufferMsg + "to CHECK";
        }

        // Send the custom message back to the client
        boost::asio::write(clientSocket, boost::asio::buffer(bufferMsg));
        bufferMsg.clear();
    }
    //! Detect Enter key (New Line) end

    // Triggers without Enter Key
    else if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "HOME" && receivedMessage == "1")
    {

        bufferMsg = cls + "Recetion\r\nScanSomething\r\n";
        // Send the custom message back to the client
        clienVars["currentMenu"] = "Recetion0";
        boost::asio::write(clientSocket, boost::asio::buffer(bufferMsg));
        bufferMsg.clear();
    }

    else if (clienVars["reqScan_Badge"] != "" && clienVars["currentMenu"] == "HOME" && receivedMessage == "2")
    {
        // Handle actions for F2 key pressed
        bufferMsg = cls + "Imports\r\nScanImportDoc\r\n";
        // Send the custom message back to the client
        clienVars["currentMenu"] = "Imports";
        boost::asio::write(clientSocket, boost::asio::buffer(bufferMsg));
        bufferMsg.clear();
    }

    // Detect F1, F2, and F3 keys   // Triggers without Enter Key
    else if (receivedMessage == "\x1BOP") // F1 key
    {

        std::cout << "F1 key detected!" << std::endl;

        // Add your logic to handle the F1 key here
    }
    else if (receivedMessage == "\x1BOQ") // F2 key
    {
        // Handle actions for F2 key pressed
        clienVars["currentMenu"] = "HOME";
        bufferMsg = cls + clienVars["uName"] + "\r\n1-Rec\r\n2-Imp\r\n";
        clienVars["currentMenu"] = "HOME";
        boost::asio::write(clientSocket, boost::asio::buffer(bufferMsg));
        bufferMsg.clear();
    }
    else if (receivedMessage == "\x1BOR") // F3 key
    {
        // Handle actions for F3 key pressed
        // Close the client socket to disconnect the client
        boost::system::error_code ec;
        clientSocket.close(ec);

        if (ec)
        {
            std::cerr << "Error disconnecting client: " << ec.message() << std::endl;
        }
    }
}

/**
 * Handles the communication with a client connected via a TCP socket.
 *
 * @param clientSocket The socket representing the client connection.
 *
 * This function is responsible for managing the interaction with a client connected to the server.
 * Upon connection, it sends a welcome message to the client. Subsequently, it enters a loop to continually
 * read data from the client, process and send responses. The communication involves checking for read errors,
 * processing and sending the received message using the `processAndSend` function.
 *
 * The loop continues until the client initiates a disconnection by pressing F3 or if a read error occurs.
 * In the event of a disconnection, the function terminates the connection. Any exceptions that occur during
 * the process are caught, and an error message is printed.
 * !-- uses  thread-local storage contributes to thread safety in this context.
 * Client-specific variables, such as `bufferMsg` and `clienVars`, are initialized to manage communication state.
 * `clienVars` is a map storing various client-related information, and `bufferMsg` serves as a buffer for received messages.
 *
 * @note The function assumes the existence of a database connection string (`connStr`) and the `processAndSend` function.
 */
void handleClient(tcp::socket clientSocket)
{
    // Client vars
    std::string bufferMsg; // Buffer for the received messages
    std::unordered_map<std::string, std::string> clienVars;

    clienVars["reqScan_Badge"] = "";
    clienVars["uName"] = "";
    clienVars["currentMenu"] = "HOME";
    clienVars["doc"] = "";
    clienVars["scaned"] = "";
    try
    {
        // Send the welcome message to the client
        std::string welcomeMessage = mssql_select("SELECT msg FROM TestWelcome", connStr);
        boost::asio::write(clientSocket, boost::asio::buffer(welcomeMessage));

        while (true)
        {
            std::array<char, 1024> buffer;
            boost::system::error_code error;

            // Read data from the client
            std::size_t bytesRead = clientSocket.read_some(boost::asio::buffer(buffer), error);

            // Check for read errors
            if (error == boost::asio::error::eof)
            {
                // Connection closed by the client
                break;
            }
            else if (error)
            {
                // Other read error occurred
                throw boost::system::system_error(error);
            }

            // Check if the socket is open before processing and sending the received message
            if (clientSocket.is_open())
            {
                // Process and send the received message
                processAndSend(clientSocket, bufferMsg, buffer, bytesRead, clienVars);

                // Check if the client requested to disconnect by pressing F3
                if (buffer[0] == 'F' && buffer[1] == '3')
                {
                    // // std::cout << "Client requested to disconnect. Exiting..." << std::endl;
                    break;
                }
            }
            else
            {
                //// std::cout << "Socket is not open. Exiting..." << std::endl;
                break;
            }
        }
    }
    catch (std::exception &e)
    {
        if (clientSocket.is_open())
        {
            std::cerr << "Exception in thread: " << e.what() << std::endl;
        }
    }
}

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

int main()
{
    try
    {
        // Create an IO context to manage asynchronous operations
        boost::asio::io_context ioContext;

        // Create an acceptor to listen for incoming connections on port 12345
        tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), 12345));

        // Create a thread pool to handle client connections
        std::vector<std::thread> threadPool;
        const std::size_t numThreads = std::thread::hardware_concurrency();

        // Create and launch worker threads in the thread pool
        for (std::size_t i = 0; i < numThreads; ++i)
        {
            // Each worker thread runs the IO context
            threadPool.emplace_back([&ioContext]()
            {
                ioContext.run();
            });
        }

        // Inform that the server has started and is listening on port 12345
        std::cout << "Server started. Listening on port 12345 with #" << numThreads << " threads" << std::endl;

        // Create a thread for handling shutdown
        std::thread shutdownThread(shutdownCheck, std::ref(ioContext), std::ref(acceptor));

        // Accept and handle client connections
        while (!shutdownRequested)
        {
            // Wait for a client to connect
            tcp::socket clientSocket(ioContext);
            acceptor.accept(clientSocket);

            // Handle the client connection in a separate thread
            std::thread clientThread(handleClient, std::move(clientSocket));
            clientThread.detach();
        }

        std::cout << "Server stoping..."<< std::endl;
        // Check if the acceptor is open before closing
        if (acceptor.is_open())
        {
            // Close acceptor
            acceptor.close();
        }

        // Check if the ioContext is running before stopping
        if (!ioContext.stopped())
        {
            // Stop ioContext
            ioContext.stop();
        }
        return 0;

    }
    catch (std::exception &e)
    {
        // Handle exceptions by printing an error message
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
