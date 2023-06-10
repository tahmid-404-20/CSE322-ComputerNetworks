package util.fileUpload;

import java.io.Serializable;

public class InitiateFileUpload implements Serializable {
    public String fileName;
    public String fileType;
    public long fileSize;
    public boolean isResponseToRequest;
    public long requestId;

    public InitiateFileUpload(String fileName, String fileType, long fileSize) {
        this.fileName = fileName;
        this.fileType = fileType;
        this.fileSize = fileSize;
        this.isResponseToRequest = false;
    }

    public InitiateFileUpload(String fileName, long fileSize, long requestId) {
        this.fileName = fileName;
        this.fileType = "public";
        this.fileSize = fileSize;
        this.requestId = requestId;
        this.isResponseToRequest = true;
    }
}
