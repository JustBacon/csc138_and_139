import java.io.*;
import java.net.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import javax.net.ssl.*;
import java.security.KeyStore;

public class WebServer {
    public static void main(String[] args) {
        int httpPort = 80;
        int httpsPort = 443;

        String keystorePath = null;
        String keystorePassword = null;

        // check if there are arguments for keystore
        // only run http if arguments are not set
        if (args.length == 2) {
            keystorePath = args[0];
            keystorePassword = args[1];
        } else if (args.length != 0) {
            System.err.println("Error: Provide both keystore path and password or none");
            System.out.println("Starting in HTTP-only mode");
        }

        // run http
        new Thread(() -> startHttpServer(httpPort)).start();
        
        // run https if keystore arguments exist
        if (keystorePath != null && keystorePassword != null) {
            final String finalKeystorePath = keystorePath;
            final String finalKeystorePassword = keystorePassword;
            new Thread(() -> startHttpsServer(httpsPort, finalKeystorePath, finalKeystorePassword)).start();
        }

    }

    // start http server port 80
    private static void startHttpServer(int port) {
        // create server socket on port 80
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("HTTP Server running on port " + port);

            // wait for client connection
            while (true) {
                Socket clientSocket = serverSocket.accept();
                HttpRequestHandler requestHandler = new HttpRequestHandler(clientSocket);
                new Thread(requestHandler).start();
            }
        } catch (IOException e) {
            System.err.println("Server exception: " + e.getMessage());
        }
    }

    // start https server on port 443
    // use SSL/TLS certificate to open https port
    private static void startHttpsServer(int port, String keystorePath, String keystorePassword) {
        try {
            // this point to the keystore file that contains the SSL/TLS certificate
            System.setProperty("javax.net.ssl.keyStore", keystorePath);
            System.setProperty("javax.net.ssl.keyStorePassword", keystorePassword);

            // keystore.jks is loaded using FileInputStream
            // password is needed to access the keystore
            KeyStore keyStore = KeyStore.getInstance("JKS");
            try (FileInputStream keyStoreStream = new FileInputStream(keystorePath)) {
                keyStore.load(keyStoreStream, keystorePassword.toCharArray());
            }

            // KeyManagerFactory is used to manage the keys from the keystore
            // the init method initializes key manager
            // allows key manager to access the server's private key and certificate to establish secure connection
            KeyManagerFactory keyManagerFactory = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
            keyManagerFactory.init(keyStore, keystorePassword.toCharArray());

            // SSLContext is used to create secure sockets
            // TLS protocol
            SSLContext sslContext = SSLContext.getInstance("TLS");
            sslContext.init(keyManagerFactory.getKeyManagers(), null, null);

            // This factory is used to create SSL server sockets
            SSLServerSocketFactory sslServerSocketFactory = sslContext.getServerSocketFactory();
            SSLServerSocket sslServerSocket = (SSLServerSocket) sslServerSocketFactory.createServerSocket(port);
            System.out.println("HTTPS Server running on port " + port);

            // wait for client connection
            while (true) {
                Socket clienSocket = sslServerSocket.accept();
                HttpRequestHandler requestHandler = new HttpRequestHandler(clienSocket);
                new Thread(requestHandler).start();
            }

        } catch (Exception e) {
            System.err.println("HTTPS Server exception: " + e.getMessage());
        }
    }
}

// handles a single instance variable
// connection between the server and client
class HttpRequestHandler implements Runnable {
    private Socket socket;

    public HttpRequestHandler(Socket socket) {
        this.socket = socket;
    }


    @Override
    public void run() {
        // InputStream read data from client
        // BufferedOutputStream and DataOutputStream write data to client
        // BufferedReader read text data from input stream
        try (InputStream is = socket.getInputStream();
            BufferedOutputStream bos = new BufferedOutputStream(socket.getOutputStream());
            DataOutputStream os = new DataOutputStream(bos);
            BufferedReader br = new BufferedReader(new InputStreamReader(is))) {

            // get the first line of HTTP request
            // contain information like GET and the requested resource like /index.html
            String requestLine = br.readLine();
            System.out.println("Received request: " + requestLine);

            // if request line is empty or null skip
            if (requestLine == null || requestLine.isEmpty()) {
                return;
            }
            
            // if request line does NOT start with GET
            // send a response saying it is not allowed. send 405 method not allowed
            if (!requestLine.startsWith("GET")) {
                sendMethodNotAllowedResponse(os);
                return;
            }

            // the request line is spliit into tokens based on spaces
            // 2nd token is the path to the requested resource
            String[] tokens = requestLine.split(" ");
            String fileName = tokens[1].equals("/") ? "/index.html" : tokens[1];
            
            // handle favicon.ico
            if (fileName.equals("/favicon.ico")) {
                Path faviconPath = Paths.get("./favicon.ico");
                if (Files.exists(faviconPath)) {
                    sendFaviconResponse(os, faviconPath);
                } else {
                    sendNotFoundResponse(os);
                }
                return;
            }
            
            // Serve other files
            Path filePath = Paths.get("." + fileName);

            if (Files.exists(filePath) && !Files.isDirectory(filePath)) {
                sendFileResponse(os, filePath);
            } else {
                sendNotFoundResponse(os);
            }
        } catch (SocketException e) {
            System.err.println("Connection reset by client: " + e.getMessage());
        } catch (IOException e) {
            System.err.println("Request handling exception: " + e.getMessage());
        } finally {
            try {
                socket.close();
            } catch (IOException e) {
                System.err.println("Socket closing exception: " + e.getMessage());
            }
        }
    }

    // Send the requested file to client
    private void sendFileResponse(DataOutputStream os, Path filePath) throws IOException {
        String contentType = Files.probeContentType(filePath);
        byte[] fileContent = Files.readAllBytes(filePath);

        os.writeBytes("HTTP/1.1 200 OK\r\n");
        os.writeBytes("Content-Type: " + (contentType != null ? contentType : "application/octet-stream") + "\r\n");
        os.writeBytes("Content-Length: " + fileContent.length + "\r\n");
        os.writeBytes("\r\n");
        os.write(fileContent);
        os.flush();
    }

    // Send 404 not found HTML response
    private void sendNotFoundResponse(DataOutputStream os) throws IOException {
        String response = "<html><body><h1>404 Not Found</h1></body></html>";
        os.writeBytes("HTTP/1.1 404 Not Found\r\n");
        os.writeBytes("Content-Type: text/html\r\n");
        os.writeBytes("Content-Length: " + response.length() + "\r\n");
        os.writeBytes("\r\n");
        os.writeBytes(response);
        os.flush();
    }

    // Send 405 Method not allowed HTML response
    private void sendMethodNotAllowedResponse(DataOutputStream os) throws IOException {
        String response = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        os.writeBytes("HTTP/1.1 405 Method Not Allowed\r\n");
        os.writeBytes("Content-Type: text/html\r\n");
        os.writeBytes("Content-Length: " + response.length() + "\r\n");
        os.writeBytes("\r\n");
        os.writeBytes(response);
        os.flush();
    }

    // Send favicon.ico file if requested by the client
    private void sendFaviconResponse(DataOutputStream os,Path faviconPath) throws IOException {
        byte[] fileContent = Files.readAllBytes(faviconPath);

        os.writeBytes("HTTP/1.1 200 OK\r\n");
        os.writeBytes("Content-Type: image/x-icon\r\n");
        os.writeBytes("Content-Length: " + fileContent.length + "\r\n");
        os.writeBytes("\r\n");
        os.write(fileContent);
        os.flush();
    }
}
