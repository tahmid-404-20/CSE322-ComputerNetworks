package client;

import util.NetworkUtil;
import util.fileTransmission.InitiateFileUpload;

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
                } else if (tokens.get(1).equalsIgnoreCase("download")) {

                }
            }
        }
    }
}
