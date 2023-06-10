package server;

import util.NetworkUtil;
import util.fileDownload.*;
import util.fileUpload.*;

import java.io.*;
import java.net.SocketException;
import java.util.HashMap;
import java.util.Random;

public class ServerReadThread implements Runnable {
    String clientName;
    NetworkUtil nu;
    Thread t;
    private HashMap<String, ClientInfo> activeClientMap;
    private ServerBufferState serverBufferState;
    private CurrentFileUploadInfo currentFileUploadInfo;

    public ServerReadThread(String clientName, NetworkUtil nu, HashMap<String,
            ClientInfo> activeClientMap, ServerBufferState serverBufferState) {
        this.clientName = clientName;
        this.nu = nu;
        this.activeClientMap = activeClientMap;
        this.serverBufferState = serverBufferState;
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

                        currentFileUploadInfo = new CurrentFileUploadInfo(fileId,
                                initiateFileUpload.fileName, initiateFileUpload.fileType, initiateFileUpload.fileSize);
                        nu.write(new FileUploadPermission(fileId, initiateFileUpload.fileName, chunkSize, "Permission Granted"));
                    } else {
                        nu.write(new FileUploadPermission("Server Buffer Full. Transfer Aborted"));
                    }
                }

                if (o instanceof FileUploadChunk fUChunk) {
                    serverBufferState.addChunk(fUChunk.fileId, fUChunk.bytes, fUChunk.chunkSize);
                    System.out.println("Received chunk " + fUChunk.chunkSize + "bytes of file " + fUChunk.fileId + " from " + clientName);
                    // send ack

                    // for testing timeOut, just comment the line below
                    nu.write(new FileUploadChunkACK(fUChunk.fileId, fUChunk.chunkSize));
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
                            nu.write(new FileUploadTermination(fUTerm.fileId, "Server Received File Successfully"));
                        } else {
                            // an error occurred, discard file from buffer
                            serverBufferState.removeFileFromBuffer(fUTerm.fileId);
                            nu.write(new FileUploadTermination(fUTerm.fileId, "An error occurred, file discarded"));
                        }
                    } else if (fUTerm.text.equalsIgnoreCase("Socket Timed Out")) {
                        // discard file from buffer
                        System.out.println("Socket Timed Out");
                        serverBufferState.removeFileFromBuffer(fUTerm.fileId);
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

            } catch (IOException | ClassNotFoundException e) {
                if (e instanceof SocketException) {
                    try {
                        if (currentFileUploadInfo != null) {
                            serverBufferState.removeFileFromBuffer(currentFileUploadInfo.fileId);
                            System.out.println("Removed all the chunks of file(fileId:" + currentFileUploadInfo.fileId + ") from buffer");
                            currentFileUploadInfo = null;
                        }
                        nu.closeConnection();
                        activeClientMap.remove(clientName);
                        System.out.println("Client " + clientName + " disconnected");
                        break;
                    } catch (IOException ioException) {
                        ioException.printStackTrace();
                    }
                } else {
                    System.out.println("Exception in ServerReadThread: " + e);
                }
            }


//                String s = (String) nu.read();
//
//                // tokenize s using , as delimiter using StringTokenizer
//                // send the tokenized string to all clients
//                StringTokenizer st = new StringTokenizer(s, ",");
//                List<String> tokens = new ArrayList<>();
//
//                while (st.hasMoreTokens()) {
//                    tokens.add(st.nextToken());
//                }
//
//                String command = tokens.get(0);
//                if(command.equalsIgnoreCase("file")) {
//                    if(tokens.get(1).equalsIgnoreCase("upload")) {
//                        String fileType = tokens.get(2);
//                        String fileName = tokens.get(3);
//                        int fileSize = Integer.parseInt(tokens.get(4));
//                        receiveFile(fileType, fileName, 1024, fileSize);
//                        while(true);
//                    } else if(tokens.get(1).equalsIgnoreCase("download")) {
//
//                    }
//                }

//            } catch (SocketException e) {
//                System.out.println("Error in ServerReadThread.run(): " + e);
//
//            }
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

    /*
    void receiveFile(String fileType, String fileName, int chunkSize, int fileSize) throws IOException {
        FileOutputStream fileOutputStream =
                new FileOutputStream("files/" + clientName + "/" + fileType +
                        "/" + fileName);


            // Define chunk size (in bytes)
//            int chunkSize = 1024;
            byte[] buffer = new byte[chunkSize];
            int bytesRead;

            // Read data from the input stream and write to the file
            while (fileSize>0 && (bytesRead = nu.socket.getInputStream().read(buffer)) != -1) {
                System.out.println("Bytes read: " + bytesRead);
                fileOutputStream.write(buffer, 0, bytesRead);
                System.out.println("Received Byte");
                fileSize -= bytesRead;
            }

            fileOutputStream.close();
            System.out.println("File received successfully.");
    }
    */

}
