package server;

import util.NetworkUtil;

import java.io.File;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class Server {

    public static final int PORT = 6000;
    public static final long MAX_BUFFER_SIZE = 10000*1024;
    public static final int MAX_CHUNK_SIZE = 1024;
    public static final int MIN_CHUNK_SIZE = 10;

    public static void main(String[] args) throws IOException {
        new Server();
//        final int PORT = 6000;

//        ServerSocket serverSocket = null;
//        Socket socket = null;
//        InputStream inputStream = null;
//        FileOutputStream fileOutputStream = null;
//
//        try {
//            // Start server and listen on a specific port
//            serverSocket = new ServerSocket(PORT);
//            System.out.println("Server started. Waiting for client...");
//
//            // Accept client connection
//            socket = serverSocket.accept();
//            System.out.println("Client connected.");
//
//            // Create input stream to receive data from the client
//            inputStream = socket.getInputStream();
//
//            // Create file output stream to write data to the file and open a folder to save the file
//            new File("files").mkdirs();
//            fileOutputStream = new FileOutputStream("files/received_file3.pdf");
//
//
//            // Define chunk size (in bytes)
//            int chunkSize = 1024;
//            byte[] buffer = new byte[chunkSize];
//            int bytesRead;
//
//            // Read data from the input stream and write to the file
//            while ((bytesRead = inputStream.read(buffer)) != -1) {
//                System.out.println("Bytes read: " + bytesRead);
//                fileOutputStream.write(buffer, 0, bytesRead);
//            }
//
//            System.out.println("File received successfully.");
//        } finally {
//            // Close all resources
//            if (fileOutputStream != null) {
//                fileOutputStream.close();
//            }
//
//            if (inputStream != null) {
//                inputStream.close();
//            }
//
//            if (socket != null) {
//                socket.close();
//            }
//
//            if (serverSocket != null) {
//                serverSocket.close();
//            }
//
//            System.out.println("Server closed.");
//        }
    }

    private ServerSocket serverSocket;
    private final HashMap<String, NetworkUtil> activeClientMap;
    private final HashMap<String, ClientInfo> clientInfoMap;
    private ServerMessageDump serverMessageDump;
    private final ServerBufferState serverBufferState;

    Server() {
        activeClientMap= new HashMap<>();
        serverBufferState = new ServerBufferState();
        clientInfoMap = new HashMap<>();
        serverMessageDump = new ServerMessageDump(clientInfoMap);
        gatherClientInfo();

        try {
            serverSocket = new ServerSocket(PORT);
            System.out.println("Server started, waiting for client...");

            while (true) {
                Socket clientSocket = serverSocket.accept();
                serve(clientSocket);
            }
        } catch (Exception e) {
            System.out.println("server.Server starts:" + e);
        }
    }

    public void serve(Socket clientSocket) throws IOException, ClassNotFoundException {
        NetworkUtil networkUtil = new NetworkUtil(clientSocket);
        networkUtil.write("Connected to server. Please Send your name: ");

        String userName = (String) networkUtil.read();
        if (activeClientMap.containsKey(userName)) {  // user already online
            networkUtil.write("User already logged in. Please try again");
            networkUtil.closeConnection();
        } else {
            // now see if logged in before, then no need to create a directory
            boolean found = false;
            for (String name : lookUpClientNames()) {
                if (name.equals(userName)) {
                    found = true;
                    networkUtil.write("Login Successful");
                    activeClientMap.put(userName, networkUtil);
                    new ServerReadThread(userName,networkUtil, activeClientMap, serverBufferState, clientInfoMap, serverMessageDump);
                    break;
                }
            }

            // need to open a directory
            if(!found) {
                // open a new directory for the user, add to active client list
                new File("files/" + userName + "/public").mkdirs();
                new File("files/" + userName + "/private").mkdirs();
                networkUtil.write("Login Successful");
                activeClientMap.put(userName, networkUtil);
                clientInfoMap.put(userName, new ClientInfo(userName));  // no entry in clientInfoMap before

                new ServerReadThread(userName,networkUtil, activeClientMap, serverBufferState, clientInfoMap, serverMessageDump);
            }
        }
    }

    private void gatherClientInfo() {
        for(String name : lookUpClientNames()) {
            clientInfoMap.put(name, new ClientInfo(name));
        }
    }

    private List<String> lookUpClientNames() {
        String directoryPath = "files";
        File directory = new File(directoryPath);
        File[] files = directory.listFiles();
        List<String> fileNames = new ArrayList<>();

        if (files != null) {
            for (File file : files) {
                if (file.isDirectory()) {
                    fileNames.add(file.getName());
                }
            }
        }
        return fileNames;
    }
}
