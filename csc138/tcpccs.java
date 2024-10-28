import java.io.*;
import java.net.*;

class tcpccs {
    private Socket clientSocket;                    // socket used to establish connection
    private DataOutputStream outToServer;           // output that goes to the server
    private BufferedReader inFromServer;            // input coming from server
    private BufferedReader reader;                  // used for user input
    private static String username;                 // name of the user
    private volatile boolean connected = true;      // used for shutdown handling
    private volatile boolean userExit = false;      // tracks if user used /quit command
    private static final int SERVER_PORT = 12345;   // Server's port number


    public static void main(String[] args) {
        // Check for arguments. Need server's hostname or ip. Also need a username.
        if(args.length < 2){
            System.out.println("Usage: java tcpccs.java <Server's Hostname> <Username>");
        }
        String hostname = args[0];
        username = args[1];

        new tcpccs(hostname, username);
    }

    // Establishing connection to server
    public tcpccs(String serverIP, String username){
        this.username = username;
        try {
            this.clientSocket = new Socket(serverIP, SERVER_PORT);
            this.outToServer = new DataOutputStream(clientSocket.getOutputStream());
            this.inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            this.reader = new BufferedReader(new InputStreamReader(System.in));
            System.out.println("Connected to the server. You can start sending messages.");

            // created a new thread to always listen messages coming from the server
            new Thread(() -> listenMessageFromServer()).start();

            // loop for sending messages to server. The server will handle the broadcast to other clients.
            sendMessageLoop();
        } catch (Exception e) {
            System.out.println("Error connecting to server: " + e.getMessage());
        }
    }

    // Listen for messages coming from the server
    private void listenMessageFromServer(){
        try {
            String messageFromServer;
            while(connected && (messageFromServer = inFromServer.readLine()) != null){
                System.out.println(messageFromServer);
            }
        } catch (IOException e) {
            if(!userExit){
                System.out.println("Server disconnected. Exiting...");
            }
            connected = false;
        }
    }

    // get user input to send messages
    private void sendMessageLoop(){
        try {
            String message;
            while (connected && (message = reader.readLine()) != null) {
                // Check if user types /quit
                // Will exit the chat server
                if (message.equalsIgnoreCase("/quit")){
                    System.out.println("Exiting chat...");
                    userExit = true;
                    connected = false;
                    break;
                }
                outToServer.writeBytes(username + ": " + message + "\n");
                outToServer.flush();
                
            }
        } catch (Exception e) {
            System.out.println("Error sending message: " + e.getMessage());
        } finally{
            cleanUp();
        }
    }

    // makes sure it closes the socket
    private void cleanUp() {
        try {
            if (outToServer != null) outToServer.close();
            if (inFromServer != null) inFromServer.close();
            if (reader != null) reader.close();
            if (clientSocket != null && !clientSocket.isClosed()) clientSocket.close();
            System.out.println("Connection closed.");
        } catch (IOException e) {
            System.out.println("Error closing resources: " + e.getMessage());
        }
    }
    
}