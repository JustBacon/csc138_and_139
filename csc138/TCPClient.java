import java.io.*;
import java.net.*;

class TCPClient {
    private static String hostname;
    private static int port;
    public static void main(String argv[]) throws Exception {
        String sentence;
        String modifiedSentence;

        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

        System.out.println("What is the hostname or ip address?");
        hostname = reader.readLine();
        System.out.println("What is the port number?");
        port = Integer.parseInt(reader.readLine());

        BufferedReader inFromUser = new BufferedReader(new InputStreamReader(System.in));
        Socket clientSocket = new Socket(hostname, port);

        DataOutputStream outToServer = new DataOutputStream(clientSocket.getOutputStream());

        BufferedReader inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
        
        sentence = inFromUser.readLine();
        outToServer.writeBytes(sentence + '\n');
        modifiedSentence = inFromServer.readLine();

        System.out.println("FROM SERVER: " + modifiedSentence);
        clientSocket.close();
    }
}