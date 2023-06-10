package util.fileUpload;

import java.io.Serializable;

public class InitiateFileUpload implements Serializable {
    public String fileName;
    public String fileType;
    public long fileSize;

    public InitiateFileUpload(String fileName, String fileType, long fileSize) {
        this.fileName = fileName;
        this.fileType = fileType;
        this.fileSize = fileSize;
    }
}
