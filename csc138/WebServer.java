import java.io.*;
import java.net.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class WebServer {
    public static void main(String[] args) {
        int port = 6789;

        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("HTTP Server running on port " + port);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                HttpRequestHandler requestHandler = new HttpRequestHandler(clientSocket);
                new Thread(requestHandler).start();
            }
        } catch (IOException e) {
            System.err.println("Server exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
}

class HttpRequestHandler implements Runnable {
    private Socket socket;

    public HttpRequestHandler(Socket socket) {
        this.socket = socket;
    }

    @Override
    public void run() {
        try (InputStream is = socket.getInputStream();
                BufferedOutputStream bos = new BufferedOutputStream(socket.getOutputStream());
                DataOutputStream os = new DataOutputStream(bos);
                BufferedReader br = new BufferedReader(new InputStreamReader(is))) {

            String requestLine = br.readLine();
            System.out.println("Received request: " + requestLine);

            if (requestLine == null || requestLine.isEmpty()) {
                return;
            }
            
            if (!requestLine.startsWith("GET")) {
                sendMethodNotAllowedResponse(os);
                return;
            }

            String[] tokens = requestLine.split(" ");
            String fileName = tokens[1].equals("/") ? "/index.html" : tokens[1];
            
            if (fileName.equals("/favicon.ico")) {
                Path faviconPath = Paths.get("./favicon.ico");
                if (Files.exists(faviconPath)) {
                    sendFaviconResponse(os, faviconPath);
                } else {
                    sendNotFoundResponse(os);
                }
                return;
            }
            
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
            e.printStackTrace();
        } finally {
            try {
                socket.close();
            } catch (IOException e) {
                System.err.println("Socket closing exception: " + e.getMessage());
            }
        }
    }

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

    private void sendNotFoundResponse(DataOutputStream os) throws IOException {
        String response = "<html><body><h1>404 Not Found</h1></body></html>";
        os.writeBytes("HTTP/1.1 404 Not Found\r\n");
        os.writeBytes("Content-Type: text/html\r\n");
        os.writeBytes("Content-Length: " + response.length() + "\r\n");
        os.writeBytes("\r\n");
        os.writeBytes(response);
        os.flush();
    }

    private void sendMethodNotAllowedResponse(DataOutputStream os) throws IOException {
        String response = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        os.writeBytes("HTTP/1.1 405 Method Not Allowed\r\n");
        os.writeBytes("Content-Type: text/html\r\n");
        os.writeBytes("Content-Length: " + response.length() + "\r\n");
        os.writeBytes("\r\n");
        os.writeBytes(response);
        os.flush();
    }

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
