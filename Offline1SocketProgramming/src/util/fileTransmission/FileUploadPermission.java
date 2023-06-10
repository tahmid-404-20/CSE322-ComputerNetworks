package util.fileTransmission;

import java.io.Serializable;

public class FileUploadPermission implements Serializable {
    public int fileId;
    public String fileName;
    public int chunkSize;
    public String text;

    // granting permission


    public FileUploadPermission(int fileId, String fileName, int chunkSize, String text) {
        this.fileId = fileId;
        this.fileName = fileName;
        this.chunkSize = chunkSize;
        this.text = text;
    }

    // not granting permission
    public FileUploadPermission(String text) {
        this.text = text;
    }
}
