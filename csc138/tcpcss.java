import java.io.*;
import java.net.*;
import java.util.concurrent.ConcurrentHashMap;

public class tcpcss{
    private static final int SERVER_PORT = 12345;
    private static ConcurrentHashMap<Integer, ClientHandler> clientHandlers = new ConcurrentHashMap<>(); // keep track of active client connection
    private static boolean running = true;

    public static void main(String[] args) throws Exception{
        // Setup server socket with port: 12345
        ServerSocket serverSocket = new ServerSocket(SERVER_PORT);

        // Handle Ctrl-C
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("Shutdown signal received.");
            running = false;
        }));

        System.out.println("Server started, waiting for clients!");
        int clientId = 0;

        while(running){
            // Accept client connection
            Socket clientSocket = serverSocket.accept();
            
            // new ClientHaandler for incoming connection
            ClientHandler clientHandler = new ClientHandler(clientSocket, clientId);
            clientHandlers.put(clientId, clientHandler);
            
            // create new thread for clients
            Thread thread = new Thread(clientHandler, "Thread-" + clientId);
            thread.start();
            clientId++;
        }
    }

    // send message to all the clients that is currently connected
    public static void broadcastMessage(String message, int senderId){
        for (ClientHandler clientHandler : clientHandlers.values()){
            // this checks who sent the message, so it does not get broadcast to the sender.
            if(clientHandler.getClientId() != senderId && clientHandler.isActive()){
                clientHandler.sendMessage(message);
            }
        }
    }

    // remove client from the collection
    public static void removeClient(int clientId){
        clientHandlers.remove(clientId);
    }

}

class ClientHandler implements Runnable {

    private Socket clientSocket;            // client's socket
    private int clientId;                   // client's id
    private boolean active = true;          // checks if client is active
    private DataOutputStream outToClient;   // used to send messages to client

    // Constructor
    public ClientHandler(Socket socket, int clientId){
        this.clientSocket = socket;
        this.clientId = clientId;
    }

    // getter function to get clientId
    public int getClientId(){
        return clientId;
    }

    public boolean isActive(){
        return active;
    }

    // send message to client
    public void sendMessage(String message) {
        try {
            outToClient.writeBytes(message + "\n");
            outToClient.flush();
        } catch (Exception e) {
            e.printStackTrace();
            active = false;
        }
    }

    // run function
    @Override
    public void run() {
        // display the new connection with thread name, ip, and port number
        System.out.println( "New connection, thread name is " + Thread.currentThread().getName() +
                            " ip is: " + clientSocket.getInetAddress().getHostAddress() +
                            ", port: " + clientSocket.getPort());
        System.out.println("Adding to list of sockets as " + clientId);
        
        try {
            BufferedReader inFromClient = new BufferedReader(new InputStreamReader(clientSocket.getInputStream())); // Get userinput from clientSocket
            DataOutputStream outToClient = new DataOutputStream(clientSocket.getOutputStream());                    // A way to get data out to client
            
            this.outToClient = outToClient;
            String message;

            // print the client's messages on the server's terminal
            while((message = inFromClient.readLine()) != null){
                System.out.println("client: " + message);
                
                // send the messages to other client
                tcpcss.broadcastMessage(message, clientId);
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            // if Client disconnects, remove client
            tcpcss.removeClient(this.clientId);
            try {
                // make sure the socket gets closed
                clientSocket.close();
                System.out.println("Client disconnected.");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    

}