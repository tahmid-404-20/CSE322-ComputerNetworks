package client;

import util.NetworkUtil;
import util.fileDownload.FileDownloadChunk;
import util.fileDownload.FileDownloadPermission;
import util.fileDownload.FileDownloadTermination;
import util.fileUpload.FileUploadChunk;
import util.fileUpload.FileUploadChunkACK;
import util.fileUpload.FileUploadPermission;
import util.fileUpload.FileUploadTermination;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.SocketTimeoutException;

public class ClientReadThread implements Runnable{
    NetworkUtil nu;
    Thread t;
    CurrentFileDownloadInfo currentFileDownloadInfo;

    public ClientReadThread(NetworkUtil networkUtil) {
        this.nu = networkUtil;
        t=new Thread(this);
        t.start();
    }

    @Override
    public void run() {
        while(true) {
            try {
                Object o = nu.read();

                if (o instanceof FileUploadPermission fUPerm) {
                    if (fUPerm.text.equalsIgnoreCase("Permission Granted")) {
                        System.out.println("Permission granted to upload file");
                        uploadFile(fUPerm.fileName, fUPerm.fileId, fUPerm.chunkSize);
                    } else {
                        System.out.println(fUPerm.text);
                    }
                }

                if(o instanceof FileDownloadPermission fDPerm) {
                    if(fDPerm.text.equalsIgnoreCase("File Found. Download Starting")) {
                        System.out.println("Starting download");
                        currentFileDownloadInfo = new CurrentFileDownloadInfo(fDPerm.fileName);
                    } else {
                        System.out.println(fDPerm.text);
                    }
                }

                if(o instanceof FileDownloadChunk fDChunk) {
                    currentFileDownloadInfo.addChunk(fDChunk.bytes, fDChunk.chunkSize);
                    System.out.println("Received chunk " + fDChunk.chunkSize + " bytes of file " + currentFileDownloadInfo.fileName +" from Server");
                }

                if(o instanceof FileDownloadTermination fDTerm) {

                    if(fDTerm.text.equalsIgnoreCase("File Download Complete")) {
                        System.out.println("Download complete");
                        currentFileDownloadInfo.saveFile();
                    } else {
                        System.out.println(fDTerm.text);
                    }
                    currentFileDownloadInfo = null;
                }


            } catch (IOException | ClassNotFoundException e) {
                e.printStackTrace();
            }
        }
    }

    void uploadFile(String fileName, int fileId , int chunkSize) throws IOException, ClassNotFoundException {
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
                if(o instanceof FileUploadChunkACK fUChunkACK) {
                    sentChunks++;
                    System.out.println("Server received chunk " + sentChunks + "(" + fUChunkACK.chunkSize + ") of " +  chunksToBeSent + " chunks");
                    if(sentChunks == chunksToBeSent) {
                        // send completion message to server
                        nu.socket.setSoTimeout(0);  // timeOut is only for fileTransfer
                        nu.write(new FileUploadTermination(fileId, "File Upload Complete"));

                        Object ob = nu.read();
                        if(ob instanceof FileUploadTermination fUTerm) {
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
}
