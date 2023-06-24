package server;

import client.Client;
import util.NetworkUtil;
import util.fileDownload.*;
import util.fileUpload.*;
import util.lookUps.LookUpRequest;
import util.lookUps.LookUpResponse;
import util.message.Message;
import util.message.SendRequest;

import java.io.*;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Random;

public class ServerReadThread implements Runnable {
    String clientName;
    NetworkUtil nu;
    Thread t;
    ServerMessageDump serverMessageDump;
    private HashMap<String, NetworkUtil> activeClientMap;
    private ServerBufferState serverBufferState;
    private HashMap<String, ClientInfo> clientInfoMap;
    private CurrentFileUploadInfo currentFileUploadInfo;

    public ServerReadThread(String clientName, NetworkUtil nu, HashMap<String, NetworkUtil> activeClientMap,
                            ServerBufferState serverBufferState, HashMap<String, ClientInfo> clientInfoMap,
                            ServerMessageDump serverMessageDump) {

        this.clientName = clientName;
        this.nu = nu;
        this.activeClientMap = activeClientMap;
        this.serverBufferState = serverBufferState;
        this.clientInfoMap = clientInfoMap;
        this.serverMessageDump = serverMessageDump;
        t = new Thread(this);
        t.start();
    }

    @Override
    public void run() {
        while (true) {
            try {
                Object o = nu.read();
                if (o instanceof InitiateFileUpload initiateFileUpload) {
                    if ((serverBufferState.occupiedSize + initiateFileUpload.fileSize) <= Server.MAX_BUFFER_SIZE) {

                        String filePath = "files/" + clientName + "/" + initiateFileUpload.fileType + "/" + initiateFileUpload.fileName;

                        if (new File(filePath).exists()) {
                            nu.write(new FileUploadPermission("File Already Exists. Transfer Aborted"));
                            continue;
                        }

                        Random random = new Random();
                        int chunkSize = random.nextInt(Server.MAX_CHUNK_SIZE - Server.MIN_CHUNK_SIZE + 1) + Server.MIN_CHUNK_SIZE;

                        // generate unique fileId
                        int fileId = random.nextInt(10000);
                        while (serverBufferState.currentBuffer.containsKey(fileId)) {
                            fileId = random.nextInt(10000);
                        }

                        if (initiateFileUpload.isResponseToRequest) {
                            currentFileUploadInfo = new CurrentFileUploadInfo(fileId,
                                    initiateFileUpload.fileName, initiateFileUpload.fileType, initiateFileUpload.fileSize, initiateFileUpload.requestId);
                        } else {
                            currentFileUploadInfo = new CurrentFileUploadInfo(fileId,
                                    initiateFileUpload.fileName, initiateFileUpload.fileType, initiateFileUpload.fileSize);
                        }

                        nu.write(new FileUploadPermission(fileId, initiateFileUpload.fileName, chunkSize, "Permission Granted"));
                    } else {
                        nu.write(new FileUploadPermission("Server Buffer Full. Transfer Aborted"));
                    }
                }

                if (o instanceof FileUploadChunk fUChunk) {
                    serverBufferState.addChunk(fUChunk.fileId, fUChunk.bytes, fUChunk.chunkSize);
                    System.out.println("Received chunk " + fUChunk.chunkSize + "bytes of file " + fUChunk.fileId + " from " + clientName);

                    // send ack
                    // for testing timeOut, just uncomment the block below
//                    try {
//                        Thread.sleep(Client.SOCKET_TIMEOUT + 2000);
//                    } catch (InterruptedException e) {
//                        e.printStackTrace();
//                    }
                    if (nu.socket.getInputStream().available() <= 0) {
                        nu.write(new FileUploadChunkACK(fUChunk.fileId, fUChunk.chunkSize));
                    }
                }

                if (o instanceof FileUploadTermination fUTerm) {  // either file upload complete or socket timed out, currentFileInfo = null at end
                    if (fUTerm.text.equalsIgnoreCase("File Upload Complete")) {
                        // check if file is complete
                        if (serverBufferState.isFileComplete(fUTerm.fileId, currentFileUploadInfo.fileSize)) {
                            // write to fileOutputBuffer and send ok message
                            String filePath = "files/" + clientName + "/" + currentFileUploadInfo.fileType + "/" + currentFileUploadInfo.fileName;
                            FileOutputStream fos = new FileOutputStream(filePath);
                            serverBufferState.writeFileToOutputBuffer(fUTerm.fileId, fos);

                            System.out.println("Received file successfully from " + clientName);

                            // if the file upload was a response to a request, send the message to the client
                            if (currentFileUploadInfo.isResponseToRequest) {
                                Message msg = serverMessageDump.getMessage(currentFileUploadInfo.requestId);
                                if (msg != null) {
                                    msg.message = "Hello from server. In response to your request (" + msg.message +
                                            "), file - " + currentFileUploadInfo.fileName + " has been uploaded successfully by " + clientName;
                                    clientInfoMap.get(msg.sender).addMessage(msg);
                                }
                            }

                            nu.write(new FileUploadTermination(fUTerm.fileId, "Server Received File Successfully"));
                        } else {
                            // an error occurred, discard file from buffer
                            serverBufferState.removeFileFromBuffer(fUTerm.fileId);
                            nu.write(new FileUploadTermination(fUTerm.fileId, "An error occurred, file discarded"));
                        }
                    } else if (fUTerm.text.equalsIgnoreCase("Socket Timed Out")) {
                        // discard file from buffer
                        System.out.println("Socket Timed Out");
                        if (currentFileUploadInfo != null) {
                            System.out.println("Discarding file " + currentFileUploadInfo.fileName + " from buffer");
                            serverBufferState.removeFileFromBuffer(fUTerm.fileId);
                        }
                    }

                    currentFileUploadInfo = null;
                }

                if (o instanceof InitiateSelfFileDownload initiateSFD) {
                    String filePath = "files/" + clientName + "/" + initiateSFD.fileType + "/" + initiateSFD.fileName;
                    sendFile(initiateSFD.fileName, filePath, Server.MAX_CHUNK_SIZE);
                }

                if (o instanceof InitiateOtherFileDownload initiateOFD) {
                    String filePath = "files/" + initiateOFD.userName + "/public/" + initiateOFD.fileName;
                    sendFile(initiateOFD.fileName, filePath, Server.MAX_CHUNK_SIZE);
                }

                if (o instanceof LookUpRequest LUR) {
                    String request = LUR.requestText;

                    if (request.equalsIgnoreCase("LookUp Own Files")) {
                        nu.write(new LookUpResponse("Response to LookUp Own Files", lookUpOwnFiles()));
                    } else if (request.equalsIgnoreCase("LookUp Other's Files")) {
                        nu.write(new LookUpResponse("Response to LookUp Other's Files", lookUpOthersFiles()));
                    } else if (request.equalsIgnoreCase("LookUp Other Clients")) {
                        nu.write(new LookUpResponse("Response to LookUp Other Clients", lookUpOtherClients()));
                    } else if (request.equalsIgnoreCase("LookUp Unread Messages")) {
                        nu.write(new LookUpResponse("Response to LookUp Unread Messages", lookUpUnreadMessages()));
                    }
                }

                if (o instanceof SendRequest SR) {
                    String description = SR.message;
                    serverMessageDump.addMessage(new Message(description, clientName));
                    nu.write(new LookUpResponse("Response to file request", "Server received request successfully"));
                }

                if (o instanceof String str) {
                    if (str.equalsIgnoreCase("Logout")) {
                        nu.write("Logout Successful");
                        try {
                            /*  wait for client to receive logout message,
                                otherwise client will throw SocketException
                                as this is a thread, it will not hamper other clients
                             */
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        logOut();
                        break;
                    }
                }

            } catch (IOException | ClassNotFoundException e) {
                if (e instanceof SocketException) {  // client disconnected
                    try {
                        if (currentFileUploadInfo != null) {
                            serverBufferState.removeFileFromBuffer(currentFileUploadInfo.fileId);
                            System.out.println("Removed all the chunks of file(fileId:" + currentFileUploadInfo.fileId + ") from buffer");
                            currentFileUploadInfo = null;
                        }

                        logOut();
                        break;
                    } catch (IOException ioException) {
                        ioException.printStackTrace();
                    }
                } else {
                    System.out.println("Exception in ServerReadThread: " + e);
                }
            }
        }
    }

    void sendFile(String fileName, String filePath, int chunkSize) throws IOException {
        try {
            FileInputStream fis = new FileInputStream(filePath);
            nu.write(new FileDownloadPermission(fileName, "File Found. Download Starting"));

            long fileLength = new File(fileName).length();
            byte[] buffer = new byte[chunkSize];
            int bytesRead;

            while ((bytesRead = fis.read(buffer)) != -1) {
                nu.write(new FileDownloadChunk(fileName, buffer, bytesRead));
                System.out.println("Sending file: " + fileName + " to " + clientName + "--> chunkSize: " + bytesRead);
                buffer = new byte[chunkSize]; // otherwise the last chunk will be sent multiple times
            }

            nu.write(new FileDownloadTermination(fileName, "File Download Complete"));

        } catch (FileNotFoundException e) {
            nu.write(new FileDownloadPermission(fileName, "File Not Found"));
        }
    }

    // lookUP Codes
    private String lookUpUnreadMessages() {
        List<String> messageList = new ArrayList<>();
        messageList.add("RequestId-MessageBody-SenderClientName\n");

        for (Message m : clientInfoMap.get(clientName).getUnreadMessages()) {
            messageList.add(m.requestId + "-" + m.message + "-" + m.sender + "\n");
        }

        // all unread messages are now read
        clientInfoMap.get(clientName).getUnreadMessages().clear();

        StringBuilder sb = new StringBuilder();
        for (String s : messageList) {
            sb.append(s);
        }

        return sb.toString();
    }

    private String lookUpOtherClients() {
        List<String> clientNames = new ArrayList<>();
        for (String clientName : clientInfoMap.keySet()) {
            if (!clientName.equalsIgnoreCase(this.clientName)) {
                clientNames.add(clientName + " ");

                if (activeClientMap.containsKey(clientName)) {
                    clientNames.add("- Online\n");
                } else {
                    clientNames.add("\n");
                }
            }
        }
        StringBuilder sb = new StringBuilder();
        for (String s : clientNames) {
            sb.append(s);
        }

        return sb.toString();
    }

    private String lookUpOwnFiles() {
        List<String> fileNames = new ArrayList<>();

        // private files
        String directoryPathPrivate = "files/" + clientName + "/private/";
        File directory = new File(directoryPathPrivate);
        File[] files = directory.listFiles();
        fileNames.add("Private Files:\n");
        if (files != null) {
            for (File file : files) {
                if (file.isFile()) {
                    fileNames.add(file.getName() + "\n");
                }
            }
        }

        // public files
        String directoryPathPublic = "files/" + clientName + "/public/";
        directory = new File(directoryPathPublic);
        files = directory.listFiles();
        fileNames.add("Public Files:\n");
        if (files != null) {
            for (File file : files) {
                if (file.isFile()) {
                    fileNames.add(file.getName() + "\n");
                }
            }
        }

        StringBuilder sb = new StringBuilder();
        for (String s : fileNames) {
            sb.append(s);
        }

        return sb.toString();
    }

    private String lookUpOthersFiles() {
        List<String> fileNames = new ArrayList<>();

        String directoryPath;
        for (String userName : clientInfoMap.keySet()) {
            if (userName.equals(clientName)) continue;

            directoryPath = "files/" + userName + "/public/";
            fileNames.add("Files of " + userName + ":\n");
            File directory = new File(directoryPath);
            File[] files = directory.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isFile()) {
                        fileNames.add(file.getName() + "\n");
                    }
                }
            }
        }

        StringBuilder sb = new StringBuilder();
        for (String s : fileNames) {
            sb.append(s);
        }

        return sb.toString();
    }

    private void logOut() throws IOException {
        nu.closeConnection();
        activeClientMap.remove(clientName);
        System.out.println("Client " + clientName + " disconnected");
    }
}
