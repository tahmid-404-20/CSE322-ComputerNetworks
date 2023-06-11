package client;

import util.NetworkUtil;
import util.fileDownload.InitiateOtherFileDownload;
import util.fileDownload.InitiateSelfFileDownload;
import util.fileUpload.InitiateFileUpload;
import util.lookUps.LookUpRequest;
import util.message.SendRequest;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.StringTokenizer;

public class ClientWriteThread implements Runnable {
    NetworkUtil networkUtil;
    Thread t;

    public ClientWriteThread(NetworkUtil networkUtil) {
        this.networkUtil = networkUtil;
        t = new Thread(this);
        t.start();
    }

    @Override
    public void run() {
        Scanner scr = new Scanner(System.in);
        while (true) {
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

                    if(file.exists()) {
                        long fileSize = file.length();
                        try {
                            System.out.println("Sending file upload request");
                            networkUtil.write(new InitiateFileUpload(fileName, fileType, fileSize));
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    } else {
                        System.out.println("File does not exist");
                    }
//
                } else if (tokens.get(1).trim().equalsIgnoreCase("download")) {
                    if(tokens.size() == 5) { // download self file
                        String fileType = tokens.get(3).trim();
                        String fileName = tokens.get(4).trim();

                        try {
                            networkUtil.write(new InitiateSelfFileDownload(fileName, fileType));
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    } else {  // download others file
                        String clientName = tokens.get(2).trim();
                        String fileName = tokens.get(3).trim();

                        try {
                            networkUtil.write(new InitiateOtherFileDownload(clientName, fileName));
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                } else if(tokens.get(1).trim().equalsIgnoreCase("lookUpOthers")) {
                    try {
                        networkUtil.write(new LookUpRequest("LookUp Other's Files"));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                } else if(tokens.get(1).trim().equalsIgnoreCase("lookUpOwn")) {
                    try {
                        networkUtil.write(new LookUpRequest("LookUp Own Files"));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                } else {
                    System.out.println("Invalid command");
                }
            } else if (command.equalsIgnoreCase("lookUpOthers")) {
                try {
                    networkUtil.write(new LookUpRequest("LookUp Other Clients"));
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else if (command.equalsIgnoreCase("lookUpUnreadMessages")) {
                try {
                    networkUtil.write(new LookUpRequest("LookUp Unread Messages"));
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else if(command.equalsIgnoreCase("request")) {
                String requestDescription = tokens.get(1).trim();

                try {
                    networkUtil.write(new SendRequest(requestDescription));
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else if(command.equalsIgnoreCase("respond")) {
                long requestId = Long.parseLong(tokens.get(1).trim());
                String fileName = tokens.get(2).trim();

                File file = new File(fileName);

                if(file.exists()) {
                    long fileSize = file.length();
                    try {
                        System.out.println("Sending file upload request to respond to request");
                        networkUtil.write(new InitiateFileUpload(fileName, fileSize, requestId));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                } else {
                    System.out.println("File does not exist");
                }

            } else if(command.equalsIgnoreCase("logout")) {
                try {
                    networkUtil.write("Logout");
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else
             {
                System.out.println("Invalid command");
            }
        }
    }
}
