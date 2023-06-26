package server;

import util.NetworkUtil;

import java.io.File;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class Server {

    public static final int PORT = 6000;
    public static final long MAX_BUFFER_SIZE = 10000*1024;
    public static final int MAX_CHUNK_SIZE = 1024;
    public static final int MIN_CHUNK_SIZE = 10;

    public static void main(String[] args) throws IOException {
        new Server();
    }

    private ServerSocket serverSocket;
    private final Map<String, NetworkUtil> activeClientMap;
    private final Map<String, ClientInfo> clientInfoMap;
    private ServerMessageDump serverMessageDump;
    private final ServerBufferState serverBufferState;

    Server() {
        activeClientMap= new ConcurrentHashMap<>();
        serverBufferState = new ServerBufferState();
        clientInfoMap = new ConcurrentHashMap<>();
        serverMessageDump = new ServerMessageDump(clientInfoMap);
        gatherClientInfo();

        try {
            serverSocket = new ServerSocket(PORT);
            System.out.println("Server started, waiting for client...");

            while (true) {
                Socket clientSocket = serverSocket.accept();
                new ServerLoginThread(activeClientMap, clientInfoMap, serverMessageDump, serverBufferState, clientSocket);
            }
        } catch (Exception e) {
            System.out.println("server.Server starts:" + e);
        }
    }

    private void gatherClientInfo() {
        for(String name : lookUpClientNames()) {
            clientInfoMap.put(name, new ClientInfo(name));
        }
    }

    static List<String> lookUpClientNames() {
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
