package server;

import util.NetworkUtil;

import java.io.File;
import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ServerLoginThread implements Runnable {
    private final Map<String, NetworkUtil> activeClientMap;
    private final Map<String, ClientInfo> clientInfoMap;
    private ServerMessageDump serverMessageDump;
    private final ServerBufferState serverBufferState;
    private Socket clientSocket;
    Thread t;

    public ServerLoginThread(Map<String, NetworkUtil> activeClientMap, Map<String, ClientInfo> clientInfoMap,
                             ServerMessageDump serverMessageDump, ServerBufferState serverBufferState, Socket clientSocket) {
        this.activeClientMap = activeClientMap;
        this.clientInfoMap = clientInfoMap;
        this.serverMessageDump = serverMessageDump;
        this.serverBufferState = serverBufferState;
        this.clientSocket = clientSocket;
        t = new Thread(this);
        t.start();
    }

    @Override
    public void run() {
        try {
            serve(clientSocket);
        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    private void serve(Socket clientSocket) throws IOException, ClassNotFoundException {
        NetworkUtil networkUtil = new NetworkUtil(clientSocket);
        networkUtil.write("Connected to server. Please Send your name: ");

        String userName = (String) networkUtil.read();
        if (activeClientMap.containsKey(userName)) {  // user already online
            networkUtil.write("User already logged in. Please try again");
            networkUtil.closeConnection();
        } else {
            // now see if logged in before, then no need to create a directory
            boolean found = false;
            for (String name : Server.lookUpClientNames()) {
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
}
