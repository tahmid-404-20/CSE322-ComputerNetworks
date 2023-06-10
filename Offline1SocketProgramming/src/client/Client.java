package client;

import server.Server;
import util.NetworkUtil;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Scanner;

public class Client {
    public static final int SOCKET_TIMEOUT = 5000;

    public NetworkUtil networkUtil;

    public Client(String serverAddress, int serverPort) {
        Scanner scr = new Scanner(System.in);
        try {

            networkUtil = new NetworkUtil(serverAddress, serverPort);
            String message = (String) networkUtil.read();
            System.out.print(message);

            String name = scr.nextLine();
            networkUtil.write(name);

            String reply = (String) networkUtil.read();
            System.out.println(reply);

            if(reply.equalsIgnoreCase("User already logged in. Please try again")){
                networkUtil.closeConnection();
                System.exit(0);
            }

            // login successful, open read and write thread
            new ClientReadThread(networkUtil);
            new ClientWriteThread(networkUtil);
        } catch (Exception e) {
            System.out.println(e);
        }
    }

    public static void main(String[] args) throws IOException {
        String serverAddress = "127.0.0.1";
        Client client = new Client(serverAddress, Server.PORT);
//        Socket socket = null;
//        FileInputStream fileInputStream = null;
//        OutputStream outputStream = null;
//
//        try {
//            // Create socket connection to the server
//            socket = new Socket("localhost", 6000);
//
//            // Create file input stream to read the file
//            fileInputStream = new FileInputStream("1905002_BSc_CSE_JAN2023Registration.pdf");
//
//            // Create output stream to send data to the server
//            outputStream = socket.getOutputStream();
//
//            // Define chunk size (in bytes)
//            int chunkSize = 1024;
//            byte[] buffer = new byte[chunkSize];
//            int bytesRead;
//
//            // Read data from the file and send it to the server
//            while ((bytesRead = fileInputStream.read(buffer)) != -1) {
//                outputStream.write(buffer, 0, bytesRead);
//            }
//
//            System.out.println("File sent successfully.");
//        } finally {
//            // Close all resources
//            if (outputStream != null) {
//                outputStream.close();
//            }
//
//            if (fileInputStream != null) {
//                fileInputStream.close();
//            }
//
//            if (socket != null) {
//                socket.close();
//            }
//        }
    }

    void uploadFile(String fileType, String fileName, int chunkSize) throws IOException {
        File file = new File(fileName);
        networkUtil.write("file,upload," + fileType + "," + fileName+ "," + file.length());
        FileInputStream fileInputStream = new FileInputStream(fileName);


        // Create output stream to send data to the server
        OutputStream outputStream = networkUtil.socket.getOutputStream();

        // Define chunk size (in bytes)
//            int chunkSize = 1024;
        byte[] buffer = new byte[chunkSize];
        int bytesRead;

        // Read data from the file and send it to the server
        while ((bytesRead = fileInputStream.read(buffer)) != -1) {
            outputStream.write(buffer, 0, bytesRead);
            outputStream.flush();
        }

        fileInputStream.close();
        System.out.println("File sent successfully.");

    }
}
