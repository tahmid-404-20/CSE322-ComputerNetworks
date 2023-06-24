package client;

import server.Server;
import util.NetworkUtil;
import util.fileDownload.*;
import util.fileUpload.*;
import util.lookUps.LookUpRequest;
import util.lookUps.LookUpResponse;
import util.message.SendRequest;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.StringTokenizer;

public class Client {
    public static final int SOCKET_TIMEOUT = 30000;

    public NetworkUtil nu;
    private CurrentFileDownloadInfo currentFileDownloadInfo;

    public Client(String serverAddress, int serverPort) {
        Scanner scr = new Scanner(System.in);
        try {

            nu = new NetworkUtil(serverAddress, serverPort);
            String message = (String) nu.read();
            System.out.print(message);

            String name = scr.nextLine();
            nu.write(name);

            String reply = (String) nu.read();
            System.out.println(reply);

            if (reply.equalsIgnoreCase("User already logged in. Please try again")) {
                nu.closeConnection();
                System.exit(0);
            }

            // login successful, open read and write thread
            start(scr);
//            new ClientReadThread(nu);
//            new ClientWriteThread(nu);
        } catch (Exception e) {
            System.out.println(e);
        }
    }

    public static void main(String[] args) throws IOException {
        String serverAddress = "127.0.0.1";
        Client client = new Client(serverAddress, Server.PORT);
    }

    void start(Scanner scr) {
        while (true) {
            if(!write(scr)) {
                continue;
            }
            try {
                Object o = nu.read();

                if (o instanceof FileUploadPermission fUPerm) {
                    if (fUPerm.text.equalsIgnoreCase("Permission Granted")) {
                        System.out.println("Permission granted to upload file");
                        uploadFile(fUPerm.fileName, fUPerm.fileId, fUPerm.chunkSize);
                    } else {
                        System.out.println(fUPerm.text);
                    }
                } else if (o instanceof FileDownloadPermission fDPerm) {
                    if (fDPerm.text.equalsIgnoreCase("File Found. Download Starting")) {
                        System.out.println("Starting download");
                        currentFileDownloadInfo = new CurrentFileDownloadInfo(fDPerm.fileName);
                        downloadFile();
                    } else {
                        System.out.println(fDPerm.text);
                    }
                } else if (o instanceof LookUpResponse LUR) {
                    System.out.println(LUR.requestDetails + "\n" + LUR.responseText);
                } else if (o instanceof String s) {
                    if (s.equalsIgnoreCase("Logout Successful")) {
                        nu.closeConnection();
                        System.out.println(s);
                        break;
                    } else {
                        System.out.println(s);
                    }
                }

            } catch (IOException | ClassNotFoundException e) {
                e.printStackTrace();
            }
        }
    }


    boolean write(Scanner scr) {
        System.out.print("Enter a command: ");
        String s = scr.nextLine();

        StringTokenizer st = new StringTokenizer(s, ",");
        List<String> tokens = new ArrayList<>();
        while (st.hasMoreTokens()) {
            tokens.add(st.nextToken());
        }

        String command = tokens.get(0).trim();
        // file -> upload, download -> self, otherDownload(for download only)
        if (command.equalsIgnoreCase("file")) {
            if (tokens.get(1).trim().equalsIgnoreCase("upload")) {
                String fileType = tokens.get(2).trim();
                String fileName = tokens.get(3).trim();
                File file = new File(fileName);

                if (file.exists()) {
                    long fileSize = file.length();
                    try {
                        System.out.println("Sending file upload request");
                        nu.write(new InitiateFileUpload(fileName, fileType, fileSize));
                        return true;
                    } catch (IOException e) {
                        e.printStackTrace();
                        return false;
                    }
                } else {
                    System.out.println("File does not exist");
                    return false;
                }
//
            } else if (tokens.get(1).trim().equalsIgnoreCase("download")) {
                if (tokens.size() == 5) { // download self file
                    String fileType = tokens.get(3).trim();
                    String fileName = tokens.get(4).trim();

                    try {
                        nu.write(new InitiateSelfFileDownload(fileName, fileType));
                        return true;
                    } catch (IOException e) {
                        e.printStackTrace();
                        return false;
                    }
                } else {  // download others file
                    String clientName = tokens.get(2).trim();
                    String fileName = tokens.get(3).trim();

                    try {
                        nu.write(new InitiateOtherFileDownload(clientName, fileName));
                        return true;
                    } catch (IOException e) {
                        e.printStackTrace();
                        return false;
                    }
                }
            } else if (tokens.get(1).trim().equalsIgnoreCase("lookUpOthers")) {
                try {
                    nu.write(new LookUpRequest("LookUp Other's Files"));
                    return true;
                } catch (IOException e) {
                    e.printStackTrace();
                    return false;
                }

            } else if (tokens.get(1).trim().equalsIgnoreCase("lookUpOwn")) {
                try {
                    nu.write(new LookUpRequest("LookUp Own Files"));
                    return true;
                } catch (IOException e) {
                    e.printStackTrace();
                    return false;
                }
            } else {
                System.out.println("Invalid command");
                return false;
            }
        } else if (command.equalsIgnoreCase("lookUpOthers")) {
            try {
                nu.write(new LookUpRequest("LookUp Other Clients"));
                return true;
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        } else if (command.equalsIgnoreCase("lookUpUnreadMessages")) {
            try {
                nu.write(new LookUpRequest("LookUp Unread Messages"));
                return true;
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        } else if (command.equalsIgnoreCase("request")) {
            String requestDescription = tokens.get(1).trim();

            try {
                nu.write(new SendRequest(requestDescription));
                return true;
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        } else if (command.equalsIgnoreCase("respond")) {
            long requestId = Long.parseLong(tokens.get(1).trim());
            String fileName = tokens.get(2).trim();

            File file = new File(fileName);

            if (file.exists()) {
                long fileSize = file.length();
                try {
                    System.out.println("Sending file upload request to respond to request");
                    nu.write(new InitiateFileUpload(fileName, fileSize, requestId));
                    return true;
                } catch (IOException e) {
                    e.printStackTrace();
                    return false;
                }
            } else {
                System.out.println("File does not exist");
                return false;
            }

        } else if (command.equalsIgnoreCase("logout")) {
            try {
                nu.write("Logout");
                return true;
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        } else {
            System.out.println("Invalid command");
            return false;
        }
    }

    void uploadFile(String fileName, int fileId, int chunkSize) throws IOException, ClassNotFoundException {
        long fileLength = new File(fileName).length();
        System.out.println("File Name: " + fileName);
        FileInputStream fileInputStream = new FileInputStream(fileName);

        byte[] buffer = new byte[chunkSize];
        int bytesRead;

        // Read data from the file and send it to the server
        int sentChunks = 0;
        int chunksToBeSent = (int) (fileLength / chunkSize) + (fileLength % chunkSize == 0 ? 0 : 1);
        while ((bytesRead = fileInputStream.read(buffer)) != -1) {
            nu.write(new FileUploadChunk(fileId, buffer, bytesRead));
            buffer = new byte[chunkSize];  // otherwise the last chunk will be sent multiple times

            nu.socket.setSoTimeout(Client.SOCKET_TIMEOUT);
            try {
                Object o = nu.read();
                if (o instanceof FileUploadChunkACK fUChunkACK) {
                    sentChunks++;
                    System.out.println("Server received chunk " + sentChunks + "(" + fUChunkACK.chunkSize + ") of " + chunksToBeSent + " chunks");
                    if (sentChunks == chunksToBeSent) {
                        // send completion message to server
                        nu.socket.setSoTimeout(0);  // timeOut is only for fileTransfer
                        nu.write(new FileUploadTermination(fileId, "File Upload Complete"));

                        Object ob = nu.read();
                        if (ob instanceof FileUploadTermination fUTerm) {
                            System.out.println(fUTerm.text);
                        }
                        break;
                    }
                }
            } catch (SocketTimeoutException e) {
                System.out.println("Socket timed out");
                nu.socket.setSoTimeout(0);
                nu.write(new FileUploadTermination(fileId, "Socket Timed Out"));
                break;
            }
        }

        fileInputStream.close();
    }

    void downloadFile() {
        try {
            while (true) {
                Object o = nu.read();

                if (o instanceof FileDownloadChunk fDChunk) {
                    currentFileDownloadInfo.addChunk(fDChunk.bytes, fDChunk.chunkSize);
                    System.out.println("Received chunk " + fDChunk.chunkSize + " bytes of file " + currentFileDownloadInfo.fileName + " from Server");
                } else if (o instanceof FileDownloadTermination fDTerm) {

                    if (fDTerm.text.equalsIgnoreCase("File Download Complete")) {
                        System.out.println("Download complete");
                        currentFileDownloadInfo.saveFile();
                    } else {
                        System.out.println(fDTerm.text);
                    }
                    currentFileDownloadInfo = null;
                    return;
                }
            }
        } catch (IOException | ClassNotFoundException e) {
            e.printStackTrace();
        }
    }
}
